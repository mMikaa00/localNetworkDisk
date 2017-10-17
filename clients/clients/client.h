#pragma once
#pragma comment (lib,"ws2_32.lib") 
#include <WinSock2.h>
#include <string>
#include <unordered_set>
#include <iostream>
#include "groupManager.h"
using namespace std;

#define MAX_BUFF 1024


void InitMember(void);
BOOL  InitSocket(void);
BOOL ConnectServer(void);
void recvAndSend(void);
bool RecvLine(SOCKET s, char* buf);
bool recvData(SOCKET s, char* buf);
bool sendData(SOCKET s, char* str);
void ExitClient(void);
void ShowConnectMsg(BOOL bSuc);

bool sendUserid();