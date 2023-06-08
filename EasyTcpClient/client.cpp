#include "EasyTcpClient.hpp"
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
	const int cCount = FD_SETSIZE -1;
	EasyTcpClient* client[cCount];
	for (int n = 0; n < cCount; n++)
	{
		client[n] = new EasyTcpClient();
		client[n]->Connect("127.0.0.1", 4567);
	}
	
	//启动UI线程
	std::thread t1(cmdThread);
	t1.detach();

	Login login;
	strcpy(login.userName, "JHY");
	strcpy(login.PassWord, "666");

	while (g_bRun)
	{
		for (int n = 0; n < cCount; n++)
		{
			
			client[n]->OnRun();
			client[n]->SendData(&login);
		}
	}

	//关闭连接
	for (int n = 0; n < cCount; n++)
	{
		delete client[n];
		client[n]->Close();
	}

	printf("已退出..\n");
	getchar();//防止程序一闪而过
	return 0;
}
