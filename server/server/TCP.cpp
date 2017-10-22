#include "TCP.h"

bool sendData(SOCKET s, char* str,int n)
{
	int retVal;//返回值  
	int nlength;
	if (n == 0)
		nlength = strlen(str);
	else
		nlength = n;
	while (1)
	{
		retVal = send(s, str, nlength, 0);//一次发送  
										  //错误处理  
		if (SOCKET_ERROR == retVal)
		{
			int nErrCode = WSAGetLastError();//错误代码  
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

	return TRUE;        //发送成功  
}

/**
*  读取数据
*/
int recvData(SOCKET s, char* buf, int n)
{
	int retVal = 0;
	bool bLineEnd = FALSE;      //行结束   
	int  nReadLen = 0;          //读入字节数  
	int readlength;
	if (n == 0)
		readlength = MAX_BUFF;
	else
		readlength = n;

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
			else if (WSAENETDOWN == nErrCode || WSAETIMEDOUT == nErrCode || WSAECONNRESET == nErrCode) //客户端关闭了连接  
			{
				retVal = 0; //读数据失败  
				break;                          //线程退出  
			}
		}

		if (0 == nReadLen)           //未读取到数据  
		{
			retVal = 0;
			break;
		}

		if (n == 0) {
			retVal = nReadLen;
			buf[nReadLen] = 0;
			break;
		}
		else
		{
			readlength -= nReadLen;
			if (readlength == 0) {
				buf[nReadLen] = 0;
				retVal = n;
				break;
			}
			buf += nReadLen;
		}
	}
	return retVal;
}