#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
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
#include"MessageHeader.hpp"

// 缓冲区最小单元
#ifndef RECV_BUFFF_SIZE
#define RECV_BUFF_SIZE 10240 
#endif // !RECV_BUFFF_SIZE


class ClientSocket
{
private:
	// socket fd_set file desc set
	SOCKET _sockfd;
	//消息缓冲区 第二缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	//消息缓冲区的数据尾部指针
	int _lastPos ;
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}
	SOCKET sockfd()
	{
		return _sockfd;
	}
	char* msgBuf()
	{
		return _szMsgBuf;
	}
	int getLsatPos()
	{
		return _lastPos;
	}
	void setLastPos(int pos)
	{
		_lastPos = pos;
	}
};

class EasyTcpServer
{
private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
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
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);//host to net unsigned short

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
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("监听socket: %d 失败...\n",_sock);
		}
		else {
			printf("监听socket: %d 成功...\n", _sock);
		}
		return ret;
	}

	//接收数据 处理粘包 拆分包
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
			NewUserJoin userJoin;
			SendDataToAll(&userJoin);
			_clients.push_back(new ClientSocket(cSock));
			printf("服务端socket: %d 中,有新客户端加入: Socket = %d,IP = %s \n", (int)_sock, (int)cSock, inet_ntoa(clientAddr.sin_addr));
		}
		return cSock;
	}

	//关闭Socket
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
	#ifdef _WIN32
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			closesocket(_sock);
			WSACleanup();
	#else
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				close(_clients[n]->sockfd());
				delete _clients[n];
			}
			close(_sock);
	#endif
			_clients.clear();
		}
	}
	
	//响应客户端消息
	bool OnRun()
	{
		if (isRun())
		{
			//伯克利套接字 BSD socket
			fd_set fdRead;//描述符集合，用于管理socket
			fd_set fdWrite;
			fd_set fdExp;
			//置空集合
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);
			//将当前描述符（socket）加入集合
			FD_SET(_sock, &fdRead);
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExp);
			SOCKET maxSock = _sock;
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				FD_SET(_clients[n]->sockfd(), &fdRead);
				if (maxSock < _clients[n]->sockfd())
				{
					maxSock = _clients[n]->sockfd();
				}
			}
			// int select(int nfds, fd set *readfds, fd set *writefds,fd set* exceptfds, struct timeval* timeout);
			// nfds是一个整数值，是指fd set集合中所有的描述符(socket)的范围值，不是数量值; 即最大值1 在Windows中可以写0
			timeval t = { 1,0 };
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
			if (ret < 0)
			{
				printf("select任务结束...\n");
				Close();
				return false;
			}
			//接收客户端消息
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				Accept();
			}
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				if (FD_ISSET(_clients[n]->sockfd(), &fdRead))
				{
					if (-1 == RecvData(_clients[n]))
					{
						auto iter = _clients.begin() + n;//std::vector<SOCKET>::iterator
						if (iter != _clients.end())
						{
							delete _clients[n];
							_clients.erase(iter);
						}
					}
				}
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

	//缓冲区
	char _szRecv[RECV_BUFF_SIZE] = {};
	// 消息缓冲区 第二缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10] = {};
	//消息缓冲区尾部指针
	int _lastPos = 0;
	//接收数据 处理粘包 拆分包
	int RecvData(ClientSocket* pClient)
	{
		// 接收客户端消息
		int nLen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			printf("客户端Socket: %d 已退出,任务结束...\n", pClient->sockfd());
			return -1;
		}
		//将接收消息缓冲区 接收到的消息 拷贝到 消息缓冲区
		memcpy(pClient->msgBuf() + pClient->getLsatPos(), _szRecv, nLen);
		//消息缓冲区的数据尾部位置
		pClient->setLastPos(pClient->getLsatPos() + nLen); 
		//判断消息缓冲区的数据长度是否大于消息头 获取当前消息体长度
		while (pClient->getLsatPos() >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			if (pClient->getLsatPos() >= header->dataLength)
			{
				//剩余未处理消息长度
				int nSize = pClient->getLsatPos() - header->dataLength;
				//处理网络消息
				OnNetMsg(pClient->sockfd(), header);
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
	
	//处理客户端消息
	virtual void OnNetMsg(SOCKET cSock, DataHeader* header)
	{
		switch (header->cmd)
		{
			case CMD_LOGIN:
			{
			
				Login* login = (Login*)header;
				//printf("收到客户端Socket: %d 请求:CMD_LOGIN,数据长度: %d ,userName=%s ,PassWord=%s\n", (int)cSock, login->dataLength, login->userName, login->PassWord);
				//忽略判断用户名、密码是否正确
				LoginResult ret;
				SendData(cSock, &ret);
			}
			break;
			case CMD_LOGOUT:
			{
				Logout* logout = (Logout*)header;
				//printf("收到客户端Socket: %d 请求:CMD_LOGOUT,数据长度: %d ,userName=%s\n", (int)_sock, (int)cSock, logout->dataLength, logout->userName);
				//忽略判断用户名、密码是否正确
				LogoutResult ret;
				SendData(cSock, &ret);
			}
			break;
			default:
			{
				printf("服务端socket: %d 收到未定义消息，数据长度:%d \n", _sock, header->dataLength);
				DataHeader ret;
				SendData(cSock, &ret);
			}
			break;
		}
	}

	//回送至指定socket
	int SendData(SOCKET cSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(cSock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	//回送至所有Socket
	void SendDataToAll(DataHeader* header)
	{
		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			SendData(_clients[n]->sockfd(), header);
		}
	}

};

#endif // !_EasyTcpServer_hpp_
