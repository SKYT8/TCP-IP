#include "EasyTcpClient.hpp"
#include<thread>

//线程处理函数
void cmdThread(EasyTcpClient* client)
{
	//利用线程进行命令输入
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			client->Close();
			printf("退出cmdThread线程.\n");
			break;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login;
			strcpy(login.userName, "JHY");
			strcpy(login.PassWord, "666");
			client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout logout;
			strcpy(logout.userName, "JHY");
			client->SendData(&logout);
		}
		else
		{
			printf("不支持的命令，请重新输入.\n");
		}
	}
}
int main() 
{
	//连接windows下服务器
	EasyTcpClient client1;
	client1.Connect("127.0.0.1", 4567);
	
	//连接Linux下服务器
	//EasyTcpClient client2;
	//client2.Connect("127.0.0.1", 4567);

	//连接windows下服务器
	//EasyTcpClient client3;
	//client3.Connect("127.0.0.1", 4567);
	
	//启动UI线程
	std::thread t1(cmdThread, &client1);
	t1.detach();

	//std::thread t2(cmdThread, &client2);
	//t2.detach();

	//std::thread t3(cmdThread, &client3);
	//t3.detach();



	while (client1.isRun())
	{
		client1.OnRun();
	}

	//关闭连接
	client1.Close();

	printf("已退出..\n");
	getchar();//防止程序一闪而过
	return 0;
}
