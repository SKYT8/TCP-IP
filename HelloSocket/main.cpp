#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
#include<WinSock2.h>
#include<iostream>
#include<stdio.h>
//#pragma comment(lib,"ws2_32.lib") //仅支持windows系统，引入静态链接库

int main()
{
	//启动Windows Socket 2.x 环境
	WORD ver = MAKEWORD(2, 2);//WORD版本号
	WSADATA dat;
	WSAStartup(ver, &dat);//启动win Socket



	//清楚Windows Socket环境
	WSACleanup();//关闭win Socket
	return 0;
}