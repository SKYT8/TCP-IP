#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
	#define FD_SETSIZE 2506 // 重定义Windows下的套接字描述符的最大值
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<unistd.h> //uni std
	#include<arpa/inet.h>
	#include<string.h>

	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include<stdio.h>
#include<vector>
#include<map>
#include<thread>
#include<mutex>
#include<functional>
#include<atomic>
#include"MessageHeader.hpp"
#include"CELLTimestamp.hpp"
#include"CELLTask.hpp"

// 缓冲区最小单元
#ifndef RECV_BUFFF_SIZE
#define RECV_BUFF_SIZE 10240 * 5
#define SEND_BUFF_SIZE RECV_BUFF_SIZE
#endif // !RECV_BUFFF_SIZE

//宏定义服务器线程数量
#define _CellServer_THREAD_COUNT 4

//客户端数据类型
class ClientSocket
{
private:
	// socket fd_set file desc set 
	SOCKET _sockfd;
	//消息缓冲区 第二缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE];
	//消息缓冲区的数据尾部指针
	int _lastPos ;

	//发送缓冲区 第二缓冲区
	char _szSendBuf[SEND_BUFF_SIZE];
	//发送缓冲区的数据尾部指针
	int _lastSendPos;
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;

		memset(_szMsgBuf, 0, RECV_BUFF_SIZE);
		_lastPos = 0;

		memset(_szSendBuf, 0, SEND_BUFF_SIZE);
		_lastSendPos = 0;
	}
	
	SOCKET sockfd()
	{
		return _sockfd;
	}
	
	char* msgBuf()
	{
		//返回一个指向_szMsgBuf数组的指针
		return _szMsgBuf;
	}
	
	int getLastPos()
	{
		return _lastPos;
	}
	
	void setLastPos(int pos)
	{
		_lastPos = pos;
	}

	//发送消息给客户端
	int SendData(DataHeader* header)
	{
		int ret = SOCKET_ERROR;
		//要发送数据长度
		int nSendLen = header->dataLength;
		//要发送的数据
		const char* pSendData = (const char*)header;

		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
			{
				//计算SEND_BUFF中可拷贝长度
				int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
				//拷贝数据
				memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
				//计算剩余数据位置
				pSendData += nCopyLen;
				//计算剩余数据长度
				nSendLen -= nSendLen;
				//发送该数据
				ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
				//数据尾部指针置零
				_lastSendPos = 0;
				if (ret == SOCKET_ERROR)
				{
					return ret;
				}
			}
			else
			{
				//将待发送数据拷贝纸缓冲区尾部
				memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
				//更改缓冲区尾部指针
				_lastSendPos += nSendLen;
				break;
			}
			return ret;
		}
	}
		
};

class CellServer;
//网络事件接口
class INetEvent
{
public:
	//客户端加入事件
	virtual void OnNetJoin(ClientSocket* pClient) = 0; //纯虚函数
	//客户端离开事件
	virtual void OnNetLeave(ClientSocket * pClient) = 0; //纯虚函数
	//客户端消息事件
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocket * pClient, DataHeader* header) = 0;//纯虚函数
	//recv事件
	virtual void OnNetRecv(ClientSocket* pClient) = 0;
private:
};

//网络消息发送任务
class CellSendMsg2Client:public CellTask
{
private:
	ClientSocket* _pClient;
	DataHeader* _pHeader;

public:
	CellSendMsg2Client(ClientSocket* pClient, DataHeader * header)
	{ 
		_pClient = pClient;
		_pHeader = header;
	}

	void doTask()
	{
		_pClient->SendData(_pHeader);
		delete _pHeader;
	}



};

