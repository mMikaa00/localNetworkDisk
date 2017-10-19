#include "server.h"

userManager sUsers;
HANDLE sAcceptThread;
CRITICAL_SECTION cs;
SOCKET sServer;
bool sConfig;
char dataBuf[MAX_BUFF];

void InitMember() {
	{
		InitializeCriticalSection(&cs);                         //��ʼ���ٽ���  
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

	cout << "createThread succeed" << endl;
	pair<string, string> curuser;
	if (!getuserInfo(accept, curuser)) {				//��ȡ�û���Ϣ
		exitThread((SOCKET*)pParam);
		return 0;
	}
	EnterCriticalSection(&cs);
	if (!sUsers.finduser(curuser)) {					//������ݿ����Ƿ��Ѵ��ڸ��û���������������Ӹ��û�
		if (!sUsers.adduser(curuser))
		{
			cout << "add user failed!" << endl;
			LeaveCriticalSection(&cs);
			exitThread((SOCKET*)pParam);
			return 0;
		}
		else
			cout << "add new user to database" << endl;
	}
	
	if (sUsers.getgroup(curuser.second)->finduser(curuser.first) != -1)				//�����û��Ƿ�������
	{
		cout << "other user had connected using this id..." << endl;
		LeaveCriticalSection(&cs);
		exitThread((SOCKET*)pParam);
		return 0;
	}

	cout << "client " << curuser.first << ' ' << curuser.second <<" connected!" << endl;

	group* curgroup = sUsers.getgroup(curuser.second);							//��group�и�user���ֵ��Ϊsocketֵ����ʶ���û������ӣ�����ȡgroup�е������¼����ؼ��κ��ļ�����
	LeaveCriticalSection(&cs);

	CRITICAL_SECTION &groupcs = curgroup->getcs();								//��group�л�ȡ��Ҫ����Դ
	curgroup->setsocket(curuser.first, accept);							
	HANDLE &event1 = curgroup->getevent1();
	HANDLE &event2 = curgroup->getevent2();
	int &read_num = curgroup->getrdn();
	unordered_map<string,file> &filefolder = curgroup->getfile();

	char recvbuf[100];
	while (1) {
		if(!recvData(accept, recvbuf))
			break;
		if (!strcmp(recvbuf, "SYN")) {								//ʵ��ͬ�����ύ���ֲ�������Ӧ��ͬ����Ӧֻ��ȡ���ݣ��ύ��Ӧ���޸����ݣ�ͬ����Ӧ��ʼʱ���������������̶߳�ȡ���ݵ��������޸Ĳ���
			EnterCriticalSection(&groupcs);
			++read_num;
			curgroup->setsocket(curuser.first, 0);					//���û�����ͬ�����ݣ���socket���ܱ������̷߳���
			LeaveCriticalSection(&groupcs);

			WaitForSingleObject(event1, INFINITE);					//
			ResetEvent(event2);
			synchronizeData(accept, curuser,filefolder);

			EnterCriticalSection(&groupcs);
			if (--read_num==0)
				SetEvent(event2);
			curgroup->setsocket(curuser.first, accept);				//�û�ͬ��������ɣ���socket���Ա������̷߳���
			LeaveCriticalSection(&groupcs);	
		}
		else if (!strcmp(recvbuf, "CMT")) {
			EnterCriticalSection(&groupcs);
			curgroup->setsocket(curuser.first, 0);
			LeaveCriticalSection(&groupcs);

			ResetEvent(event1);									//һ���ύ��Ӧ��ʼ����ʱ�������ݽ��������������������߳��޸Ļ��ȡ
			WaitForSingleObject(event2, INFINITE);
			ResetEvent(event2);
			commitData(accept,curuser,filefolder);
			
			for (auto &k : curgroup->getusers()) {				//�ύ�ɹ���֪ͨ���������������û�ͬ������
				if (k.second<=1)
					continue;
				sendData(k.second, "SYN");
			}


			SetEvent(event1);
			SetEvent(event2);

			EnterCriticalSection(&groupcs);
			curgroup->setsocket(curuser.first, accept);
			LeaveCriticalSection(&groupcs);
		}
	}
	

	cout << "client " << curuser.first << " disconnected!" << endl;

	curgroup->setsocket(curuser.first, -1);

	exitThread((SOCKET*)pParam);
	return 0;//�߳��˳�  
}


bool getuserInfo(SOCKET &s,pair<string,string> &user)			//��ȡ�û���Ϣ
{
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
* �����߳�
*/
void exitThread(SOCKET* s) {
	closesocket(*s);
	delete s;
}

/**
* ͬ������
*/

void synchronizeData(SOCKET &s,pair<string,string> &user, unordered_map<string,file> &filefolder) {
	cout << user.first << " is synchronizing!" << endl;
	for (auto &k : filefolder) 										//���ͱ��������ļ�У����Ϣ��Զ��
		sendcheckcode(s, k.second);
	sendcheckcode(s, file("end", ""));

	while (1) {														//��ӦԶ���ļ�����
		
		recvData(s, dataBuf,100);
		if (strcmp(dataBuf, "end"))
			sendfile(s, filefolder[dataBuf]);
		else
			break;
	}								

	//file test("liuwenbo", "woshiyigehaha");
	cout << user.first << " complete synchronizing!" << endl;
}

/**
* ����ͻ����ύ��������
*/

void commitData(SOCKET &s, pair<string, string> &user, unordered_map<string,file> &filefolder) {
	cout << user.first << " is commiting!" << endl;

	unordered_map<string, int> checkcode;						//����Զ��У����Ϣ�����ֵ�
	while (1){
		auto temp=recvcheckcode(s);
		if (temp.first != "end")
			checkcode.insert(temp);
		else
			break;
	}

	for (auto i=filefolder.begin();i!=filefolder.end();) {			//����check�ֵ��鱾�������Ƿ���Ҫɾ������£�����Ҫ���µ�fileУ����Ϣ���͸�Զ��
		auto curcode = checkcode.find(i->first);
		if (curcode == checkcode.end())
			i = filefolder.erase(i);
		else if (i->second.getversion() == curcode->second) {
			checkcode.erase(curcode);
			++i;
		}
	}

	for (auto &k : checkcode) {									//�����ֵ���ʣ����ļ�����Զ�������ļ�����
		char neededfile[100];
		strcpy(neededfile, k.first.c_str());
		sendData(s,neededfile,100);
		file temp=recvfile(s);
		filefolder[temp.getfileId()] = temp;
	}

	Sleep(5000);//tttttttttttttttt
	sendData(s, "end",100);
	cout << user.first << " complete commiting!" << endl;
}

/**
* ������֤��
*/
void sendcheckcode(SOCKET &s, file &f) {
	strcpy(dataBuf, f.getfileId().c_str());
	sendData(s, dataBuf, 100);
	memcpy(dataBuf, &f.getversion(), 4);
	dataBuf[4] = 0;
	sendData(s, dataBuf, 4);
}
/**
* ������֤��
*/
pair<string, int> recvcheckcode(SOCKET &s) {
	pair<string, int> ret;
	recvData(s, dataBuf, 100);
	ret.first = dataBuf;
	recvData(s, dataBuf, 4);
	ret.second = (*(int*)dataBuf);
	return ret;
}
/**
* �����ļ�
*/
void sendfile(SOCKET &s, file &f) {
	strcpy(dataBuf, f.getfileId().c_str());
	sendData(s, dataBuf, 100);
	strcpy(dataBuf, f.getcontent().c_str());
	sendData(s, dataBuf, MAX_BUFF);
	memcpy(dataBuf, &f.getversion(), 4);
	dataBuf[4] = 0;
	sendData(s, dataBuf, 4);
}
/**
* �����ļ�
*/
file recvfile(SOCKET &s) {
	file temp;
	recvData(s, dataBuf, 100);
	temp.setfileId(dataBuf);
	recvData(s, dataBuf, MAX_BUFF);
	temp.setcontent(dataBuf);
	recvData(s, dataBuf, 4);
	temp.setversion(*(int*)dataBuf);
	return temp;
}