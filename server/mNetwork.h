#pragma once
#pragma comment (lib,"ws2_32.lib")  
#include <WinSock2.h>
#include <unordered_map>
#include <iostream>
#include "file.h"
#define MAX_BUFF 4096

int recvData(SOCKET s, char* buf, int n, bool sign = false);
int sendData(SOCKET s, char* str, int n=0, bool sign = false);


class transmitor {
public:
	transmitor(SOCKET &sok, unordered_map<string, file> &fF,string p) :
	s(sok),fileFolder(fF),path(p)
	{
		dataBuf = new char[MAX_BUFF];
	}
	~transmitor() {
		delete[] dataBuf;
	}
	/**
	* 发送验证码
	*/
	void sendcheckcode(file &f) {
		strcpy(dataBuf, f.getfileId().c_str());
		sendData(s, dataBuf, 100,true);
		memcpy(dataBuf, &f.getversion(), 4);
		dataBuf[4] = 0;
		sendData(s, dataBuf, 4,true);
	}
	/**
	* 接受验证码
	*/
	pair<string, int> recvcheckcode() {
		pair<string, int> ret;
		recvData(s, dataBuf, 100,true);
		ret.first = dataBuf;
		recvData(s, dataBuf, 4,true);
		ret.second = (*(int*)dataBuf);
		return ret;
	}
	/**
	* 发送文件
	*/
	void sendfile(file &f) {
		strcpy(dataBuf, f.getfileId().c_str());
		sendData(s, dataBuf, 100,true);

		sendFileFromPath(f.getpath().c_str());
		memcpy(dataBuf, &f.getversion(), 4);
		dataBuf[4] = 0;
		sendData(s, dataBuf, 4,true);
	}
	/**
	* 接受文件
	*/
	file recvfile() {
		file temp;
		recvData(s, dataBuf, 100,true);
		temp.setfileId(dataBuf);

		temp.setpath(path+dataBuf);
		recvFiletoPath(temp.getpath().c_str());

		recvData(s, dataBuf, 4,true);
		temp.setversion(*(int*)dataBuf);
		return temp;
	}

	void sendFileFromPath(const char* path) {
		FILE *file = NULL;
		file = fopen(path, "r");
		if (file == NULL)
			exit(1);
		fseek(file, 0, SEEK_SET);
		while (!feof(file))
		{
			int len = fread(dataBuf, 1, MAX_BUFF, file);
			if (sendData(s,dataBuf,len) < 0)
			{
				perror("send file :");
				break;
			}
			if (len < sizeof(dataBuf))
				break;
		}
		recvData(s, dataBuf, 10);
		if (!strcmp(dataBuf, "end"))
			fclose(file);
		else
			cout << "send file error!" << endl;
	}

	void recvFiletoPath(const char* path) {
		FILE *file = NULL;
		file = fopen(path, "w");
		while (1)
		{
			int len = recvData(s,dataBuf,MAX_BUFF);
			if (len <= 0)
				break;
			fwrite(dataBuf, 1, len, file);
			if (len < MAX_BUFF)
				break;
		}
		sendData(s, "end", 4);
		fclose(file);
	}

	void synchronizeData();
	void commitData();

private:
	char *dataBuf;
	string path;
	SOCKET &s;
	unordered_map<string, file> &fileFolder;
};

class socketManager {
public:
	socketManager(SOCKET &s) :myskt(s) {};
	socketManager() {
		closesocket(myskt);
	}
private:
	SOCKET myskt;
};