//网络数据接收服务
class CellServer
{
private:
	SOCKET _sock;
	//正式客户队列
	std::map<SOCKET,ClientSocket*> _clients;
	//客户缓冲区队列
	std::vector<ClientSocket*> _clientsBuff;
	//缓冲队列的锁
	std::mutex _mutex;
	std::thread* _pThread;
	//网络事件对象
	INetEvent* _pNetEvent;
	//
	CellTaskServer _TaskServer;

public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pThread = nullptr;
		_pNetEvent = nullptr;
	}
	
	virtual ~CellServer()
	{
		delete _pThread;
		Close();
		_sock = INVALID_SOCKET;
	}
	
	void setEventObj(INetEvent* event)
	{
		_pNetEvent = event;
	}

	//关闭Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (auto iter : _clients)
			{
				closesocket(iter.second->sockfd());
				delete iter.second;
			}

			//关闭套接字
			closesocket(_sock);
#else
			for (auto iter : _clients)
			{
				close(iter.second->sockfd());
				delete iter.second;
			}
			close(_sock);
#endif
			_clients.clear();
		}
	}

	//判断是否运行中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//处理客户端消息
	//备份客户端socket fs_set
	fd_set _fdRead_bak;
	//客户端列表是否有变化
	bool _clients_change;
	SOCKET _maxSock;
	void OnRun()
	{
		_clients_change = true;
		while (isRun())
		{
			//将缓存队列中的客户端加入正式队列
			if (!_clientsBuff.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
				}
				_clientsBuff.clear();
				_clients_change = true;
			}
			//如果没有需要处理的客户端则跳过循环
			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);//跨平台的休眠一毫秒（C++里的chrono）
				std::this_thread::sleep_for(t); 
				continue;
			}
				
			//伯克利套接字 BSD socket
			fd_set fdRead;//描述符集合，用于管理socket
			//置空集合
			FD_ZERO(&fdRead);
			
			//将客户端套接字加入文件描述符集合中
			if (_clients_change)
			{
				_clients_change = false;
				_maxSock = _clients.begin()->second->sockfd();
				for (auto iter : _clients)
				{
					FD_SET(iter.second->sockfd(), &fdRead);
					//更新最大的套接字文件描述符
					if (_maxSock < iter.second->sockfd())
					{
						_maxSock = iter.second->sockfd();
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else
			{
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			timeval t = { 0,0 };//只用于收数据，没有IO操作
			//int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, &t);
			if (ret < 0)
			{
				printf("select任务结束...\n");
				Close();
				return ;
			}
			else if (ret == 0)
			{
				continue;
			}

#ifndef _WIN32
			for (int n = 0; n < fdRead.fd_count; n++)
			{
				auto iter = _clients.find(fdRead.fd_array[n]);
				if (iter != _clients.end())
				{
					if (-1 == RecvData(iter->second))//执行接收数据操作  并判断是否出现错误
					{
						if (_pNetEvent)
						{
							_pNetEvent->OnNetLeave(iter->second);
						}
						_clients_change = true;
						_clients.erase(iter->first);
					}
				}
				else
				{
					pritnf("Error.iter != _clients.end()...\n");
				}
			}
#else
			std::vector<ClientSocket*> temp;
			for (auto iter : _clients)
			{
				if (FD_ISSET(iter.second->sockfd(), &fdRead))//检查描述符集合中的套接字相应位是否设置成读取状态（设置成读取状态则接收消息）
				{
					if (-1 == RecvData(iter.second))//执行接收数据操作  并判断是否出现错误
					{
						if (_pNetEvent)
						{
							_pNetEvent->OnNetLeave(iter.second);
						}
						_clients_change = false;
						temp.push_back(iter.second);
					}
				}
			}
			for (auto pClient : temp)
			{
				_clients.erase(pClient->sockfd());//在_clients数组中删除套接字位置
				delete pClient;//释放出现错误这个套接字
			}
#endif // !_WIN32

		}
	}

	//接收数据 处理粘包 拆分包 的具体实现
	int RecvData(ClientSocket* pClient)
	{
		// 接收客户端消息 将消息缓存于缓冲区  并返回收到数据大小
		char* szRecv = pClient->msgBuf() + pClient->getLastPos();
		//int nLen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0);
		int nLen = (int)recv(pClient->sockfd(), szRecv, (RECV_BUFF_SIZE * 5) - pClient->getLastPos(), 0);
		//Recv 计数
		_pNetEvent->OnNetRecv(pClient); 
		if (nLen <= 0)
		{
			//printf("客户端Socket: %d 已退出,任务结束...\n", pClient->sockfd());
			return -1;
		}
		//消息缓冲区（第二缓冲区） 将缓冲区的消息 拷贝到 第二缓冲区
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), szRecv, nLen);
		//消息缓冲区的数据尾部位置
		pClient->setLastPos(pClient->getLastPos() + nLen);
		//判断消息缓冲区的数据长度是否大于消息头 获取当前消息体长度
		while (pClient->getLastPos() >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			if (pClient->getLastPos() >= header->dataLength)
			{
				//剩余未处理消息长度
				int nSize = pClient->getLastPos() - header->dataLength;
				//处理网络消息
				OnNetMsg(pClient, header);
				//将消息缓冲区中的未处理数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				//位置指针前移 
				pClient->setLastPos(nSize);
			}
			else
			{
				//消息缓冲区数据未达到一条完整消息，继续从接收缓冲区读取
				break;
			}
		}
		return 0;
	}

	//响应客户端消息
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)
	{
		_pNetEvent->OnNetMsg(this,pClient,header);
	}

	//将客户端加入缓冲队列
	void addClient(ClientSocket* pClient)
	{
		std::lock_guard<std::mutex> lock(_mutex);//自解锁
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	//开启线程
	void Start()
	{
		//将成员函数转换成函数对象
		_pThread = new std::thread(std::mem_fn(&CellServer::OnRun),this);
		_TaskServer.Start();
	}
	
	//计算客户端数量
	size_t getClientCount()
	{
		//返回 正式队列和缓冲队列中 全部客户端数量
		return _clients.size() + _clientsBuff.size();
	}

	//
	void addSendTask(ClientSocket* pClient, DataHeader* header)
	{
		CellSendMsg2Client* task = new CellSendMsg2Client(pClient, header);
		_TaskServer.addTask(task);
	}

};

