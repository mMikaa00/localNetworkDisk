#include "client.h"  


/**
初始化全局变量
*/
client::client()
{ 
	
	sClient = INVALID_SOCKET;   //套接字  
	sStdinfd = INVALID_SOCKET;
	hThread = INVALID_HANDLE_VALUE;
	
	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // 物理文件句柄  
		NULL,                    // 默认安全级别  
		PAGE_READWRITE,          // 可读可写  
		0,                       // 高位文件大小  
		64,                // 地位文件大小  
		"portnum"                   // 共享内存名称  
	);

	port = (int *)MapViewOfFile(
		hMapFile,            // 共享内存的句柄  
		FILE_MAP_ALL_ACCESS, // 可读写许可  
		0,
		0,
		64
	);
	
	char dir[100];
	GetCurrentDirectory(100, dir);
	path = dir;
	path += "\\";
	cout << path<<endl;
}
client::~client() {
	CloseHandle(hMapFile);
}


/**
* 创建套接字
*/
BOOL  client::InitSocket(void)
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

// 获得本机的IP地址  
string client::GetLocalIP()
{
	// 获得本机主机名  
	char hostname[100] = { 0 };
	gethostname(hostname, 100);
	struct hostent FAR* lpHostEnt = gethostbyname(hostname);
	if (lpHostEnt == NULL)
	{
		return "172.18.103.161";
	}

	// 取得IP地址列表中的第一个为返回的IP(因为一台主机可能会绑定多个IP)  
	LPSTR lpAddr = lpHostEnt->h_addr_list[0];

	// 将IP地址转化成字符串形式  
	struct in_addr inAddr;
	memmove(&inAddr, lpAddr, 4);

	return inet_ntoa(inAddr);
}

/**
* 连接服务器
*/
BOOL client::ConnectServer(void)
{
	int reVal;          //返回值  
	sockaddr_in serAddr;//服务器地址  
						//输入要连接的主机地址  
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8888);
	serAddr.sin_addr.S_un.S_addr = inet_addr(GetLocalIP().c_str());

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
bool client::InitStdinThread() {
	if (hThread != INVALID_HANDLE_VALUE)			//当线程已经被创建后不再重复创建，用于断网重连时客户端的reset
		return false;
	if (*port >= 8900 || *port <= 8888)
		*port = 8889;
	else
		*port += 1;
	cout << "port=" << *port << endl;//tttttttttttttt
	DWORD dwWaitResult;
	HANDLE hSemaphore = CreateSemaphore(NULL, 0, 1,"liuwenbo" );
	hThread = CreateThread(NULL, 0, ThreadFunc, this, 0, NULL);
	
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
		return false;
	}

	sockaddr_in stdserv;
	stdserv.sin_family = AF_INET;
	stdserv.sin_port = htons(*port);
	stdserv.sin_addr.s_addr = inet_addr(GetLocalIP().c_str());

	connect(sStdinfd, (SOCKADDR*)&stdserv, sizeof(stdserv));
	CloseHandle(hSemaphore);
	return true;
}


DWORD WINAPI ThreadFunc(LPVOID lpParam)
{
	client *cli = (client*)lpParam;
	char buf[100];
	SOCKET listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(*cli->port);
	servaddr.sin_addr.s_addr = inet_addr(client::GetLocalIP().c_str());

	bind(listenfd, (SOCKADDR*)&servaddr, sizeof(servaddr));
	listen(listenfd, 5);
	
	HANDLE hSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, "liuwenbo");
	ReleaseSemaphore(hSemaphore, 1, NULL);
	SOCKET connfd = accept(listenfd, NULL, NULL);
		if (connfd != -1) {
			cout << "stdin connected!" << endl;
		}
		else
			cout << "stdin connect error!" << endl;

	while (1) {
		cin >> buf;
		send(connfd, buf, strlen(buf)+1 , 0);
	}
	return 0;
}

/**
* 客户端退出
*/
void client::ExitClient(void)
{
	CloseHandle(hThread);
	closesocket(sClient);
	WSACleanup();
}
/**
* 重置SOCKET
*/
void client::resetSocket() {
	closesocket(sClient);
	sClient = socket(AF_INET, SOCK_STREAM, 0);
}
/**
* 获取用户信息
*/
void client::getUserInfo() {
	if (curuser.first.empty()) {		//用户已经输入过
		cout << "please input your userid and groupid:" << endl;
		cin >> curuser.first;
		cin >> curuser.second;
	}
}
/**
* 发送用户信息
*/
bool client::checkUserid()
{	
	char buf[100];
	strcpy(buf, curuser.first.c_str());
	sendData(sClient, buf, 100);
	strcpy(buf, curuser.second.c_str());
	sendData(sClient, buf, 100);
	recvData(sClient, buf, 100);
	if (!strcmp(buf, "FALSE"))
		return false;
	return true;
}
/**
* 与服务端和用户交互接口
*/
void client::getinfo() {
	initFileFolder((path+"*.*").c_str(), fileFolder);
	transmitor ts(sClient,fileFolder,path);
	SynchronizeData(ts);
	int maxfd;
	fd_set rfd;
	FD_ZERO(&rfd);
	char buf[100];
	while (1) {
		int n;
		FD_SET(sStdinfd, &rfd);
		FD_SET(sClient, &rfd);
		maxfd = max(sStdinfd, sClient) + 1;
		memset(buf, 0, 100);
		n=select(maxfd, &rfd, NULL, NULL, NULL);
		if (FD_ISSET(sStdinfd, &rfd)) {
			recvData(sStdinfd, buf,100);

			if (!strcmp(buf, "SYN")) 
				SynchronizeData(ts);
			else if (!strcmp(buf, "CMT")) 
				CommitData(ts);
			else if (!strcmp(buf, "print")) {
				print();
			}
			else if (!strcmp(buf, "edit")) {
				recvData(sStdinfd, buf,100);
				auto g = fileFolder.find(buf);
				if (g != fileFolder.end()) {
					g->second.setversion(16);
					CommitData(ts);
				}
			}
		}

		if (FD_ISSET(sClient, &rfd)) {
			recvData(sClient, buf,100);
			if (!strcmp(buf, "SYN")) 
				SynchronizeData(ts);
		}
	}
}


void client::print() {
	cout << setw(20) << "FileId"<<setw(30)<<"path"<<setw(10)<<"version"<<endl;
	for (auto &k : fileFolder) {
		cout << setw(20) << k.second.getfileId() << setw(30) << k.second.getpath() << setw(10) << k.second.getversion() << endl;
	}
}

void client::SynchronizeData(transmitor& ts) {
	sendData(sClient, "SYN", 4);
	char buf[10];
	for (int i = 0; i != 5; ++i) {
		recvData(sClient, buf,10);
		if (!strcmp(buf, "TRUE")) {
			ts.commitData();
			return;
		}
	}
}
void client::CommitData(transmitor& ts) {
	sendData(sClient, "CMT", 4);
	char buf[10];
	for (int i = 0; i != 5; ++i) {
		recvData(sClient, buf,10);
		if (!strcmp(buf, "TRUE")) {
			Sleep(5000);			//模拟延缓提交
			ts.synchronizeData();
			ts.commitData();
			return;
		}
	}

}