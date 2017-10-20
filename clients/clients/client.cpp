#include "client.h"  


//变量  
SOCKET  sClient,sStdinfd;                            //套接字  
char dataBuf[MAX_BUFF];                  //数据缓冲区    
BOOL    bConnecting;                        //与服务器的连接状态  
unordered_map<string, file> filefolder;//{ {"abc",file("abc","fdf")},{ "qwe",file("qwe","grerer") },{ "zxc",file("zxc","kjlrtoi") }, };					//存储本地文件空间
HANDLE hMapFile;
int *port;

/**
初始化全局变量
*/
void InitMember(void)
{
	//  InitializeCriticalSection(&cs);  

	sClient = INVALID_SOCKET;   //套接字  
	sStdinfd = INVALID_SOCKET;

	bConnecting = FALSE;        //为连接状态  

	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // 物理文件句柄  
		NULL,                    // 默认安全级别  
		PAGE_READWRITE,          // 可读可写  
		0,                       // 高位文件大小  
		64,                // 地位文件大小  
		L"portnum"                   // 共享内存名称  
	);

	port = (int *)MapViewOfFile(
		hMapFile,            // 共享内存的句柄  
		FILE_MAP_ALL_ACCESS, // 可读写许可  
		0,
		0,
		64
	);

	
}

/**
* 创建套接字
*/
BOOL  InitSocket(void)
{
	int         reVal;  //返回值  
	WSADATA     wsData; //WSADATA变量  
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);//初始化Windows Sockets Dll  

												//创建套接字  
	sClient = socket(AF_INET, SOCK_STREAM, 0);
	sStdinfd = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sClient|| INVALID_SOCKET == sStdinfd)
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
* 初始化输入线程并连接
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
* 客户端退出
*/
void ExitClient(void)
{
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
/**
* 发送用户信息
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
* 与服务端和用户交互接口
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
					Sleep(5000);			//模拟延缓提交
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

	unordered_map<string, int> checkcode;						//接受远端校验信息存入字典
	while (1) {
		auto temp = recvcheckcode(sClient);
		if (temp.first != "end")
			checkcode.insert(temp);
		else
			break;
	}

	for (auto i = filefolder.begin(); i != filefolder.end();) {			//根据check字典检查本地数据是否需要删除或更新，将需要更新的file校验信息发送给远端
		auto curcode = checkcode.find(i->first);
		if (curcode == checkcode.end()) {
			i = filefolder.erase(i);
			continue;
		}
		else if (i->second.getversion() >= curcode->second) 
			checkcode.erase(curcode);
		++i;
	}

	for (auto &k : checkcode) {									//根据字典中剩余的文件名向远端申请文件数据
		char neededfile[100];
		strcpy(neededfile, k.first.c_str());
		sendData(sClient, neededfile, 100);
		file temp = recvfile(sClient);
		filefolder[temp.getfileId()] = temp;
	}
	sendData(sClient, "end",100);
}

void commitData() {
	for (auto &k : filefolder) 										//发送本端所有文件校验信息给远端
		sendcheckcode(sClient, k.second);
	sendcheckcode(sClient, file("end", ""));

	while (1) {														//发送对端请求的文件
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