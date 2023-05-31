#define WIN32_LEAN_AND_MEAN //��������ⲿ������ͻ
#define _WINSOCK_DEPRECATED_NO_WARNINGS//����inet_nota������������

#include<iostream>
#include<Windows.h>
#include<WinSock2.h>
#include<stdio.h>
#include<vector>

#pragma comment(lib,"ws2_32.lib") //��֧��windowsϵͳ�����뾲̬���ӿ�
using namespace std;
//���뿼���ֽڶ���

//ö��ָ��
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

struct Login:public DataHeader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char PassWord[32];
};

struct LoginResult:public DataHeader
{
	LoginResult()
	{
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};

struct Logout:public DataHeader
{
	Logout()
	{
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};

struct LogoutResult:public DataHeader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};

struct NewUserJoin:public DataHeader
{
	NewUserJoin() 
	{
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};

//����һ��ȫ�ֵ�SOCKET�������飬�����洢����ͻ���
vector<SOCKET> g_clients;

//���գ������������ܿͻ���������Ϣ������������
int processor(SOCKET _cSock)
{
	//��������������Ϣ����
	char szRecv[1024] = {};
	//5 ���ܿͻ�������
	int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
	DataHeader* header = (DataHeader*)szRecv;
	if (nLen <= 0)
	{
		printf("�ͻ������˳����������.");
		return -1;
	}
	//6 ��������
	switch (header->cmd)
	{
		case CMD_LOGIN:
		{
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);//��ͷ�Ѿ����գ��� ����ʣ�µ���Ϣ��
			Login* login = (Login*)szRecv;
			printf("�յ��ͻ���<Socket = %d>����:CMD_LOGIN, ���ݳ���:%d, userName:%s, PassWord:%s\n", _cSock, login->dataLength, login->userName, login->PassWord);
			//�����ж��û����������Ƿ���ȷ
			LoginResult ret;
			send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
		}
		break;
		case CMD_LOGOUT:
		{  
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			Logout* logout = (Logout*)szRecv;
			printf("�յ��ͻ���<Socket = %d>����:CMD_LOGOUT, ���ݳ���:%d, userName:%s\n", _cSock, logout->dataLength, logout->userName);
			//�����ж��û����������Ƿ���ȷ
			LogoutResult ret;
			send(_cSock, (char*)&ret, sizeof(LogoutResult), 0);

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
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
	//1������һ��Socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//2������һ��Bind �����ڽ��ܿͻ������ӵ�����˿�
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;//ָ��Э�飬IPV4
	_sin.sin_port = htons(4567);//�˿�host to net unsigned short
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;//  �������޶�ʹ������ip�� inet_addr("127.0.0.1")

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

	while (true)
	{
		//������ �׽���socket
		fd_set fdRead; // ����һ�����������ϣ������������������׽���
		fd_set fdWrite;
		fd_set fdExp;

		//������
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);
		//�����������뼯��
		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);		
		FD_SET(_sock, &fdExp);

		for (int n = (int)g_clients.size() - 1; n >= 0; n--)
		{
			//���ͻ����׽�����ӵ�����������
			FD_SET(g_clients[n], &fdRead);
		}

		// int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
		// nfds��һ������ֵ����ָfd_set���������е���������socket���ķ�Χֵ����������ֵ�������ֵ+1 ��Windows�п���д 0�� 
		timeval t = {1,0};//�ṹ���к��������ݣ���Ӧ��ͺ��룻��ֵ��ΪNULL��ʾ������ֱ�ȵ����׽���׼����������ֵ��Ϊ0��ʾ�������أ�Ҳ���Խ�ֵ��Ϊ����ֵ

		//����Ϊselectģ��
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &t); 
		if (ret < 0)
		{
			printf("select�������.\n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);

			//4�� accept �ȴ��ͻ�������
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(sockaddr_in);
			SOCKET _cSock = INVALID_SOCKET;

			_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
			if (INVALID_SOCKET == _cSock)
			{
				printf("ERROR,���յ���Ч�ͻ���SOCKET...\n");
			}

			//֪ͨ�����ͻ������¿ͻ��˼���
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				NewUserJoin userjoin;
				userjoin.sock = (int)g_clients[n];
				send(g_clients[n], (const char*)&userjoin, sizeof(NewUserJoin), 0);
			}
			g_clients.push_back(_cSock);//���¼������ӵĿͻ��� ����������������
			printf("�¿ͻ��˼��룺socket = %d, IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
		}
		//6 7 ��������λ���׽����������е� �ͻ����׽��ֵ������շ����ݣ�
		for (size_t n = 0; n < fdRead.fd_count; n++)
		{
			if (-1 == processor(fdRead.fd_array[n])) //���Ѿ�׼���õ��׽������봦������д���  ���жϵ�ǰ�׽����Ƿ���������
			{
				auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[n]);//��������Ӳ��������׽���   ��g_clients�в��������׽����ҵ�������
				if (iter != g_clients.end())
				{
					g_clients.erase(iter);
				}
			}
		}
		//printf("���������ڿ���ʱ�䣬��������ҵ��..\n");
	}

	//����ر��׽���
	for (size_t n = g_clients.size() - 1; n >= 0; n--)
	{
		closesocket(g_clients[n]);
	}

	//8 closeSocket �ر��׽���
	closesocket(_sock);


	//���Windows socket����
	WSACleanup();
	printf("�ͻ������˳����������.\n");
	getchar();
	return 0;
}



