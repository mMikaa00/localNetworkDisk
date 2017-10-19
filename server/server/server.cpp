#include "server.h"

userManager sUsers;
HANDLE sAcceptThread;
CRITICAL_SECTION cs;
SOCKET sServer;
bool sConfig;
char dataBuf[MAX_BUFF];

void InitMember() {
	{
		InitializeCriticalSection(&cs);                         //初始化临界区  
		sAcceptThread = NULL;                                   //设置为NULL  
		sServer = INVALID_SOCKET;                               //设置为无效的套接字  
		sConfig = false;
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
	if (!getuserInfo(accept, curuser)) {				//获取用户信息
		exitThread((SOCKET*)pParam);
		return 0;
	}
	EnterCriticalSection(&cs);
	if (!sUsers.finduser(curuser)) {					//检查数据库中是否已存在该用户，若不存在则添加该用户
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
	
	if (sUsers.getgroup(curuser.second)->finduser(curuser.first) != -1)				//检查该用户是否已连接
	{
		cout << "other user had connected using this id..." << endl;
		LeaveCriticalSection(&cs);
		exitThread((SOCKET*)pParam);
		return 0;
	}

	cout << "client " << curuser.first << ' ' << curuser.second <<" connected!" << endl;

	group* curgroup = sUsers.getgroup(curuser.second);							//将group中该user配对值设为socket值，标识该用户已连接，并获取group中的两个事件，关键段和文件引用
	LeaveCriticalSection(&cs);

	CRITICAL_SECTION &groupcs = curgroup->getcs();								//从group中获取需要的资源
	curgroup->setsocket(curuser.first, accept);							
	HANDLE &event1 = curgroup->getevent1();
	HANDLE &event2 = curgroup->getevent2();
	int &read_num = curgroup->getrdn();
	unordered_map<string,file> &filefolder = curgroup->getfile();

	char recvbuf[100];
	while (1) {
		if(!recvData(accept, recvbuf))
			break;
		if (!strcmp(recvbuf, "SYN")) {								//实现同步与提交两种操作的响应，同步响应只读取数据，提交响应会修改数据，同步相应开始时，可以允许其他线程读取数据但不允许修改操作
			EnterCriticalSection(&groupcs);
			++read_num;
			curgroup->setsocket(curuser.first, 0);					//该用户即将同步数据，其socket不能被其他线程访问
			LeaveCriticalSection(&groupcs);

			WaitForSingleObject(event1, INFINITE);					//
			ResetEvent(event2);
			synchronizeData(accept, curuser,filefolder);

			EnterCriticalSection(&groupcs);
			if (--read_num==0)
				SetEvent(event2);
			curgroup->setsocket(curuser.first, accept);				//用户同步数据完成，其socket可以被其他线程访问
			LeaveCriticalSection(&groupcs);	
		}
		else if (!strcmp(recvbuf, "CMT")) {
			EnterCriticalSection(&groupcs);
			curgroup->setsocket(curuser.first, 0);
			LeaveCriticalSection(&groupcs);

			ResetEvent(event1);									//一旦提交相应开始进行时，将数据进行锁定，不允许其他线程修改或读取
			WaitForSingleObject(event2, INFINITE);
			ResetEvent(event2);
			commitData(accept,curuser,filefolder);
			
			for (auto &k : curgroup->getusers()) {				//提交成功后通知该组其他已连接用户同步数据
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
	return 0;//线程退出  
}


bool getuserInfo(SOCKET &s,pair<string,string> &user)			//获取用户信息
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
* 结束线程
*/
void exitThread(SOCKET* s) {
	closesocket(*s);
	delete s;
}

/**
* 同步数据
*/

void synchronizeData(SOCKET &s,pair<string,string> &user, unordered_map<string,file> &filefolder) {
	cout << user.first << " is synchronizing!" << endl;
	for (auto &k : filefolder) 										//发送本端所有文件校验信息给远端
		sendcheckcode(s, k.second);
	sendcheckcode(s, file("end", ""));

	while (1) {														//回应远端文件请求
		
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
* 处理客户端提交数据请求
*/

void commitData(SOCKET &s, pair<string, string> &user, unordered_map<string,file> &filefolder) {
	cout << user.first << " is commiting!" << endl;

	unordered_map<string, int> checkcode;						//接受远端校验信息存入字典
	while (1){
		auto temp=recvcheckcode(s);
		if (temp.first != "end")
			checkcode.insert(temp);
		else
			break;
	}

	for (auto i=filefolder.begin();i!=filefolder.end();) {			//根据check字典检查本地数据是否需要删除或更新，将需要更新的file校验信息发送给远端
		auto curcode = checkcode.find(i->first);
		if (curcode == checkcode.end())
			i = filefolder.erase(i);
		else if (i->second.getversion() == curcode->second) {
			checkcode.erase(curcode);
			++i;
		}
	}

	for (auto &k : checkcode) {									//根据字典中剩余的文件名向远端申请文件数据
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
* 发送验证码
*/
void sendcheckcode(SOCKET &s, file &f) {
	strcpy(dataBuf, f.getfileId().c_str());
	sendData(s, dataBuf, 100);
	memcpy(dataBuf, &f.getversion(), 4);
	dataBuf[4] = 0;
	sendData(s, dataBuf, 4);
}
/**
* 接受验证码
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
* 发送文件
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
* 接受文件
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