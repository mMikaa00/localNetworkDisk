#pragma once
#pragma comment (lib,"ws2_32.lib") 
#include <WinSock2.h>
#include "TCP.h"
#include "file.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <unordered_map>
using namespace std;

#define MAX_BUFF 1024


void InitMember(void);
BOOL InitSocket(void);
string GetLocalIP();
BOOL ConnectServer(void);
bool InitStdinThread();
DWORD WINAPI ThreadFunc(LPVOID lpParam);
void ExitClient(void);
void ShowConnectMsg(BOOL bSuc);

bool checkUserid();
void getinfo();

void synchronizeData();
void commitData();

void sendcheckcode(SOCKET &s, file &f);
void sendfile(SOCKET &s, file &f);
pair<string, int> recvcheckcode(SOCKET &s);
file recvfile(SOCKET &s);

void print();
