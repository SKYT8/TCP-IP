#include"Alloctor.h"
#include<stdlib.h>
#include"MemoryMgr.hpp"

void* operator new(size_t nsize)
{
	return MemoryMgr::Instance().allocMem(nsize); 
}
void operator delete(void *p)
{
	MemoryMgr::Instance().freeMem(p);
}

void* operator new[] (size_t nsize)
{
	return MemoryMgr::Instance().allocMem(nsize);
}
void operator delete[](void *p)
{
	MemoryMgr::Instance().freeMem(p);
}

void* mem_alloc(size_t nsize)
{
	return malloc(nsize);;
}

void mem_free(void* p)
{
	free(p);
}