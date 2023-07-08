#ifndef _MemoryMgr_hpp_
#define _MemoryMgr_hpp_
#include<stdlib.h>
#include<assert.h>
#include<mutex>

#ifdef _DEBUG
	#ifndef Xprintf
		#include<stdio.h>
		#define xPrintf(...) printf(__VA_ARGS__);fflush(stdout)
	#endif // !Xprintf
#else
	#ifndef xPrintf
		#define xPrintf(...)
	#endif // !xPrintf
#endif // _DEBUG

//最大内存单元尺寸
#define MAX_MEMORY_SIZE 1024

class MemoryAlloc;
//内存块  最小单元
class MemoryBlock
{
public:
	int nID;//内存块编号
	int nRef;//引用次数
	MemoryAlloc* pAlloc;// 所属的大内存块地址
	MemoryBlock* pNext;//下一块位置
	bool bPool;//是否在内存池中
private:
	int nnsize;//预留四字节  字节对齐 将32字节凑齐
};

//内存池->操作对象为向系统申请的内存空间
class MemoryAlloc
{
public:
	MemoryAlloc() {
		_pBuf = nullptr;
		_pHeader = nullptr;
		_nBlockSize = 0;
		_nSize = 0;
		xPrintf("MemoryAlloc Initiall...\n");
	}

	~MemoryAlloc() {
		if(_pBuf)
			free(_pBuf);
	}
	
	//向系统申请内存
	void* allocMemory(size_t nsize)
	{
		std::lock_guard<std::mutex> lg(_mutex);
		if (!_pBuf)
		{
			initMemory();
		}
		MemoryBlock* pReturn = nullptr;
		if (nullptr == _pHeader)
		{//如果大内存块中没有空间了  则再向系统申请空间
			pReturn = (MemoryBlock*)malloc(nsize + sizeof(MemoryBlock));
			pReturn->bPool = false;
			pReturn->nID = -1; 
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
		}
		else
		{
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;
			assert(0 == pReturn->nRef);
			pReturn->nRef = 1;
		}
		return ((char*)pReturn + sizeof(MemoryBlock));
	}
	
	//释放内存
	void freeMemory(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock* )((char*)pMem - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);
		if (pBlock->bPool)
		{
			std::lock_guard<std::mutex> lg(_mutex);
			if (--pBlock->nRef != 0)
			{
				return;
			}
			//将内存放回内存池中  以待后用
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else
		{
			//不在内存池中 说明是向系统申请的内存 直接释放
			free(pBlock);
		}
	}

	//初始化内存池
	void initMemory()
	{	
		xPrintf("initMemory:_nSize=%d,_nBlockSize=%d\n", _nSize, _nBlockSize);
		//断言
		assert(nullptr == _pBuf);
		if (_pBuf)	return;
		//计算内存池所内存需大小
		size_t bufSize = (_nSize + sizeof(MemoryBlock)) * _nBlockSize;
		size_t realSize = _nSize + sizeof(MemoryBlock);
		//向系统申请内存
		_pBuf = (char*)malloc(bufSize);

		//初始计划内存池
		_pHeader = (MemoryBlock*)_pBuf;
		_pHeader->bPool = true;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->pAlloc = this;
		_pHeader->pNext = nullptr;

		//遍历内存块 进行索引初始化
		MemoryBlock* pTemp1 = _pHeader;
		for (size_t n = 1; n < _nBlockSize; n++)
		{
			MemoryBlock* pTemp2 = (MemoryBlock*)(_pBuf + (n * realSize));
			pTemp2->bPool = true;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->pAlloc = this;
			pTemp2->pNext = nullptr;
			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2; 
		}
	}
protected:
	//内存池地址
	char* _pBuf;
	//内存单元的大小
	size_t _nSize;
	//内存单元的数量
	size_t _nBlockSize;
	//头部指针
	MemoryBlock* _pHeader;
	std::mutex _mutex;
};

//模板重写  便在声明成员变量时初始化
template<size_t nSize, size_t nBlocSize>
class MemoryAlloctor :public MemoryAlloc
{
public:
	MemoryAlloctor()
	{//内存对齐操作
		const size_t n = sizeof(void*);
		_nSize = (nSize / n) * n + (nSize % n ? n:0 );
		_nBlockSize = nBlocSize;
	}
};

//内存管理工具
class MemoryMgr
{
private:
	MemoryMgr() {
		init_szAlloc(0, 64, &_mem64);
		init_szAlloc(65, 128, &_mem128);
		init_szAlloc(129, 256, &_mem256);
		init_szAlloc(257, 512, &_mem512);
		init_szAlloc(513, 1024, &_mem1024);
	}
	~MemoryMgr()
	{

	}
public:
	static MemoryMgr& Instance()
	{//单例模式 静态
		static MemoryMgr mgr;
		return mgr;
	}
	//向内存池申请内存
	void* allocMem(size_t nsize)
	{
		if (nsize <= MAX_MEMORY_SIZE)
		{
			return _szAlloc[nsize]->allocMemory(nsize);
		}
		else
		{
			MemoryBlock*  pReturn = (MemoryBlock*)malloc(nsize + sizeof(MemoryBlock));
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			return ((char*)pReturn + sizeof(MemoryBlock));
		}	
	}
	//释放内存
	void freeMem(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		if (pBlock->bPool)
		{
			pBlock->pAlloc->freeMemory(pMem);
		}
		else
		{
			if(--pBlock->nRef == 0)
				free(pBlock);
		}
	}
	//增加内存块的引用计数
	void addRef(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		++pBlock->nRef;
	}
private:
	//初始化内存池映射数组
	void init_szAlloc(int nBegin, int nEnd, MemoryAlloc* pMem)
	{
		for (int n = nBegin; n < nEnd; n++)
		{
			_szAlloc[n] = pMem;
		}
	}
private:
	MemoryAlloctor<64,  100000> _mem64;
	MemoryAlloctor<128, 100000> _mem128;
	MemoryAlloctor<256, 100000> _mem256;
	MemoryAlloctor<512, 100000> _mem512;
	MemoryAlloctor<1024,100000> _mem1024;
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE + 1];
};

#endif // !_MemoryMgr_hpp_
