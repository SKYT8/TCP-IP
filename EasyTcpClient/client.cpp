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

//客户端数量
const int cCount = 10000;
//发送线程数量
const int tCount = 4;
//客户端数组
EasyTcpClient* client[cCount];
void sendThread(int id)
{ 
	//4个线程 1 - 4
	int c = cCount / tCount;
	int begin = (id - 1) * c; 
	int end = id * c; 
	for (int n = begin; n < end; n++)
	{
		client[n] = new EasyTcpClient();
	}
	for (int n = begin; n < end; n++)
	{
		client[n]->Connect("127.0.0.1", 4567);
		printf("线程:%d,Connect= %d..\n", id,n);
	}

	std::chrono::milliseconds t(5000);
	std::this_thread::sleep_for(t);

	//一次发一个数据
	//Login login;
	//strcpy(login.userName, "JHY");
	//strcpy(login.PassWord, "666");
	//一次发送十个数据, 同时需要修改SendData方法
	Login login[10];
	for (int n = 0; n < 10; n++)
	{
		strcpy(login[n].userName, "JHY");
		strcpy(login[n].PassWord, "666");
	}

	const int nLen = sizeof(login);
	while (g_bRun)
	{ 
		for (int n = begin; n <  end; n++)
		{
			//client[n]->OnRun();
			client[n]->SendData(login,nLen);
		}
	}

	//关闭连接
	for (int n = begin; n < end; n++)
	{
		delete client[n];
		client[n]->Close();
	}
}

int main() 
{
	//启动发送线程线程
	for (int n = 0; n < tCount; n++)
	{
		std::thread t1(sendThread,n+1);
		t1.detach();
	}
	//启动UI线程
	std::thread t1(cmdThread);
	t1.detach();

	while (g_bRun)
	{
		Sleep(100);
	}
	printf("已退出..\n");
	//getchar();//防止程序一闪而过
	return 0;
} 
