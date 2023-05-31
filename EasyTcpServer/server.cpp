#define WIN32_LEAN_AND_MEAN //解决引入外部依赖冲突
#define _WINSOCK_DEPRECATED_NO_WARNINGS//处理inet_nota函数过旧问题

#include<iostream>
#include<Windows.h>
#include<WinSock2.h>
#include<stdio.h>
#include<vector>

#pragma comment(lib,"ws2_32.lib") //仅支持windows系统，引入静态链接库
using namespace std;
//必须考虑字节对齐

//枚举指令
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

struct Login:public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char PassWord[32];
};

struct LoginResult:public DataHeader
{
	LoginResult()
	{
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};

struct Logout:public DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};

struct LogoutResult:public DataHeader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};

struct NewUserJoin:public DataHeader
{
	NewUserJoin() 
	{
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};

//创建一个全局的SOCKET类型数组，用来存储多个客户端
vector<SOCKET> g_clients;

//接收，处理函数：接受客户端请求消息，并处理请求
int processor(SOCKET _cSock)
{
	//缓冲区，管理消息长度
	char szRecv[1024] = {};
	//5 接受客户端数据
	int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
	DataHeader* header = (DataHeader*)szRecv;
	if (nLen <= 0)
	{
		printf("客户端已退出，任务结束.");
		return -1;
	}
	//6 处理请求
	switch (header->cmd)
	{
		case CMD_LOGIN:
		{
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);//包头已经接收，仅 接收剩下的消息体
			Login* login = (Login*)szRecv;
			printf("收到客户端<Socket = %d>请求:CMD_LOGIN, 数据长度:%d, userName:%s, PassWord:%s\n", _cSock, login->dataLength, login->userName, login->PassWord);
			//忽略判断用户名和密码是否正确
			LoginResult ret;
			send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
		}
		break;
		case CMD_LOGOUT:
		{  
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			Logout* logout = (Logout*)szRecv;
			printf("收到客户端<Socket = %d>请求:CMD_LOGOUT, 数据长度:%d, userName:%s\n", _cSock, logout->dataLength, logout->userName);
			//忽略判断用户名和密码是否正确
			LogoutResult ret;
			send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);

		}
		break;
		default:
		{
			DataHeader header = { 0,CMD_ERROR };
			send(_cSock, (char*)&header, sizeof(DataHeader), 0);
		}
		break;
	}

}

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
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;//  本例中限定使用内网ip： inet_addr("127.0.0.1")

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

	while (true)
	{
		//伯克利 套接字socket
		fd_set fdRead; // 申明一个描述符集合，用描述符来管理多个套接字
		fd_set fdWrite;
		fd_set fdExp;

		//清理集合
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);
		//将描述符加入集合
		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);		
		FD_SET(_sock, &fdExp);

		for (int n = (int)g_clients.size() - 1; n >= 0; n--)
		{
			//将客户端套接字添加到描述符集合
			FD_SET(g_clients[n], &fdRead);
		}

		// int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
		// nfds是一个整数值，是指fd_set集合中所有的描述符（socket）的范围值，不是数量值；即最大值+1 在Windows中可以写 0； 
		timeval t = {1,0};//结构体中含两个数据，对应秒和毫秒；将值设为NULL表示阻塞，直等到有套接字准备就绪；将值设为0表示立即返回；也可以将值设为其他值

		//设置为select模型
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &t); 
		if (ret < 0)
		{
			printf("select任务结束.\n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);

			//4、 accept 等待客户端连接
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(sockaddr_in);
			SOCKET _cSock = INVALID_SOCKET;

			_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
			if (INVALID_SOCKET == _cSock)
			{
				printf("ERROR,接收到无效客户端SOCKET...\n");
			}

			//通知其他客户端有新客户端加入
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				NewUserJoin userjoin;
				userjoin.sock = (int)g_clients[n];
				send(g_clients[n], (const char*)&userjoin, sizeof(NewUserJoin), 0);
			}
			g_clients.push_back(_cSock);//将新加入连接的客户端 加入描述符集合中
			printf("新客户端加入：socket = %d, IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
		}
		//6 7 遍历处理位于套接字描述符中的 客户端套接字的请求（收发数据）
		for (size_t n = 0; n < fdRead.fd_count; n++)
		{
			if (-1 == processor(fdRead.fd_array[n])) //将已经准备好的套接字送入处理程序中处理  并判断当前套接字是否链接正常
			{
				auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[n]);//如果有连接不正常的套接字   在g_clients中不正常的套接字找到并擦除
				if (iter != g_clients.end())
				{
					g_clients.erase(iter);
				}
			}
		}
		//printf("服务器处于空闲时间，处理其他业务..\n");
	}

	//处理关闭套接字
	for (size_t n = g_clients.size() - 1; n >= 0; n--)
	{
		closesocket(g_clients[n]);
	}

	//8 closeSocket 关闭套接字
	closesocket(_sock);


	//清除Windows socket环境
	WSACleanup();
	printf("客户端已退出，任务结束.\n");
	getchar();
	return 0;
}



