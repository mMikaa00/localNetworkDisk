#pragma once
#include <WinSock2.h>
#define MAX_BUFF 1024

int recvData(SOCKET &s, char* buf);
bool sendData(SOCKET &s, char* str,int n=0);
