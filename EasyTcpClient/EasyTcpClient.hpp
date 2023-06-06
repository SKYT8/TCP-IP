#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_//确保只会被调用一次
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include<Windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<unistd.h>
	#include<arpa/inet.h>
	#include<string.h> 
	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif // WIN32
#include<stdio.h>
#include "MessageHeader.hpp"
class EasyTcpClient
{
	SOCKET _sock;
public:
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
	}
	virtual ~EasyTcpClient()
	{
		Close();
	}

	//初始化socket
	void InitSocket()
	{
	#ifdef _WIN32
		//启动Windows Socket 2.x 环境
		WORD ver = MAKEWORD(2, 2);//WORD版本号
		WSADATA dat;
		WSAStartup(ver, &dat);//启动win Socket
	#endif
		//检测连接是否有效，无效则关闭
		if (_sock != INVALID_SOCKET)
		{
			printf("关闭旧连接..\n");
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("ERROR,建立Socket失败.\n");
		}
		else
		{
			printf("TRUE,建立Socket成功.\n");
		}
	}

	//连接服务器
	int Connect(const char* ip, unsigned short port)
	{
		if (_sock == INVALID_SOCKET)
		{
			InitSocket();
		}
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
	#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
	#else
		_sin.sin_addr.s_addr = inet_addr(ip);
	#endif
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("<socket=%d>连接服务器<%s:%d>失败.\n",_sock,ip,port);
		}
		else
		{
			printf("<socket=%d>连接服务器<%s:%d>成功.\n",_sock,ip, port);
		}
		return ret;
	}


	//接收网络请求 处理粘包 拆分包 问题
	int RecvData(SOCKET _cSock)
	{
		// 缓冲区，管理消息长度
		char szRecv[4096] = {};
		// 接受服务端返回数据
		int nLen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)szRecv;
		if (nLen <= 0)
		{
			printf("<socket=%d>与服务器断开连接，任务结束.\n",_sock);
			return -1;
		}
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(header);
		return 0;
	}

	// 响应网络请求
	void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
			case CMD_LOGIN_RESULT:
			{
				LoginResult* loginResult = (LoginResult*)header;
				printf("<socket=%d>收到服务端返回消息:CMD_LOGINRESULT, 数据长度:%d\n", _sock, loginResult->dataLength);
			}
			break;
			case CMD_LOGOUT_RESULT:
			{
				LogoutResult* logoutResult = (LogoutResult*)header;
				printf("客户端socket: %d 收到服务端返回消息:CMD_LOGOUTRESULT, 数据长度:%d\n", _sock, logoutResult->dataLength);
			}
			break;
			case CMD_NEW_USER_JOIN:
			{
				NewUserJoin* userjoin = (NewUserJoin*)header;
				printf("客户端socket: %d 收到服务端返回消息:CMD_NEW_USER_JOIN,客户端Socket: %d 加入, 数据长度:%d\n", (int)_sock, (int)userjoin->sock, userjoin->dataLength);
			}
			break;
		}
	}

	//回送网络数据
	int SendData(DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(_sock, (const char*)&header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	//判断是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//查询网络状态 select网络模型
	bool OnRun()
	{
		if (isRun())
		{
			fd_set fdReads;
			FD_ZERO(&fdReads);
			FD_SET(_sock, &fdReads);
			timeval t = { 0, 0 };
			int ret = select(_sock + 1, &fdReads, 0, 0, &t);
			if (ret < 0)
			{
				printf("<socket=%d>select任务结束1..\n", _sock);
				Close();
				return false;
			}
			if (FD_ISSET(_sock, &fdReads))
			{
				FD_CLR(_sock, &fdReads);
				// 接收服务器消息
				if (-1 == RecvData(_sock))
				{
					printf("<socket=%d>select任务结束2..\n", _sock);
					Close();
					return false;
				}
			}
			return true;
		}
		return false;
	}
	
	//关闭Win sock2.x 环境
	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
	#ifdef _WIN32
			closesocket(_sock);
			//清楚Windows Socket环境
			WSACleanup();//关闭win Socket
	#else
			close(_sock);
	#endif
		}
		_sock = INVALID_SOCKET;
	}

private:

};



#endif // !_EasyTcpClient_hpp_
