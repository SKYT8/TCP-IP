/*
#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _WIN32
	#include<Windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib") //仅支持windows系统，引入静态链接库
#else
	#include<unistd.h>
	#include<arpa/inet.h>
	#include<string.h>

	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif
#include<iostream>
#include<stdio.h>
#include<thread>



//必须考虑字节对齐

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};
//包头
struct DataHeader
{
	short dataLength;//数据长度
	short cmd; //数据命令
};
struct Login :public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char PassWord[32];
};

struct LoginResult :public DataHeader
{
	LoginResult()
	{
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};

struct Logout :public DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};

struct LogoutResult :public DataHeader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};
struct NewUserJoin :public DataHeader
{
	NewUserJoin()
	{
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};


int processor(SOCKET _cSock)
{
	//缓冲区，管理消息长度
	char szRecv[1024] = {};
	//5 接受服务端返回数据
	int nLen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
	DataHeader* header = (DataHeader*)szRecv;
	if (nLen <= 0)
	{
		printf("与服务器断开连接，任务结束.\n");
		return -1;
	}
	//6 处理请求
	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LoginResult* loginResult = (LoginResult*)szRecv;
		printf("收到服务端返回消息:CMD_LOGINRESULT, 数据长度:%d\n", loginResult->dataLength);
	}
	break;
	case CMD_LOGOUT_RESULT:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LogoutResult* logoutResult = (LogoutResult*)szRecv;
		printf("收到服务端返回消息:CMD_LOGOUTRESULT, 数据长度:%d\n", logoutResult->dataLength);
	}
	break;
	case CMD_NEW_USER_JOIN:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		NewUserJoin* userjoin = (NewUserJoin*)szRecv;
		printf("收到服务端返回消息:CMD_NEW_USER_JOIN,客户端<SOCKET = %d>加入， 数据长度:%d\n", (int)userjoin->sock, userjoin->dataLength);
	}
	break;
	default:
	{
		DataHeader header = { 0,CMD_ERROR };
		send(_cSock, (char*)&header, sizeof(DataHeader), 0);
	}
	break;
	}
	return 0;
}

bool g_bRun = true;
//线程处理函数
void cmdThread(SOCKET sock)
{
	//利用线程进行命令输入
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("退出cmdThread线程.\n");
			break;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login;
			strcpy(login.userName, "JHY");
			strcpy(login.PassWord, "666");
			send(sock, (const char*)&login, sizeof(Login), 0);
		}
		else
		{
			printf("不支持的命令，请重新输入.\n");
		}
	}
}
int main()
{
#ifdef _WIN32
	//启动Windows Socket 2.x 环境
	WORD ver = MAKEWORD(2, 2);//WORD版本号
	WSADATA dat;
	WSAStartup(ver, &dat);//启动win Socket
#endif
	//用四步实现简易的TCP客户端
	//1、建立一个socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock)
	{
		printf("ERROR,建立Socket失败.\n");
	}
	else
	{
		printf("TRUE,建立Socket成功.\n");
	}
	//2、连接服务器 connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = inet_addr("192.168.192.112");
#else
	_sin.sin_addr.s_addr = inet_addr("192.168.191.112");
#endif
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		printf("ERROR,连接Socket失败.\n");
	}
	else
	{
		printf("TRUE,连接Socket成功.\n");
	}
	//启动线程
	std::thread t1(cmdThread, _sock);
	t1.detach();

	while (g_bRun)
	{
		fd_set fdReads;
		FD_ZERO(&fdReads);
		FD_SET(_sock, &fdReads);
		timeval t = { 1, 0 };
		int ret = select(_sock + 1, &fdReads, 0, 0, &t);
		if (ret < 0)
		{
			printf("select任务结束1..\n");
			break;
		}
		if (FD_ISSET(_sock, &fdReads))
		{
			FD_CLR(_sock, &fdReads);
			//接收 处理服务器消息
			if (-1 == processor(_sock))
			{
				printf("select任务结束2..\n");
				break;
			}
		}

		//printf("客户端处于空闲时间，处理其他业务..\n");


	}
	//7、关闭套接字closesocket
#ifdef _WIN32
	closesocket(_sock);
	//清楚Windows Socket环境
	WSACleanup();//关闭win Socket
#else
	close(_sock);
#endif
	getchar();//防止程序一闪而过
	return 0;

}
*/