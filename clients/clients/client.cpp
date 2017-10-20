#include "client.h"  


//����  
SOCKET  sClient,sStdinfd;                            //�׽���  
char dataBuf[MAX_BUFF];                  //���ݻ�����    
BOOL    bConnecting;                        //�������������״̬  
unordered_map<string, file> filefolder;//{ {"abc",file("abc","fdf")},{ "qwe",file("qwe","grerer") },{ "zxc",file("zxc","kjlrtoi") }, };					//�洢�����ļ��ռ�
HANDLE hMapFile;
int *port;

/**
��ʼ��ȫ�ֱ���
*/
void InitMember(void)
{
	//  InitializeCriticalSection(&cs);  

	sClient = INVALID_SOCKET;   //�׽���  
	sStdinfd = INVALID_SOCKET;

	bConnecting = FALSE;        //Ϊ����״̬  

	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // �����ļ����  
		NULL,                    // Ĭ�ϰ�ȫ����  
		PAGE_READWRITE,          // �ɶ���д  
		0,                       // ��λ�ļ���С  
		64,                // ��λ�ļ���С  
		L"portnum"                   // �����ڴ�����  
	);

	port = (int *)MapViewOfFile(
		hMapFile,            // �����ڴ�ľ��  
		FILE_MAP_ALL_ACCESS, // �ɶ�д���  
		0,
		0,
		64
	);

	
}

/**
* �����׽���
*/
BOOL  InitSocket(void)
{
	int         reVal;  //����ֵ  
	WSADATA     wsData; //WSADATA����  
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);//��ʼ��Windows Sockets Dll  

												//�����׽���  
	sClient = socket(AF_INET, SOCK_STREAM, 0);
	sStdinfd = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sClient|| INVALID_SOCKET == sStdinfd)
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
* ��ʼ�������̲߳�����
*/
bool InitStdinThread() {
	if (*port >= 8900 || *port <= 8888)
		*port = 8889;
	else
		*port += 1;
	cout << "port=" << *port << endl;//tttttttttttttt
	DWORD dwThreadId;
	HANDLE hThread;
	DWORD dwWaitResult;
	HANDLE hSemaphore = CreateSemaphore(NULL, 0, 1,L"liuwenbo" );
	hThread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &dwThreadId);
	if (hThread == NULL)
	{
		cout << "CreateThread failed." << endl;
		return false;
	}
	dwWaitResult = WaitForSingleObject(hSemaphore, INFINITE);
	switch (dwWaitResult)
	{
	case WAIT_OBJECT_0:
		cout<<"stdin-sock is established!" << endl;
		break;
	default:
		cout<<"wait the stdin-sock thread failed!"<<endl;
		return -1;
	}

	sockaddr_in stdserv;
	stdserv.sin_family = AF_INET;
	stdserv.sin_port = htons(*port);
	stdserv.sin_addr.s_addr = inet_addr("172.18.103.161");

	connect(sStdinfd, (SOCKADDR*)&stdserv, sizeof(stdserv));
	CloseHandle(hSemaphore);
}


DWORD WINAPI ThreadFunc(LPVOID lpParam)
{
	char buf[100];
	SOCKET listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(*port);
	servaddr.sin_addr.s_addr = inet_addr("172.18.103.161");

	bind(listenfd, (SOCKADDR*)&servaddr, sizeof(servaddr));
	listen(listenfd, 5);
	
	HANDLE hSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, L"liuwenbo");
	ReleaseSemaphore(hSemaphore, 1, NULL);
	SOCKET connfd = accept(listenfd, NULL, NULL);
		if (connfd != -1) {
			cout << "stdin connected!" << endl;
		}
		else
			cout << "stdin connect error!" << endl;

	while (1) {
		cin >> buf;
		send(connfd, buf, strlen(buf) , 0);
	}
	return 0;
}

