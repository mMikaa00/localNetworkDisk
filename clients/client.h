#pragma once
#pragma comment (lib,"ws2_32.lib") 
#include <WinSock2.h>
#include "mNetwork.h"
#include "fileManager.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <unordered_map>
using namespace std;

#ifndef MAX_BUFF
#define MAX_BUFF 4096
#endif


DWORD WINAPI ThreadFunc(LPVOID lpParam);
class client {
public:
	client();
	~client();
	friend DWORD WINAPI ThreadFunc(LPVOID lpParam);
	BOOL InitSocket(void);
	BOOL ConnectServer(void);
	bool InitStdinThread();

	void resetSocket();
	void ExitClient(void);

	void getUserInfo();
	bool checkUserid();

	void getinfo();

	static string GetLocalIP();
	void print();
	void SynchronizeData(transmitor&);
	void CommitData(transmitor&);
	
private: 
	SOCKET  sClient, sStdinfd;                            //套接字    
	BOOL    bConnecting;                        //与服务器的连接状态 
	pair<string, string> curuser;
	unordered_map<string, file> fileFolder;	//存储本地文件空间
	HANDLE hMapFile;
	HANDLE hThread;
	int *port;
	string path = "D:\\test\\client\\";
};
