#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#include<iostream>
#include<stdio.h>
#pragma comment(lib,"ws2_32.lib") //��֧��windowsϵͳ�����뾲̬���ӿ�


enum CMD
{
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_ERROR
};
//��ͷ
struct DataHeader
{
	short dataLength;//���ݳ���
	short cmd; //��������
};
struct Login
{
	char userName[32];
	char PassWord[32];
};

struct LoginResult
{
	int result;
};

struct Logout
{
	char userName[32];
};
struct LogoutResult
{
	int result;
};

int main()
{
	//����Windows Socket 2.x ����
	WORD ver = MAKEWORD(2, 2);//WORD�汾��
	WSADATA dat;
	WSAStartup(ver, &dat);//����win Socket
	//���Ĳ�ʵ�ּ��׵�TCP�ͻ���
	//1������һ��socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock)
	{
		printf("ERROR,����Socketʧ��.\n");
	}
	else
	{
		printf("TRUE,����Socket�ɹ�.\n");
	}
	//2�����ӷ����� connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		printf("ERROR,����Socketʧ��.\n");
	}
	else
	{
		printf("TRUE,����Socket�ɹ�.\n");
	}

	while (true)
	{
		//3 ������������
		char cmdBuf[128] = {};
		scanf("%s", cmdBuf);
		//4 ������������
		if (0 == strcmp(cmdBuf, "exit"))
		{
			printf("�յ��˳�����������.\n");
			break;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login = { "JHY","666" };
			DataHeader hd = { sizeof(Login), CMD_LOGIN };
			//5 �������������������
			send(_sock, (const char*)&hd, sizeof(DataHeader), 0);
			send(_sock, (const char*)&login, sizeof(Login), 0);
			//6 ���ܷ��������ص�����
			DataHeader retHeader = {};
			LoginResult loginRet = {};
			recv(_sock, (char*)&retHeader, sizeof(DataHeader), 0);
			recv(_sock, (char*)&loginRet, sizeof(LoginResult), 0);
			printf("LoginResult: %d\n", loginRet.result);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			//5 �������������������
			Logout logout = { "JHY" };
			DataHeader hd = {sizeof(Logout), CMD_LOGOUT };
			send(_sock, (const char*)&hd, sizeof(DataHeader), 0);
			send(_sock, (const char*)&logout, sizeof(Logout), 0);
			//6 ���ܷ��������ص�����
			DataHeader retHeader = {};
			LogoutResult logoutRet = {};
			recv(_sock, (char*)&retHeader, sizeof(DataHeader), 0);
			recv(_sock, (char*)&logoutRet, sizeof(LogoutResult), 0);
			printf("LogoutResult: %d \n", logoutRet.result);
		}
		else
		{
			printf("��֧�ֵ��������������.\n");
		}

	}
	//7���ر��׽���closesocket
	closesocket(_sock);
	//���Windows Socket����
	WSACleanup();//�ر�win Socket
	getchar();//��ֹ����һ������
	return 0;
}