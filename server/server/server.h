#pragma once
#pragma comment (lib,"ws2_32.lib") 
#include <WinSock2.h>
#include <string>
#include <unordered_set>
#include <iostream>
#include "group.h"
using namespace std;

#define MAX_BUFF 1024


void InitMember();
bool InitSocket(void);
BOOL createAcceptThread(void);
DWORD __stdcall AcceptThreadFunc(void* pParam);
bool recvData(SOCKET s, char* buf);
bool sendData(SOCKET s, char* str);

bool getuserInfo(SOCKET s, pair<string, string> &user);