#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
#include<WinSock2.h>
#include<iostream>
#include<stdio.h>
//#pragma comment(lib,"ws2_32.lib") //��֧��windowsϵͳ�����뾲̬���ӿ�

int main()
{
	//����Windows Socket 2.x ����
	WORD ver = MAKEWORD(2, 2);//WORD�汾��
	WSADATA dat;
	WSAStartup(ver, &dat);//����win Socket



	//���Windows Socket����
	WSACleanup();//�ر�win Socket
	return 0;
}