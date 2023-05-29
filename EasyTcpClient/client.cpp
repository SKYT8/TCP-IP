#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#include<iostream>
#include<stdio.h>
#pragma comment(lib,"ws2_32.lib") //仅支持windows系统，引入静态链接库

int main()
{
	//启动Windows Socket 2.x 环境
	WORD ver = MAKEWORD(2, 2);//WORD版本号
	WSADATA dat;
	WSAStartup(ver, &dat);//启动win Socket
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
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		printf("ERROR,连接Socket失败.\n");
	}
	else
	{
		printf("TRUE,连接Socket成功.\n");
	}

	while (true)
	{
		//3 输入请求命令
		char cmdBuf[128] = {};
		scanf("%s",cmdBuf);
		//4 处理请求命令
		if (0 == strcmp(cmdBuf, "exit"))
		{
			printf("收到退出命令，任务结束.");
			break;
		}
		else
		{
			//5 向服务器发送请求命令
			send(_sock, cmdBuf, strlen(cmdBuf) + 1, 0);
		}

		//6 接收服务器信息 recv
		char recvBuf[256] = {};
		int nlen = recv(_sock, recvBuf, 256, 0);
		if (nlen > 0)
		{
			printf("接收到数据：%s \n", recvBuf);
		}
	}


	//7、关闭套接字closesocket
	closesocket(_sock);
	//清楚Windows Socket环境
	WSACleanup();//关闭win Socket
	getchar();//防止程序一闪而过
	return 0;
}