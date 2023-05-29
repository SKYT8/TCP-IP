#define WIN32_LEAN_AND_MEAN //解决引入外部依赖冲突
#define _WINSOCK_DEPRECATED_NO_WARNINGS//处理inet_nota函数过旧问题

#include<iostream>
#include<Windows.h>
#include<WinSock2.h>
#include<stdio.h>

#pragma comment(lib,"ws2_32.lib") //仅支持windows系统，引入静态链接库
int main()
{
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
	//1、建立一个Socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//2、建立一个Bind 绑定用于接受客户端连接的网络端口
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;//指明协议，IPV4
	_sin.sin_port = htons(4567);//端口host to net unsigned short
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;//  其中限定使用内网ip： inet_addr("127.0.0.1")

	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))
	{
		printf("ERROR，绑定端口失败\n");//
	}
	else
	{
		printf("TRUE,绑定端口成功\n");
	}

	//3、listen  监听网络端口 
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("ERROR,监听端口失败\n");
	}
	else
	{
		printf("TRUE,监听端口成功\n");
	}
	//4、 accept 等待客户端连接
	sockaddr_in clientAddr = {};
	int nAddrLen = sizeof(sockaddr_in);
	SOCKET _cSock = INVALID_SOCKET;


	_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _cSock)
	{
		printf("ERROR,接收到无效客户端SOCKET...\n");
	}
	printf("新客户端加入：socket = %d, IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));

	char _recvBuf[128] = {};
	while (true)
	{
		//5 接受客户端数据
		int nLen = recv(_cSock,_recvBuf,128,0);
		if (nLen <= 0)
		{
			printf("客户端已退出，任务结束.");
			break;
		}
		printf("收到命令：%s \n", _recvBuf);
		//6 处理请求
		if (0 == strcmp(_recvBuf, "getName"))
		{
			//7 send 向客户端发送信息
			char msgBuf[] = "JHY.";
			send(_cSock, msgBuf, strlen(msgBuf) + 1, 0);//+1表示传输结束符
		}
		else if(0 == strcmp(_recvBuf, "getAge"))
		{
			//7 send 向客户端发送信息
			char msgBuf[] = "24.";
			send(_cSock, msgBuf, strlen(msgBuf) + 1, 0);//+1表示传输结束符
		}
		else
		{
			//7 send 向客户端发送信息
			char msgBuf[] = "???.";
			send(_cSock, msgBuf, strlen(msgBuf) + 1, 0);//+1表示传输结束符
		}
		


	}


	//8 closeSocket 关闭套接字
	closesocket(_sock);


	//清除Windows socket环境
	WSACleanup();
	printf("客户端已退出，任务结束。");
	getchar();
	return 0;
}