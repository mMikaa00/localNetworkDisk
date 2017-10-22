#include "server.h"

userManager sUsers;
HANDLE sAcceptThread;
HANDLE sAcceptSocket;
CRITICAL_SECTION cs;
SOCKET sServer;
bool sConfig;

void InitMember() {
	{
		InitializeCriticalSection(&cs);                         //��ʼ���ٽ���  
		sAcceptThread = NULL;                                   //����ΪNULL  
		sServer = INVALID_SOCKET;                               //����Ϊ��Ч���׽���  
		sConfig = false;
		HANDLE hSemaphore = CreateSemaphore(NULL, 0, 1, NULL);
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

		sAcceptThread = CreateThread(NULL, 0, AcceptThreadFunc, &sAccept, 0, &ulThreadId);
		if (NULL == sAcceptThread)
		{
			sConfig = FALSE;
			return FALSE;
		}
		WaitForSingleObject(sAcceptSocket, 1000);
		CloseHandle(sAcceptThread);
	}
	return TRUE;
}

DWORD __stdcall AcceptThreadFunc(void* pParam)
{
	SOCKET accept = *(SOCKET*)pParam;
	ReleaseSemaphore(sAcceptSocket, 1, NULL);
	socketManager sm(accept);

	pair<string, string> curuser;
	cout << "createThread succeed" << endl;
	try
	{
		getuserInfo(accept, curuser);  				//��ȡ�û���Ϣ
	}
	catch (const std::exception& k) {
		cout << "client terminated early" << endl;
		return 0;
	}


	EnterCriticalSection(&cs);
	if (!sUsers.finduser(curuser)) {					//������ݿ����Ƿ��Ѵ��ڸ��û���������������Ӹ��û�
		if (!sUsers.adduser(curuser))
		{
			cout << "add user failed!" << endl;
			LeaveCriticalSection(&cs);
			return 0;
		}
		else
			cout << "add new user to database" << endl;
	}

	if (sUsers.getgroup(curuser.second)->finduser(curuser.first) != -1)				//�����û��Ƿ�������
	{
		cout << "other user had connected using this id..." << endl;
		sendData(accept, "FALSE");
		LeaveCriticalSection(&cs);
		return 0;
	}
	sendData(accept, "TRUE");
	cout << "client " << curuser.first << ' ' << curuser.second << " connected!" << endl;

	group* curgroup = sUsers.getgroup(curuser.second);							//��group�и�user���ֵ��Ϊsocketֵ����ʶ���û������ӣ�����ȡgroup�е������¼����ؼ��κ��ļ�����
	LeaveCriticalSection(&cs);

	CRITICAL_SECTION &groupcs = curgroup->getcs();								//��group�л�ȡ��Ҫ����Դ
	curgroup->setsocket(curuser.first, accept);
	HANDLE &event1 = curgroup->getevent1();
	HANDLE &event2 = curgroup->getevent2();
	int &read_num = curgroup->getrdn();
	unordered_map<string, file> &fileFolder = curgroup->getfile();
	transmitor ts(accept, fileFolder,"D:\\test\\server\\");

	try {
		char buf[10];
		while (1) {
			memset(buf, 0, 10);
			recvData(accept, buf, 4);
			if (!strcmp(buf, "SYN")) {								//ʵ��ͬ�����ύ���ֲ�������Ӧ��ͬ����Ӧֻ��ȡ���ݣ��ύ��Ӧ���޸����ݣ�ͬ����Ӧ��ʼʱ���������������̶߳�ȡ���ݵ��������޸Ĳ���
				EnterCriticalSection(&groupcs);
				++read_num;
				LeaveCriticalSection(&groupcs);

				WaitForSingleObject(event1, INFINITE);					//
				ResetEvent(event2);
				sendData(accept, "TRUE");
				cout << curuser.first << " is synchronizing!" << endl;
				ts.synchronizeData();
				cout << curuser.first << " complete synchronizing!" << endl;

				EnterCriticalSection(&groupcs);
				if (--read_num == 0)
					SetEvent(event2);
				LeaveCriticalSection(&groupcs);
			}
			else if (!strcmp(buf, "CMT")) {
				WaitForSingleObject(event2, INFINITE);
				ResetEvent(event1);									//һ���ύ��Ӧ��ʼ����ʱ�������ݽ��������������������߳��޸Ļ��ȡ
				ResetEvent(event2);

				cout << curuser.first << " is commiting!" << endl;
				sendData(accept, "TRUE");
				ts.commitData();
				ts.synchronizeData();
				cout << curuser.first << " complete commiting!" << endl;

				for (auto &k : curgroup->getusers()) {				//�ύ�ɹ���֪ͨ���������������û�ͬ������
					if (k.second <= 1)
						continue;
					sendData(k.second, "SYN");
				}

				SetEvent(event1);
				SetEvent(event2);
			}
		}
	}
	catch (const std::exception& k)
	{
		cout << "client " <<curuser.first<< " disconnected!" << endl;

		EnterCriticalSection(&groupcs);
		curgroup->setsocket(curuser.first, -1);
		LeaveCriticalSection(&groupcs);
	}
	return 0;//�߳��˳�  
}


void getuserInfo(SOCKET &s,pair<string,string> &user)			//��ȡ�û���Ϣ
{
	char userId[100];
	char groupId[100];
	recvData(s, userId, 100);
	recvData(s, groupId, 100);
	user.first = userId;
	user.second = groupId;
}

