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
	SOCKET  sClient, sStdinfd;                            //�׽���    
	BOOL    bConnecting;                        //�������������״̬ 
	pair<string, string> curuser;
	unordered_map<string, file> fileFolder;	//�洢�����ļ��ռ�
	HANDLE hMapFile;
	HANDLE hThread;
	int *port;
	string path = "D:\\test\\client\\";
};
