#pragma once
#include <string>
#include <unordered_set>
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

void InitMember();
bool InitSocket(void);
BOOL createAcceptThread(void);
DWORD __stdcall AcceptThreadFunc(void* pParam);

void getuserInfo(SOCKET &s, pair<string, string> &user);

void exitThread(SOCKET*);
