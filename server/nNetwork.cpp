#include "mNetwork.h"

int sendData(SOCKET s, char* str,int n,bool sign)
{
	int writelength;
	if (n == 0)
		writelength = strlen(str) + 1;
	else
		writelength = n;
	int  nWriteLen = 0;          //�����ֽ���  

	while (1)
	{
		nWriteLen = send(s, str, writelength, 0);//һ�η���  
										  //������  
		if (SOCKET_ERROR == nWriteLen)
		{
			int nErrCode = WSAGetLastError();//�������  
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
*  ��ȡ����
*/
int recvData(SOCKET s, char* buf, int n,bool sign)
{
	bool bLineEnd = FALSE;      //�н���   
	int  nReadLen = 0;          //�����ֽ���  
	int readlength = n;

	while (1)
	{
		nReadLen = recv(s, buf, readlength, 0);
		if (SOCKET_ERROR == nReadLen)
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == nErrCode)   //�������ݻ�����������  
			{
				continue;                       //����ѭ��  
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
* ͬ������
*/

void transmitor::synchronizeData() {

	for (auto &k : fileFolder) 										//���ͱ��������ļ�У����Ϣ��Զ��
		sendcheckcode(k.second);
	sendcheckcode(file("end", "",0));


	while (1) {														//��ӦԶ���ļ�����

		recvData(s, dataBuf, 100);
		if (strcmp(dataBuf, "end"))
			sendfile(fileFolder[dataBuf]);
		else
			break;
	}

	//file test("liuwenbo", "woshiyigehaha");

}

/**
* ����ͻ����ύ��������
*/

void transmitor::commitData() {

	unordered_map<string, int> checkcode;						//����Զ��У����Ϣ�����ֵ�
	while (1) {
		auto temp = recvcheckcode();
		if (temp.first != "end")
			checkcode.insert(temp);
		else
			break;
	}

	for (auto i = fileFolder.begin(); i != fileFolder.end();) {			//����check�ֵ��鱾�������Ƿ���Ҫɾ������£�����Ҫ���µ�fileУ����Ϣ���͸�Զ��
		auto curcode = checkcode.find(i->first);
		if (curcode == checkcode.end()) {
			i = fileFolder.erase(i);
			continue;
		}
		else if (i->second.getversion() >= curcode->second)
			checkcode.erase(curcode);
		++i;
	}

	for (auto &k : checkcode) {									//�����ֵ���ʣ����ļ�����Զ�������ļ�����
		char neededfile[100];
		strcpy(neededfile, k.first.c_str());
		sendData(s, neededfile, 100);
		file temp = recvfile();
		fileFolder[temp.getfileId()] = temp;
	}

	sendData(s, "end", 100);

}