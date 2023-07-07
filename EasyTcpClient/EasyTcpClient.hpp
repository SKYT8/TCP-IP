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
private:
	SOCKET _sock;
	bool _isConnect;
public:
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
		_isConnect = false;
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
			//printf("TRUE,建立Socket成功.\n");
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
		//printf("socket:%d 正在连接服务器：%s:%d...\n", _sock, ip,port);
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			printf("<socket=%d>连接服务器<%s:%d>失败.\n",_sock,ip,port);
		}
		else
		{
			_isConnect = true;
			//printf("<socket=%d>连接服务器<%s:%d>成功.\n",_sock,ip, port);
		}
		return ret;
	}

	// 缓冲区最小单元 
	#ifndef RECV_BUFFF_SIZE
	#define RECV_BUFF_SIZE 10240 * 5
	#endif // !RECV_BUFFF_SIZE

	// 接收缓冲区  
	//char _szRecv[RECV_BUFF_SIZE] = {};
	 // 消息缓冲区 第二缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 5] = {};
	//消息缓冲区尾部指针
	int _lastPos = 0;

	//接收网络请求 处理粘包 拆分包 问题
	int RecvData(SOCKET cSock)
	{
		// 接收服务端返回数据
		char* szRecv = _szMsgBuf + _lastPos;//将数据直接拷贝到消息缓冲区  不先拷贝到接收缓冲区  减少一个拷贝时间
		int nLen = (int)recv(cSock, szRecv, (RECV_BUFF_SIZE * 5) - _lastPos, 0);
		if (nLen <= 0)
		{
			printf("<socket=%d>与服务器断开连接，任务结束.\n", _sock);
			return -1;
		}
		//将接收消息缓冲区 接收到的消息 拷贝到 消息缓冲区
		//memcpy(_szMsgBuf + _lastPos, _szRecv, nLen);
		//消息缓冲区的数据尾部位置
		_lastPos += nLen;
		//判断消息缓冲区的数据长度是否大于消息头 获取当前消息体长度
		while (_lastPos >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)_szMsgBuf;
			int a = sizeof(header);
			if (_lastPos >= header->dataLength)
			{
				//剩余未处理消息长度
				int nSize = _lastPos - header->dataLength;
				//处理网络消息
				OnNetMsg(header);
				//将消息缓冲区中的未处理数据前移
				memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
				//位置指针前移 
				_lastPos = nSize;
			}
			else
			{
				//消息缓冲区数据未达到一条完整消息，继续从接收缓冲区读取
				break;
			}
		}
		return 0;
	}

	// 处理网络请求
	void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
			case CMD_LOGIN_RESULT:
			{
				LoginResult* loginResult = (LoginResult*)header;
				//printf("<socket=%d>收到服务端返回消息:CMD_LOGINRESULT, 数据长度:%d\n", _sock, loginResult->dataLength);
			}
			break;
			case CMD_LOGOUT_RESULT:
			{
				LogoutResult* logoutResult = (LogoutResult*)header;
				//printf("客户端socket: %d 收到服务端返回消息:CMD_LOGOUTRESULT, 数据长度:%d\n", _sock, logoutResult->dataLength);
			}
			break;
			case CMD_NEW_USER_JOIN:
			{
				NewUserJoin* userjoin = (NewUserJoin*)header;
				//printf("客户端socket: %d 收到服务端返回消息:CMD_NEW_USER_JOIN,客户端Socket: %d 加入, 数据长度:%d\n", (int)_sock, (int)userjoin->sock, userjoin->dataLength);
			}
			break;
			case CMD_ERROR:
			{
				printf("客户端socket: %d 收到服务端返回消息:CMD_ERROR,数据长度:%d\n", (int)_sock, header->dataLength);
			}
			break;
			default:
			{
				printf("客户端socket: %d 收到未定义消息，数据长度:%d \n",_sock,header->dataLength);
			}
		}
	}

	//回送网络数据
	int SendData(DataHeader* header, int nLen)
	{
		int ret = SOCKET_ERROR;
		if (isRun() && header)
		{
			ret = send(_sock, (const char*)&header, nLen, 0);
			if (ret == SOCKET_ERROR)
			{
				Close();
			}
		}
		return ret;
	}

	//判断是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET && _isConnect;
	}

	//查询网络状态 接收服务器消息 select网络模型
	int _nCount = 0;
	bool OnRun()
	{
		if (isRun())
		{
			fd_set fdReads;
			FD_ZERO(&fdReads);
			FD_SET(_sock, &fdReads);
			timeval t = { 0, 0 };
			int ret = select(_sock + 1, &fdReads, 0, 0, &t);
			//printf("select ret=%d, count = %d\n",ret,_nCuont++)
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
		_isConnect = false;
	}

};



#endif // !_EasyTcpClient_hpp_
