#include "client.h"  


//����  
SOCKET  sClient;                            //�׽���  
char dataBuf[MAX_BUFF];                  //���ݻ�����    
BOOL    bConnecting;                        //�������������״̬  
			

/**
��ʼ��ȫ�ֱ���
*/
void InitMember(void)
{
	//  InitializeCriticalSection(&cs);  

	sClient = INVALID_SOCKET;   //�׽���  
								//  handleThread = NULL;        //���������߳̾��  
	bConnecting = FALSE;        //Ϊ����״̬  

								//  //��ʼ�����ݻ�����  
								//  memset(dataBuf, 0, MAX_NUM_BUF);  
}

/**
* �����������׽���
*/
BOOL  InitSocket(void)
{
	int         reVal;  //����ֵ  
	WSADATA     wsData; //WSADATA����  
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);//��ʼ��Windows Sockets Dll  

												//�����׽���  
	sClient = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sClient)
		return FALSE;

	return TRUE;
}

/**
* ���ӷ�����
*/
BOOL ConnectServer(void)
{
	int reVal;          //����ֵ  
	sockaddr_in serAddr;//��������ַ  
						//����Ҫ���ӵ�������ַ  
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8888);
	serAddr.sin_addr.S_un.S_addr = inet_addr("172.18.103.161");

	while (true)
	{
		//���ӷ�����  
		reVal = connect(sClient, (struct sockaddr*)&serAddr, sizeof(serAddr));
		//�������Ӵ���  
		if (SOCKET_ERROR == reVal)
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == nErrCode || WSAEINVAL == nErrCode)    //���ӻ�û�����  
			{
				continue;
			}
			else if (WSAEISCONN == nErrCode)//�����Ѿ����  
			{
				break;
			}
			else//����ԭ������ʧ��  
			{
				return FALSE;
			}
		}

		if (reVal == 0)//���ӳɹ�  
			break;
	}

	bConnecting = TRUE;

	return TRUE;
}


/**
*  ��ȡ����
*/
bool recvData(SOCKET s, char* buf)
{
	BOOL retVal = TRUE;
	bool bLineEnd = FALSE;      //�н���  
	memset(buf, 0, MAX_BUFF);        //��ս��ջ�����  
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
				retVal = FALSE; //������ʧ��  
				break;                          //�߳��˳�  
			}
		}

		if (0 == nReadLen)           //δ��ȡ������  
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
	int retVal;//����ֵ  
	int nlength = strlen(str);
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

		if(nlength==0)
			break;
	}

	return TRUE;        //���ͳɹ�  
}

/**
* �ͻ����˳�
*/
void ExitClient(void)
{
	//  DeleteCriticalSection(&cs);  
	//  CloseHandle(handleThread);  
	closesocket(sClient);
	WSACleanup();
}

/**
* ��ʾ���ӷ�����ʧ����Ϣ
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