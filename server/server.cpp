#include "server.h"

userManager sUsers;
HANDLE sAcceptThread;
HANDLE sAcceptSocket;
CRITICAL_SECTION cs;
SOCKET sServer;
bool sConfig;

void InitMember() {
	{
		InitializeCriticalSection(&cs);                         //初始化临界区  
		sAcceptThread = NULL;                                   //设置为NULL  
		sServer = INVALID_SOCKET;                               //设置为无效的套接字  
		sConfig = false;
		HANDLE hSemaphore = CreateSemaphore(NULL, 0, 1, NULL);
	}
}

bool InitSocket(void)
{
	//返回值  
	int reVal;

	//初始化Windows Sockets DLL  
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);

	//创建套接字  
	sServer = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sServer)
		return FALSE;

	//绑定套接字  
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8888);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	reVal = bind(sServer, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if (SOCKET_ERROR == reVal)
		return FALSE;

	//监听  
	reVal = listen(sServer, SOMAXCONN);
	if (SOCKET_ERROR == reVal)
		return FALSE;

	//等待客户端的连接  
	cout << "Server succeeded!" << endl;
	cout << "Waiting for clients..." << endl;

	return TRUE;
}

BOOL createAcceptThread(void)
{
	sConfig = TRUE;//设置服务器为运行状态  
	SOCKET  sAccept;                            //接受客户端连接的套接字  
	sockaddr_in addrClient;                     //客户端SOCKET地址  
	//创建释放资源线程  
	unsigned long ulThreadId;
	//创建接收客户端请求线程
	while (sConfig) {
		memset(&addrClient, 0, sizeof(sockaddr_in));                    //初始化  
		int lenClient = sizeof(sockaddr_in);							//地址长度 
		sAccept = accept(sServer, (sockaddr*)&addrClient, &lenClient);  //接受客户请求 
		

		if (INVALID_SOCKET == sAccept)
		{
			Sleep(100);
			int nErrCode = WSAGetLastError();
			if (nErrCode == WSAEWOULDBLOCK)  //无法立即完成一个非阻挡性套接字操作  
			{
				Sleep(100);
				continue;//继续等待  
			}
			else
			{
				return 0;//线程退出  
			}

		}
		else//接受客户端的请求  
		{
			//显示客户端的IP和端口  
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
		getuserInfo(accept, curuser);  				//获取用户信息
	}
	catch (const std::exception& k) {
		cout << "client terminated early" << endl;
		return 0;
	}


	EnterCriticalSection(&cs);
	if (!sUsers.finduser(curuser)) {					//检查数据库中是否已存在该用户，若不存在则添加该用户
		if (!sUsers.adduser(curuser))
		{
			cout << "add user failed!" << endl;
			LeaveCriticalSection(&cs);
			return 0;
		}
		else
			cout << "add new user to database" << endl;
	}

	if (sUsers.getgroup(curuser.second)->finduser(curuser.first) != -1)				//检查该用户是否已连接
	{
		cout << "other user had connected using this id..." << endl;
		sendData(accept, "FALSE");
		LeaveCriticalSection(&cs);
		return 0;
	}
	sendData(accept, "TRUE");
	cout << "client " << curuser.first << ' ' << curuser.second << " connected!" << endl;

	group* curgroup = sUsers.getgroup(curuser.second);							//将group中该user配对值设为socket值，标识该用户已连接，并获取group中的两个事件，关键段和文件引用
	LeaveCriticalSection(&cs);

	CRITICAL_SECTION &groupcs = curgroup->getcs();								//从group中获取需要的资源
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
			if (!strcmp(buf, "SYN")) {								//实现同步与提交两种操作的响应，同步响应只读取数据，提交响应会修改数据，同步相应开始时，可以允许其他线程读取数据但不允许修改操作
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
				ResetEvent(event1);									//一旦提交相应开始进行时，将数据进行锁定，不允许其他线程修改或读取
				ResetEvent(event2);

				cout << curuser.first << " is commiting!" << endl;
				sendData(accept, "TRUE");
				ts.commitData();
				ts.synchronizeData();
				cout << curuser.first << " complete commiting!" << endl;

				for (auto &k : curgroup->getusers()) {				//提交成功后通知该组其他已连接用户同步数据
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
	return 0;//线程退出  
}


void getuserInfo(SOCKET &s,pair<string,string> &user)			//获取用户信息
{
	char userId[100];
	char groupId[100];
	recvData(s, userId, 100);
	recvData(s, groupId, 100);
	user.first = userId;
	user.second = groupId;
}

