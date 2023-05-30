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
		printf("�յ�����˷�����Ϣ:CMD_NEW_USER_JOIN,�ͻ���<SOCKET = %d>���룬 ���ݳ���:%d\n", userjoin->sock, userjoin->dataLength);
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
		fd_set fdReads;
		FD_ZERO(&fdReads);
		FD_SET(_sock, &fdReads);
		timeval t = { 1,0 };
		int ret = select(_sock, &fdReads, 0, 0, &t);
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
	
		printf("�ͻ��˴��ڿ���ʱ�䣬��������ҵ��..\n");
		//��������������͵�¼����Ϣ
		Login login;
		strcpy(login.userName,"JHY");
		strcpy(login.PassWord, "666");
		send(_sock, (const char*)&login, sizeof(Login),0);
		Sleep(1000);

	}
		//7���ر��׽���closesocket
		closesocket(_sock);
		//���Windows Socket����
		WSACleanup();//�ر�win Socket
		getchar();//��ֹ����һ������
		return 0;

}