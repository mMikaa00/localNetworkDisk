#pragma once
#include <string>
#include <iostream>
#include "mNetwork.h"
#include "group.h"
using namespace std;

class csManager {
public:
	csManager(CRITICAL_SECTION &css):cs(css){
		EnterCriticalSection(&cs);
	}
	~csManager() {
		LeaveCriticalSection(&cs);
	}
private:
	CRITICAL_SECTION &cs;
};


DWORD __stdcall AcceptThreadFunc(void* pParam);


class server {
public:
	server();
	~server();
	bool InitSocket(void);
	BOOL createAcceptThread(void);
	friend DWORD __stdcall AcceptThreadFunc(void* pParam);
	static void getuserInfo(SOCKET &s, pair<string, string> &user);
private:
	HANDLE sAcceptThread;
	HANDLE sAcceptSocket;
	CRITICAL_SECTION cs;
	SOCKET  sAccept;                            //接受客户端连接的套接字 
	SOCKET sServer;								//服务器监听套接字
	bool sConfig;
	string path="d:\\";
	userManager sUsers;
};
