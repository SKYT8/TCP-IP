#define WIN32_LEAN_AND_MEAN //��������ⲿ������ͻ
#define _WINSOCK_DEPRECATED_NO_WARNINGS//����inet_nota������������

#include<iostream>
#include<Windows.h>
#include<WinSock2.h>
#include<stdio.h>

#pragma comment(lib,"ws2_32.lib") //��֧��windowsϵͳ�����뾲̬���ӿ�
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

	char _recvBuf[128] = {};
	while (true)
	{
		//5 ���ܿͻ�������
		int nLen = recv(_cSock,_recvBuf,128,0);
		if (nLen <= 0)
		{
			printf("�ͻ������˳����������.");
			break;
		}
		printf("�յ����%s \n", _recvBuf);
		//6 ��������
		if (0 == strcmp(_recvBuf, "getName"))
		{
			//7 send ��ͻ��˷�����Ϣ
			char msgBuf[] = "JHY.";
			send(_cSock, msgBuf, strlen(msgBuf) + 1, 0);//+1��ʾ���������
		}
		else if(0 == strcmp(_recvBuf, "getAge"))
		{
			//7 send ��ͻ��˷�����Ϣ
			char msgBuf[] = "24.";
			send(_cSock, msgBuf, strlen(msgBuf) + 1, 0);//+1��ʾ���������
		}
		else
		{
			//7 send ��ͻ��˷�����Ϣ
			char msgBuf[] = "???.";
			send(_cSock, msgBuf, strlen(msgBuf) + 1, 0);//+1��ʾ���������
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