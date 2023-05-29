#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<Windows.h>
#include<WinSock2.h>
#include<iostream>
#include<stdio.h>
#pragma comment(lib,"ws2_32.lib") //��֧��windowsϵͳ�����뾲̬���ӿ�


//���뿼���ֽڶ���

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_ERROR
};
//��ͷ
struct DataHeader
{
	short dataLength;//���ݳ���
	short cmd; //��������
};
struct Login :public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char PassWord[32];
};

struct LoginResult :public DataHeader
{
	LoginResult()
	{
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};

struct Logout :public DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};

struct LogoutResult :public DataHeader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
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
			//5 �������������������
			Login login;
			strcpy(login.userName,"JHY");
			strcpy(login.PassWord, "666");
			send(_sock, (const char*)&login, sizeof(Login), 0);

			//6 ���ܷ��������ص�����
			LoginResult loginRet;
			recv(_sock, (char*)&loginRet, sizeof(LoginResult), 0);
			printf("LoginResult: %d\n", loginRet.result);
		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			//5 �������������������
			Logout logout;
			strcpy(logout.userName, "JHY");
			send(_sock, (const char*)&logout, sizeof(Logout), 0);

			//6 ���ܷ��������ص�����
			LogoutResult logoutRet;
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