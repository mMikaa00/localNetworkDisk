#pragma once
#pragma comment (lib,"ws2_32.lib") 
#include <WinSock2.h>
#include <string>
#include <unordered_set>
#include <iostream>
using namespace std;

#define MAX_BUFF 1024


void InitMember();
bool InitSocket(void);
BOOL createAcceptThread(void);
DWORD __stdcall AcceptThreadFunc(void* pParam);