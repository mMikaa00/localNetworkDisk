#include "mNetwork.h"

int sendData(SOCKET s, char* str,int n,bool sign)
{
	int writelength;
	if (n == 0)
		writelength = strlen(str) + 1;
	else
		writelength = n;
	int  nWriteLen = 0;          //发送字节数  

	while (1)
	{
		nWriteLen = send(s, str, writelength, 0);//一次发送  
										  //错误处理  
		if (SOCKET_ERROR == nWriteLen)
		{
			int nErrCode = WSAGetLastError();//错误代码  
			if (WSAEWOULDBLOCK == nErrCode)
			{
				continue;
			}
			else
				return nWriteLen;
		}
		if (sign == false)
			return nWriteLen;
		else
		{
			writelength -= nWriteLen;
			if (writelength == 0)
				return n;
			str += nWriteLen;
		}
	}
}

/**
*  读取数据
*/
int recvData(SOCKET s, char* buf, int n,bool sign)
{
	bool bLineEnd = FALSE;      //行结束   
	int  nReadLen = 0;          //读入字节数  
	int readlength = n;

	while (1)
	{
		nReadLen = recv(s, buf, readlength, 0);
		if (SOCKET_ERROR == nReadLen)
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == nErrCode)   //接受数据缓冲区不可用  
			{
				continue;                       //继续循环  
			}
			else
				throw exception();
		}

		if (sign == false) 
			return nReadLen;
		else
		{
			readlength -= nReadLen;
			if (readlength == 0) 
				return n;
			buf += nReadLen;
		}
	}
}

/**
* 同步数据
*/

void transmitor::synchronizeData() {

	for (auto &k : fileFolder) 										//发送本端所有文件校验信息给远端
		sendcheckcode(k.second);
	sendcheckcode(file("end", "",0));


	while (1) {														//回应远端文件请求

		recvData(s, dataBuf, 100);
		if (strcmp(dataBuf, "end"))
			sendfile(fileFolder[dataBuf]);
		else
			break;
	}

	//file test("liuwenbo", "woshiyigehaha");

}

/**
* 处理客户端提交数据请求
*/

void transmitor::commitData() {

	unordered_map<string, int> checkcode;						//接受远端校验信息存入字典
	while (1) {
		auto temp = recvcheckcode();
		if (temp.first != "end")
			checkcode.insert(temp);
		else
			break;
	}

	for (auto i = fileFolder.begin(); i != fileFolder.end();) {			//根据check字典检查本地数据是否需要删除或更新，将需要更新的file校验信息发送给远端
		auto curcode = checkcode.find(i->first);
		if (curcode == checkcode.end()) {
			i = fileFolder.erase(i);
			continue;
		}
		else if (i->second.getversion() >= curcode->second)
			checkcode.erase(curcode);
		++i;
	}

	for (auto &k : checkcode) {									//根据字典中剩余的文件名向远端申请文件数据
		char neededfile[100];
		strcpy(neededfile, k.first.c_str());
		sendData(s, neededfile, 100);
		file temp = recvfile();
		fileFolder[temp.getfileId()] = temp;
	}

	sendData(s, "end", 100);

}