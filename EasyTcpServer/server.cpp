#include"Alloctor.h"
#include "EasyTcpServer.hpp"
#include<thread>

bool g_bRun = true;
//线程处理函数
void cmdThread()
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
		else
		{
			printf("不支持的命令，请重新输入.\n");
		}
	}
}

class MyServer:public EasyTcpServer
{
public:
	//客户端加入事件->计数 根据业务需求自行实现 目前只有一个线程触发 安全
	virtual void OnNetJoin(ClientSocket* pClient)
	{
		EasyTcpServer::OnNetJoin(pClient);
	}

	//实现客户端离开事件->计数 根据业务需求自行实现 目前cellserver 4 线程出发 不安全
	virtual void OnNetLeave(ClientSocket* pClient)
	{
		EasyTcpServer::OnNetLeave(pClient);
	}

	//实现网络事件->响应数据（回复登录消息） 根据业务需求自行实现 目前cellserver 4 线程触发 不安全
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocket* pClient, DataHeader* header)
	{
		//计数消息包
		EasyTcpServer::OnNetMsg(pCellServer, pClient, header);
		switch (header->cmd)
		{
			case CMD_LOGIN:
			{
				Login* login = (Login*)header;
				//printf("收到客户端Socket: %d 请求:CMD_LOGIN,数据长度: %d ,userName=%s ,PassWord=%s\n", (int)pClient, login->dataLength, login->userName, login->PassWord);
				//忽略判断用户名、密码是否正确
				//LoginResult ret;
				//pClient->SendData(&ret);
				LoginResult* ret = new LoginResult();
				pCellServer->addSendTask(pClient,ret);
			}
			break;
			case CMD_LOGOUT:
			{
				Logout* logout = (Logout*)header;
				//printf("收到客户端Socket: %d 请求:CMD_LOGOUT,数据长度: %d ,userName=%s\n", (int)_sock, (int)cSock, logout->dataLength, logout->userName);
				//忽略判断用户名、密码是否正确
				//LogoutResult ret;
				//pClient->SendData(&ret);
			}
			break;
			default:
			{
				printf("服务端socket: %d 收到未定义消息，数据长度:%d \n",pClient->sockfd(), header->dataLength);
				//DataHeader ret;
				//pClient->SendData(&ret);
			}
			break;
			}
	}

private:

};

int main()
{
	 
	MyServer server;
	//server.InitSocket();
	server.Bind(nullptr, 4567);
	server.Listen(5);
	server.Start(4);

	std::thread t1(cmdThread);
	t1.detach();

	while (g_bRun)
	{
		server.OnRun();
	}
	server.Close();

	printf("已退出..\n");
	getchar();
	return 0;
}