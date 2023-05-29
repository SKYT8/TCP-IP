#define WIN32_LEAN_AND_MEAN //��������ⲿ������ͻ
#define _WINSOCK_DEPRECATED_NO_WARNINGS//����inet_nota������������

#include<iostream>
#include<Windows.h>
#include<WinSock2.h>
#include<stdio.h>

#pragma comment(lib,"ws2_32.lib") //��֧��windowsϵͳ�����뾲̬���ӿ�

//���뿼���ֽڶ���

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
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
	//1������һ��Socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//2������һ��Bind �����ڽ��ܿͻ������ӵ�����˿�
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;//ָ��Э�飬IPV4
	_sin.sin_port = htons(4567);//�˿�host to net unsigned short
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;//  �����޶�ʹ������ip�� inet_addr("127.0.0.1")

	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))
	{
		printf("ERROR���󶨶˿�ʧ��\n");//
	}
	else
	{
		printf("TRUE,�󶨶˿ڳɹ�\n");
	}

	//3��listen  ��������˿� 
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("ERROR,�����˿�ʧ��\n");
	}
	else
	{
		printf("TRUE,�����˿ڳɹ�\n");
	}
	//4�� accept �ȴ��ͻ�������
	sockaddr_in clientAddr = {};
	int nAddrLen = sizeof(sockaddr_in);
	SOCKET _cSock = INVALID_SOCKET;


	_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _cSock)
	{
		printf("ERROR,���յ���Ч�ͻ���SOCKET...\n");
	}
	printf("�¿ͻ��˼��룺socket = %d, IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));


	while (true)
	{
		DataHeader header = {};
		//5 ���ܿͻ�������
		int nLen = recv(_cSock,(char *)&header, sizeof(DataHeader), 0);
		if (nLen <= 0)
		{
			printf("�ͻ������˳����������.");
			break;
		}
		//printf("�յ����%s \n", _recvBuf);
		printf("�յ���� %d ���ݳ��ȣ� %d\n", header.cmd, header.dataLength);
		//6 ��������
		switch (header.cmd)
		{
			case CMD_LOGIN:
			{
				Login login = {};
				recv(_cSock, (char*)&login, sizeof(Login), 0);
				//�����ж��û����������Ƿ���ȷ
				LoginResult ret = {0};
				send(_cSock, (char*)&header, sizeof(DataHeader), 0);
				send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
			}
			break;
			case CMD_LOGOUT:
			{
				Logout loginout = {};
				recv(_cSock, (char*)&loginout, sizeof(Logout), 0);
				//�����ж��û����������Ƿ���ȷ
				LogoutResult ret = { 1 };
				send(_cSock, (char*)&header, sizeof(DataHeader), 0);
				send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);
			}
			break;
			default:
				header.cmd = CMD_ERROR;
				header.dataLength = 0;
				send(_cSock, (char*)&header, sizeof(DataHeader), 0);
			break;
		}
	}


	//8 closeSocket �ر��׽���
	closesocket(_sock);


	//���Windows socket����
	WSACleanup();
	printf("�ͻ������˳������������");
	getchar();
	return 0;
}