class EasyTcpServer:public INetEvent
{
private:
	SOCKET _sock;
	//服务器消息处理线程
	std::vector<CellServer*> _cellServer;
	//每秒消息计时
	CELLTimestamp _tTime;
protected:
	//客户端计数
	std::atomic_int _clientCount;
	//接收消息包计数
	std::atomic_int _msgCount;
	//Socket recv 计数
	std::atomic_int _recvCount;
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
		_clientCount = 0;
		_msgCount = 0;
		_recvCount = 0;
	}
	
	virtual ~EasyTcpServer()
	{
		Close();
	}
	
	//初始化Socket
	SOCKET InitSocket()
	{
	#ifdef _WIN32
		//初始化Windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
	#endif
		if (INVALID_SOCKET != _sock)
		{
			printf("<socket=%d>关闭旧连接...\n", (int)_sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("建立Socket失败...\n");
		}
		else {
			printf("建立socket: %d 成功...\n", (int)_sock);
		}
		return _sock;
	}

	//绑定IP及端口
	int Bind(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}
		sockaddr_in _sin = {};//创建一个用于表示IPV4套接字地址结构的结构体
		_sin.sin_family = AF_INET;//指定地址的家族
		_sin.sin_port = htons(port);//指定端口号

//指定IP
#ifdef _WIN32
		if (ip){
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret)
		{
			printf("绑定端口Port: %d 失败...\n", port);
		}
		else {
			printf("绑定端口Port: %d 成功...\n", port);
		}
		return ret;
	}

	//监听端口
	int Listen(int n)
	{
		int ret = listen(_sock, n);//n表示等待队列的长度 返回值为0表示成功，其他则失败
		if (SOCKET_ERROR == ret)
		{
			printf("监听socket: %d 失败...\n",_sock);
		}
		else {
			printf("监听socket: %d 成功...\n", _sock);
		}
		return ret;
	}

	//接收客户端socket连接
	SOCKET Accept()
	{
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSock = INVALID_SOCKET;
	#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
	#endif
		if (INVALID_SOCKET == cSock)
		{
			printf("服务端Socket: %d ,接收到无效的客户端Socket: %d ...\n", (int)_sock, (int)cSock);
		}
		else
		{
			addClientToCellServer(new ClientSocket(cSock));
			//inet_ntoa(clientAddr.sin_addr) // 获取IP地址
		}
		return cSock;
	}
	
	// 将客户端放入缓冲队列  将新加入的客户端放入消息服务线程的缓冲队列  每个消息服务线程都有一个缓冲队列
	void addClientToCellServer(ClientSocket* pClient)
	{
		//查找客户端数量最少的CellServer线程消息处理线程
		auto pMinCellServer = _cellServer[0];
		for (auto pCellServer : _cellServer)
		{
			if (pMinCellServer->getClientCount() > pCellServer->getClientCount())
			{
				//查找客户端数量最少的那个线程
				pMinCellServer = pCellServer;
			}
			pMinCellServer->addClient(pClient);
			OnNetJoin(pClient);
		}
	}

	//开启服务器程序
	void Start(int nCellServer)
	{
		//创建指定数量的服务器的消息处理线程 并开启服务器的消息处理线程
		for (int n = 0; n < nCellServer; n++)
		{
			//创建消息处理线程
			auto ser = new CellServer(_sock);
			_cellServer.push_back(ser);
			//注册网络事件接收对象
			ser->setEventObj(this);
			//启动服务线程
			ser->Start();
		}
	}

	//关闭Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
	#ifdef _WIN32
			//关闭套接字
			closesocket(_sock);
			//清除Windows socket环境
			WSACleanup();
	#else
			//关闭套接字
			close(_sock);
	#endif
		}
	}
	
	//处理客户端消息
	bool OnRun()
	{
		if (isRun())
		{
			time4msg();
			//伯克利套接字 BSD socket
			fd_set fdRead;//描述符集合，用于管理socket
			//fd_set fdWrite;
			//fd_set fdExp;
			//置空集合
			FD_ZERO(&fdRead);
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExp);
			//将当前描述符（socket）加入集合
			FD_SET(_sock, &fdRead);
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExp);

			// int select(int nfds, fd set *readfds, fd set *writefds,fd set* exceptfds, struct timeval* timeout);
			// nfds是一个整数值，是指fd set集合中所有的描述符(socket)的范围值，不是数量值; 即最大值1 在Windows中可以写0
			timeval t = { 0,10 };
			//int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
			int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
			if (ret < 0)
			{
				printf("Accept select任务结束...\n");
				Close();
				return false;
			}
			//接收客户端连接请求
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				Accept();
				return true;
			}
			return true;
		}
		return false;

	}
	
	//判断是否运行中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	
	//客户端消息计时
	void time4msg()
	{
		auto t1 = _tTime.getElapsedSecond();
		if ( t1 >= 1.0)
		{
			printf("thredCount:%d,time:%lf,socket:%d,clientCount:%d,msgCount:%d, recvCount:%d...\n",_cellServer.size(), t1, _sock, (int)_clientCount, (int)(_msgCount / t1), (int)(_recvCount / t1));
			_msgCount = 0;
			_recvCount = 0;
			_tTime.update();
		}
	}

	//客户端加入事件->计数 根据业务需求自行实现 目前只有一个线程触发 安全
	virtual void OnNetJoin(ClientSocket* pClient)
	{
		_clientCount++;
	}

	//实现客户端离开事件->计数 根据业务需求自行实现 目前cellserver 4 线程出发 不安全
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		_clientCount--;
	}

	//实现网络事件->计数 根据业务需求自行实现 目前cellserver 4 线程触发 不安全
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocket* pClient, DataHeader* header)
	{
		_msgCount++;
	}

	//实现服务端收成功建立Socket计数 
	virtual void OnNetRecv(ClientSocket* pClient)
	{
		//计数接受了多少次数据
		_recvCount++;
		//printf("_recvCount:%d...\n", (int)_recvCount);
	}
};

#endif // !_EasyTcpServer_hpp_
