#pragma once
#pragma comment (lib,"ws2_32.lib")  
#include <WinSock2.h>
#include <string>
#include <unordered_set>
#include <iostream>
#include "group.h"
#include "TCP.h"
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

void InitMember();
bool InitSocket(void);
BOOL createAcceptThread(void);
DWORD __stdcall AcceptThreadFunc(void* pParam);

bool getuserInfo(SOCKET &s, pair<string, string> &user);

void exitThread(SOCKET*);

void synchronizeData(SOCKET &s, pair<string, string> &user, unordered_map<string, file> &fileFolder);
void commitData(SOCKET &S, pair<string, string> &user, unordered_map<string, file> &fileFolder);
void sendcheckcode(SOCKET &s, file &f);
void sendfile(SOCKET &s, file &f);
pair<string, int> recvcheckcode(SOCKET &s);
file recvfile(SOCKET &s);