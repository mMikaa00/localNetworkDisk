#include "client.h"  


//变量  
SOCKET  sClient;                            //套接字  
char dataBuf[MAX_BUFF];                  //数据缓冲区    
BOOL    bConnecting;                        //与服务器的连接状态  
			

/**
初始化全局变量
*/
void InitMember(void)
{
	//  InitializeCriticalSection(&cs);  

	sClient = INVALID_SOCKET;   //套接字  
								//  handleThread = NULL;        //接收数据线程句柄  
	bConnecting = FALSE;        //为连接状态  

								//  //初始化数据缓冲区  
								//  memset(dataBuf, 0, MAX_NUM_BUF);  
}

/**
* 创建非阻塞套接字
*/
BOOL  InitSocket(void)
{
	int         reVal;  //返回值  
	WSADATA     wsData; //WSADATA变量  
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);//初始化Windows Sockets Dll  

												//创建套接字  
	sClient = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sClient)
		return FALSE;

	return TRUE;
}

/**
* 连接服务器
*/
BOOL ConnectServer(void)
{
	int reVal;          //返回值  
	sockaddr_in serAddr;//服务器地址  
						//输入要连接的主机地址  
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8888);
	serAddr.sin_addr.S_un.S_addr = inet_addr("172.18.103.161");

	while (true)
	{
		//连接服务器  
		reVal = connect(sClient, (struct sockaddr*)&serAddr, sizeof(serAddr));
		//处理连接错误  
		if (SOCKET_ERROR == reVal)
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == nErrCode || WSAEINVAL == nErrCode)    //连接还没有完成  
			{
				continue;
			}
			else if (WSAEISCONN == nErrCode)//连接已经完成  
			{
				break;
			}
			else//其它原因，连接失败  
			{
				return FALSE;
			}
		}

		if (reVal == 0)//连接成功  
			break;
	}

	bConnecting = TRUE;

	return TRUE;
}


/**
*  读取数据
*/
bool recvData(SOCKET s, char* buf)
{
	BOOL retVal = TRUE;
	bool bLineEnd = FALSE;      //行结束  
	memset(buf, 0, MAX_BUFF);        //清空接收缓冲区  
	int  nReadLen = 0;          //读入字节数  

	while (!bLineEnd)
	{
		nReadLen = recv(s, buf, MAX_BUFF, 0);
		if (SOCKET_ERROR == nReadLen)
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == nErrCode)   //接受数据缓冲区不可用  
			{
				continue;                       //继续循环  
			}
			else if (WSAENETDOWN == nErrCode || WSAETIMEDOUT == nErrCode || WSAECONNRESET == nErrCode) //客户端关闭了连接  
			{
				retVal = FALSE; //读数据失败  
				break;                          //线程退出  
			}
		}

		if (0 == nReadLen)           //未读取到数据  
		{
			retVal = FALSE;
			break;
		}
		buf[nReadLen] = 0;
		bLineEnd = TRUE;
	}


	return retVal;
}


/**
* @Des:send data
*/
bool sendData(SOCKET s, char* str)
{
	int retVal;//返回值  
	int nlength = strlen(str);
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

		if(nlength==0)
			break;
	}

	return TRUE;        //发送成功  
}

/**
* 客户端退出
*/
void ExitClient(void)
{
	//  DeleteCriticalSection(&cs);  
	//  CloseHandle(handleThread);  
	closesocket(sClient);
	WSACleanup();
}

/**
* 显示连接服务器失败信息
*/
void ShowConnectMsg(BOOL bSuc)
{
	if (bSuc)
	{
		cout << "* Succeed to connect server! *" << endl;
	}
	else
	{
		cout << "* Client has to exit! *" << endl;
	}
}

bool sendUserid() {
	cout << "please input your userid and groupid:" << endl;
	char input[MAX_BUFF];
	cin >> input;
	if (!sendData(sClient, input))
		return false;
	cin >> input;
	if (!sendData(sClient, input))
		return false;
	return true;
}