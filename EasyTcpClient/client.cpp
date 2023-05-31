#define WIN32_LEAN_AND_MEAN
//#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _WIN32
#include<Windows.h>
#include<WinSock2.h>
#else
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>

#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif
#include<iostream>
#include<stdio.h>
#include<thread>
#pragma comment(lib,"ws2_32.lib") //��֧��windowsϵͳ�����뾲̬���ӿ�


//���뿼���ֽڶ���

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
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
struct NewUserJoin :public DataHeader
{
	NewUserJoin()
	{
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};


int processor(SOCKET _cSock)
{
	//��������������Ϣ����
	char szRecv[1024] = {};
	//5 ���ܷ���˷�������
	int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
	DataHeader* header = (DataHeader*)szRecv;
	if (nLen <= 0)
	{
		printf("��������Ͽ����ӣ��������.\n");
		return -1;
	}
	//6 ��������
	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LoginResult* loginResult = (LoginResult*)szRecv;
		printf("�յ�����˷�����Ϣ:CMD_LOGINRESULT, ���ݳ���:%d\n", loginResult->dataLength);
	}
	break;
	case CMD_LOGOUT_RESULT:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LogoutResult* logoutResult = (LogoutResult*)szRecv;
		printf("�յ�����˷�����Ϣ:CMD_LOGOUTRESULT, ���ݳ���:%d\n", logoutResult->dataLength);
	}
	break;
	case CMD_NEW_USER_JOIN:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		NewUserJoin* userjoin = (NewUserJoin*)szRecv;
		printf("�յ�����˷�����Ϣ:CMD_NEW_USER_JOIN,�ͻ���<SOCKET = %d>���룬 ���ݳ���:%d\n", (int)userjoin->sock, userjoin->dataLength);
	}
	break;
	default:
	{
		DataHeader header = { 0,CMD_ERROR };
		send(_cSock, (char*)&header, sizeof(DataHeader), 0);
	}
	break;
	}
	return 0;
}

bool g_bRun = true;
//�̴߳�����
void cmdThread(SOCKET sock)
{
	//�����߳̽�����������
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("�˳�cmdThread�߳�.\n");
			break;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login;
			strcpy(login.userName, "JHY");
			strcpy(login.PassWord, "666");
			send(sock, (const char*)&login, sizeof(Login), 0);
		}
		else
		{
			printf("��֧�ֵ��������������.\n");
		}
	}
}
int main()
{
#ifdef _WIN32
	//����Windows Socket 2.x ����
	WORD ver = MAKEWORD(2, 2);//WORD�汾��
	WSADATA dat;
	WSAStartup(ver, &dat);//����win Socket
#endif
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
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
	_sin.sin_addr.s_addr = inet_addr("172.23.80.117");
#endif
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		printf("ERROR,����Socketʧ��.\n");
	}
	else
	{
		printf("TRUE,����Socket�ɹ�.\n");
	}
	//�����߳�
	std::thread t1(cmdThread, _sock);
	t1.detach();

	while (g_bRun)
	{
		fd_set fdReads;
		FD_ZERO(&fdReads);
		FD_SET(_sock, &fdReads);
		timeval t = { 1, 0 };
		int ret = select(_sock + 1, &fdReads, 0, 0, &t);
		if (ret < 0)
		{
			printf("select�������1..\n");
			break;
		}
		if (FD_ISSET(_sock, &fdReads))
		{
			FD_CLR(_sock, &fdReads);
			//���� �����������Ϣ
			if (-1 == processor(_sock))
			{
				printf("select�������2..\n");
				break;
			}
		}

		//printf("�ͻ��˴��ڿ���ʱ�䣬��������ҵ��..\n");


	}
	//7���ر��׽���closesocket
#ifdef _WIN32
	closesocket(_sock);
	//���Windows Socket����
	WSACleanup();//�ر�win Socket
#else
	close(_sock);
#endif
	getchar();//��ֹ����һ������
	return 0;

}
