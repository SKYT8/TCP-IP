#include "EasyTcpClient.hpp"
#include "CELLTimestamp.hpp"
#include<thread>
#include<atomic>
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
const int cCount = 100;
//发送线程数量
const int tCount = 4;
//客户端数组
EasyTcpClient* client[cCount];
std::atomic_int sendCount =0;
std::atomic_int readyCount = 0;

void sendThread(int id)
{ 
	printf("thread:%d,start... \n", id);
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
	}
	printf("Thread:%d,Connect begin: %d,end: %d ..\n", id, begin,end);

	readyCount++;
	while (readyCount < tCount)
	{//等待其他綫程准备好发送数据
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}

	//一次发一个数据
	//Login login;
	//strcpy(login.userName, "JHY");
	//strcpy(login.PassWord, "666");
	//一次发送十个数据, 同时需要修改SendData方法
	Login login[1];
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
			if (SOCKET_ERROR != client[n]->SendData(login, nLen))
			{
				sendCount++;
			}
			client[n]->OnRun();
		}
	}

	//关闭连接 
	for (int n = begin; n < end; n++)
	{
		client[n]->Close();
		delete client[n];
	}
	printf("thread:%d,exit...\n", id);
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

	CELLTimestamp tTime;

	while (g_bRun)
	{
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			printf("thread:%d,clientCount:%d,time:%lf,sendCount:%d...\n",tCount, cCount, t,  (int)(sendCount/t));
			tTime.update();
			sendCount = 0;
		}
		Sleep(1);
	}
	printf("已退出..\n");
	//getchar();//防止程序一闪而过
	return 0;
} 