/**
* �ͻ����˳�
*/
void ExitClient(void)
{
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
/**
* �����û���Ϣ
*/

bool sendUserid()
{
	cout << "please input your userid and groupid:" << endl;
	char input[100];
	cin >> input;
	if (!sendData(sClient, input))
		return false;
	cin >> input;
	if (!sendData(sClient, input))
		return false;
	return true;
}
/**
* �����˺��û������ӿ�
*/
void getinfo() {
	sendData(sClient, "SYN",4);
	synchronizeData();
	int maxfd;
	fd_set rfd;
	FD_ZERO(&rfd);
	char input[50];
	while (1) {
		int n;
		FD_SET(sStdinfd, &rfd);
		FD_SET(sClient, &rfd);
		maxfd = max(sStdinfd, sClient) + 1;
		memset(input, 0, 50);
		n=select(maxfd, &rfd, NULL, NULL, NULL);
		if (FD_ISSET(sStdinfd, &rfd)) {
			recvData(sStdinfd, input);

			if (!strcmp(input, "SYN")) {
				sendData(sClient, "SYN",4);
				synchronizeData();
			}
			else if (!strcmp(input, "CMT")) {
				sendData(sClient, "CMT",4);
				commitData();
			}
			else if (!strcmp(input, "print")) {
				print();
			}
			else if (!strcmp(input, "add")) {
				char temp[50];
				recvData(sStdinfd, temp);
				filefolder.emplace(temp, file(temp, "adsfkjasdflk"));
				sendData(sClient, "CMT",4);
				commitData();
				synchronizeData();
			}
			else if (!strcmp(input, "edit")) {
				char temp[50];
				recvData(sStdinfd, temp);
				auto g = filefolder.find(temp);
				if (g != filefolder.end()) {
					g->second.setversion(16);
					sendData(sClient, "CMT",4);
					Sleep(5000);			//ģ���ӻ��ύ
					commitData();
					synchronizeData();
				}
			}
		}

		if (FD_ISSET(sClient, &rfd)) {
			recvData(sClient, dataBuf);
			if (!strcmp(dataBuf, "SYN")) {
				sendData(sClient, dataBuf,4);
				synchronizeData();
			}
		}
	}
}

void synchronizeData() {

	unordered_map<string, int> checkcode;						//����Զ��У����Ϣ�����ֵ�
	while (1) {
		auto temp = recvcheckcode(sClient);
		if (temp.first != "end")
			checkcode.insert(temp);
		else
			break;
	}

	for (auto i = filefolder.begin(); i != filefolder.end();) {			//����check�ֵ��鱾�������Ƿ���Ҫɾ������£�����Ҫ���µ�fileУ����Ϣ���͸�Զ��
		auto curcode = checkcode.find(i->first);
		if (curcode == checkcode.end()) {
			i = filefolder.erase(i);
			continue;
		}
		else if (i->second.getversion() >= curcode->second) 
			checkcode.erase(curcode);
		++i;
	}

	for (auto &k : checkcode) {									//�����ֵ���ʣ����ļ�����Զ�������ļ�����
		char neededfile[100];
		strcpy(neededfile, k.first.c_str());
		sendData(sClient, neededfile, 100);
		file temp = recvfile(sClient);
		filefolder[temp.getfileId()] = temp;
	}
	sendData(sClient, "end",100);
}

void commitData() {
	for (auto &k : filefolder) 										//���ͱ��������ļ�У����Ϣ��Զ��
		sendcheckcode(sClient, k.second);
	sendcheckcode(sClient, file("end", ""));

	while (1) {														//���ͶԶ�������ļ�
		recvData(sClient, dataBuf,100);
		if (strcmp(dataBuf, "end"))
			sendfile(sClient, filefolder[dataBuf]);
		else
			break;
	}
}

void sendcheckcode(SOCKET &s, file &f) {
	strcpy(dataBuf, f.getfileId().c_str());
	sendData(s, dataBuf, 100);
	memcpy(dataBuf, &f.getversion(), 4);
	dataBuf[4] = 0;
	sendData(s, dataBuf, 4);
}

pair<string, int> recvcheckcode(SOCKET &s) {
	pair<string, int> ret;
	recvData(s, dataBuf, 100);
	ret.first = dataBuf;
	recvData(s, dataBuf, 4);
	ret.second = (*(int*)dataBuf);
	return ret;
}

void sendfile(SOCKET &s, file &f) {
	strcpy(dataBuf, f.getfileId().c_str());
	sendData(s, dataBuf, 100);
	strcpy(dataBuf, f.getcontent().c_str());
	sendData(s, dataBuf, MAX_BUFF);
	memcpy(dataBuf, &f.getversion(), 4);
	dataBuf[4] = 0;
	sendData(s, dataBuf, 4);
}

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


void print() {
	cout << setw(10) << "FileId"<<setw(15)<<"content"<<setw(10)<<"version"<<endl;
	for (auto &k : filefolder) {
		cout << setw(10) << k.second.getfileId() << setw(15) << k.second.getcontent() << setw(10) << k.second.getversion() << endl;
	}
}