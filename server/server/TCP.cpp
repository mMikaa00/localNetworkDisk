#include "TCP.h"

bool sendData(SOCKET &s, char* str,int n)
{
	int retVal;//����ֵ  
	int nlength;
	if (n == 0)
		nlength = strlen(str);
	else
		nlength = n;
	while (1)
	{
		retVal = send(s, str, nlength, 0);//һ�η���  
										  //������  
		if (SOCKET_ERROR == retVal)
		{
			int nErrCode = WSAGetLastError();//�������  
			if (WSAEWOULDBLOCK == nErrCode)
			{
				continue;
			}
			else if (WSAENETDOWN == nErrCode || WSAETIMEDOUT == nErrCode || WSAECONNRESET == nErrCode)
			{
				return FALSE;
			}
		}
		nlength -= retVal;
		str += retVal;

		if (nlength == 0)
			break;
	}

	return TRUE;        //���ͳɹ�  
}

/**
*  ��ȡ����
*/
int recvData(SOCKET &s, char* buf)
{
	int retVal = 0;
	bool bLineEnd = FALSE;      //�н���  
	int  nReadLen = 0;          //�����ֽ���  

	while (!bLineEnd)
	{
		nReadLen = recv(s, buf, MAX_BUFF, 0);
		if (SOCKET_ERROR == nReadLen)
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == nErrCode)   //�������ݻ�����������  
			{
				continue;                       //����ѭ��  
			}
			else if (WSAENETDOWN == nErrCode || WSAETIMEDOUT == nErrCode || WSAECONNRESET == nErrCode) //�ͻ��˹ر�������  
			{
				retVal = 0; //������ʧ��  
				break;                          //�߳��˳�  
			}
		}

		if (0 == nReadLen)           //δ��ȡ������  
		{
			retVal = 0;
			break;
		}
		retVal = nReadLen;
		buf[nReadLen] = 0;
		bLineEnd = TRUE;
	}

	return retVal;
}