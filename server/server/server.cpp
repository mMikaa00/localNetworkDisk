#include "server.h"

userManager sUsers;
char sDataBuff[MAX_BUFF];
HANDLE sAcceptThread;
CRITICAL_SECTION cs;
SOCKET sServer;
bool sConfig;

void InitMember() {
	{
		InitializeCriticalSection(&cs);                         //初始化临界区  
		memset(sDataBuff, 0, MAX_BUFF);
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

	cout << "createThread succeed"  << endl;
	pair<string,string> curuser;
	if(!getuserInfo(accept,curuser))				//获取用户信息
		return 0;
	EnterCriticalSection(&cs);
	if (!sUsers.finduser(curuser)) {					//检查数据库中是否已存在该用户，若不存在则添加该用户
		if (sUsers.adduser(curuser))
			cout << "add new user to database" << endl;
		else
			cout << "add failed!" << endl;
	}

	if (sUsers.getgroup(curuser.second)->finduser(curuser.first)!=-1)				//检查该用户是否已连接
	{	
		cout << "other user had connected using this id..." << endl;
	}
	else {
		sUsers.getgroup(curuser.second)->setsocket(curuser.first, accept);			//将group中该user配对值设为socket值，标识该用户已连接
		
	}

	LeaveCriticalSection(&cs);
	cout << curuser.first << ' ' << curuser.second << endl;

	cout << "client " << curuser.first << " disconnected!" << endl;
	
	sUsers.getgroup(curuser.second)->setsocket(curuser.first, -1);
	closesocket(accept);
	delete pParam;
	return 0;//线程退出  
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

		if (nlength == 0)
			break;
	}

	return TRUE;        //发送成功  
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
