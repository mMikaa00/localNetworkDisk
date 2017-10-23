#pragma once
#pragma comment (lib,"ws2_32.lib")  
#include <WinSock2.h>
#include <unordered_map>
#include <iostream>
#include "fileManager.h"
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
		memcpy(dataBuf, &f.getversion(), 4);
		sendData(s, dataBuf, 4, true);
		memcpy(dataBuf, &f.getsize(), 4);
		sendData(s, dataBuf, 4, true);
		sendFileFromPath((path+f.getfileId()).c_str(),f.getsize());	
	}
	/**
	* 接受文件
	*/
	file recvfile() {
		file temp;
		recvData(s, dataBuf, 100,true);
		temp.setfileId(dataBuf);
		recvData(s, dataBuf, 4, true);
		temp.setversion(*(int*)dataBuf);
		recvData(s, dataBuf, 4, true);
		temp.setsize(*(int*)dataBuf);

		temp.setpath(path+temp.getfileId());
		recvFiletoPath(temp.getpath().c_str(),temp.getsize());
		return temp;
	}

	void sendFileFromPath(const char* path,int size) {
		FILE *file = NULL;
		file = fopen(path, "rb");
		if (file == NULL)
			exit(1);
		fseek(file, 0, SEEK_SET);
		while (size>0)
		{
			int len = fread(dataBuf, 1, MAX_BUFF, file);
			if (sendData(s,dataBuf,len) < 0)
			{
				perror("send file :");
				break;
			}
			size -= len;
			//if (len < MAX_BUFF)
				//break;
		}
		/*recvData(s, dataBuf, 10);
		if (!strcmp(dataBuf, "end"))
			fclose(file);
		else
			cout << "send file error!" << endl;*/
	}

	void recvFiletoPath(const char* path,int size) {
		FILE *file = NULL;
		file = fopen(path, "wb");
		while (size>0)
		{
			int len = recvData(s,dataBuf,MAX_BUFF);
			if (len <= 0)
				break;
			fwrite(dataBuf, 1, len, file);
			size -= len;
			//if (len < MAX_BUFF)
				//break;
		}
		//sendData(s, "end", 4);
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