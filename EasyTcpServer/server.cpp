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
int main()
{
	 
	EasyTcpServer server;
	//server.InitSocket();
	server.Bind(nullptr, 4567);
	server.Listen(5);

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