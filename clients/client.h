#pragma once
#pragma comment (lib,"ws2_32.lib") 
#include <WinSock2.h>
#include "mNetwork.h"
#include "file.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <unordered_map>
using namespace std;

#ifndef MAX_BUFF
#define MAX_BUFF 4096
#endif

void InitMember(void);
BOOL InitSocket(void);
BOOL ConnectServer(void);
bool InitStdinThread();
DWORD WINAPI ThreadFunc(LPVOID lpParam);

void resetSocket();
void ExitClient(void);

void getUserInfo();
bool checkUserid();
void getinfo();

void print();
string GetLocalIP();


void SynchronizeData(transmitor&);
void CommitData(transmitor&);