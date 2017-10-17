#include "server.h"

userManager sUsers;
char sDataBuff[MAX_BUFF];
HANDLE sAcceptThread;
CRITICAL_SECTION cs;
SOCKET sServer;
bool sConfig;

void InitMember() {
	{
		InitializeCriticalSection(&cs);                         //��ʼ���ٽ���  
		memset(sDataBuff, 0, MAX_BUFF);
		sAcceptThread = NULL;                                   //����ΪNULL  
		sServer = INVALID_SOCKET;                               //����Ϊ��Ч���׽���  
		sConfig = false;
	}
}

bool InitSocket(void)
{
	//����ֵ  
	int reVal;

	//��ʼ��Windows Sockets DLL  
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);

	//�����׽���  
	sServer = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sServer)
		return FALSE;

	//���׽���  
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8888);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	reVal = bind(sServer, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if (SOCKET_ERROR == reVal)
		return FALSE;

	//����  
	reVal = listen(sServer, SOMAXCONN);
	if (SOCKET_ERROR == reVal)
		return FALSE;

	//�ȴ��ͻ��˵�����  
	cout << "Server succeeded!" << endl;
	cout << "Waiting for clients..." << endl;

	return TRUE;
}

BOOL createAcceptThread(void)
{
	sConfig = TRUE;//���÷�����Ϊ����״̬  
	SOCKET  sAccept;                            //���ܿͻ������ӵ��׽���  
	sockaddr_in addrClient;                     //�ͻ���SOCKET��ַ  
	//�����ͷ���Դ�߳�  
	unsigned long ulThreadId;
	//�������տͻ��������߳�
	while (sConfig) {
		memset(&addrClient, 0, sizeof(sockaddr_in));                    //��ʼ��  
		int lenClient = sizeof(sockaddr_in);							//��ַ���� 
		sAccept = accept(sServer, (sockaddr*)&addrClient, &lenClient);  //���ܿͻ����� 
		

		if (INVALID_SOCKET == sAccept)
		{
			Sleep(100);
			int nErrCode = WSAGetLastError();
			if (nErrCode == WSAEWOULDBLOCK)  //�޷��������һ�����赲���׽��ֲ���  
			{
				Sleep(100);
				continue;//�����ȴ�  
			}
			else
			{
				return 0;//�߳��˳�  
			}

		}
		else//���ܿͻ��˵�����  
		{
			//��ʾ�ͻ��˵�IP�Ͷ˿�  
			char *pClientIP = inet_ntoa(addrClient.sin_addr);
			u_short  clientPort = ntohs(addrClient.sin_port);
			cout << "Accept a new client." << endl;
			cout << "IP: " << pClientIP << "\tPort: " << clientPort << endl;
		}

		SOCKET *tempAccept = new SOCKET(sAccept);
		sAcceptThread = CreateThread(NULL, 0, AcceptThreadFunc, tempAccept, 0, &ulThreadId);
		if (NULL == sAcceptThread)
		{
			sConfig = FALSE;
			return FALSE;
		}
	}
	return TRUE;
}

DWORD __stdcall AcceptThreadFunc(void* pParam)
{
	SOCKET accept = *(SOCKET*)pParam;

	cout << "createThread succeed"  << endl;
	pair<string,string> curuser;
	if(!getuserInfo(accept,curuser))				//��ȡ�û���Ϣ
		return 0;
	EnterCriticalSection(&cs);
	if (!sUsers.finduser(curuser)) {					//������ݿ����Ƿ��Ѵ��ڸ��û���������������Ӹ��û�
		if (sUsers.adduser(curuser))
			cout << "add new user to database" << endl;
		else
			cout << "add failed!" << endl;
	}

	if (sUsers.getgroup(curuser.second)->finduser(curuser.first)!=-1)				//�����û��Ƿ�������
	{	
		cout << "other user had connected using this id..." << endl;
	}
	else {
		sUsers.getgroup(curuser.second)->setsocket(curuser.first, accept);			//��group�и�user���ֵ��Ϊsocketֵ����ʶ���û�������
		
	}

	LeaveCriticalSection(&cs);
	cout << curuser.first << ' ' << curuser.second << endl;

	cout << "client " << curuser.first << " disconnected!" << endl;
	
	sUsers.getgroup(curuser.second)->setsocket(curuser.first, -1);
	closesocket(accept);
	delete pParam;
	return 0;//�߳��˳�  
}

bool getuserInfo(SOCKET s,pair<string,string> &user) {
	char userId[MAX_BUFF];
	char groupId[MAX_BUFF];
	if(!recvData(s, userId))
		return false;
	if(!recvData(s, groupId))
		return false;
	user.first = userId;
	user.second = groupId;
	return true;
}

/**
* @Des:send data to client
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

		if (nlength == 0)
			break;
	}

	return TRUE;        //���ͳɹ�  
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
