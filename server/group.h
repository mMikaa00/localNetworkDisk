#pragma once
#include <process.h>
#include "fileManager.h"

using namespace std;


class group {
public:
	group(string &a,string &p) :groupId(a),read_num(0) {			//使用固定路径完成file folder初始化
		InitializeCriticalSection(&cs);
		event1 = CreateEvent(NULL, TRUE, TRUE, NULL);
		event2 = CreateEvent(NULL, TRUE, TRUE, NULL);
		string path = p+ groupId + "\\*.*";
		initFileFolder(path.c_str(),fileFolder);
	};
	~group() {
		CloseHandle(event1);
		CloseHandle(event2);
	}
	
	const string& getgroupId() {
		return groupId;
	}

	bool adduser(string userId) {
		if (users.find(userId)!=users.end())
			return false;
		else
			users[userId] = -1;
		return true;
	}

	void setsocket(string userId, int s) {
		if (users.find(userId)==users.end())
			return;
		else
			users[userId] = s;
	}

	const unordered_map<string, int> &getusers() const{
		return users;
	}

	int finduser(string userId) const{
		auto ret = users.find(userId);
		if (ret != users.end())
			return ret->second;
		return 0;
	}

	unordered_map<string,file> &getfile() {
		return fileFolder;
	}

	HANDLE &getevent1() {
		return event1;
	}

	HANDLE &getevent2() {
		return event2;
	}

	CRITICAL_SECTION &getcs() {
		return cs;
	}

	int &getrdn() {
		return read_num;
	}

private:
	HANDLE event1;
	HANDLE event2;
	CRITICAL_SECTION cs;
	int read_num;
	string groupId;
	unordered_map<string, int> users;             //保存userId及连接后的socket，若无连接，则该值为-1
	unordered_map<string,file> fileFolder;
	
};

class userManager {
public:
	userManager(string &p) :path(p) {};
	bool adduser(pair<string,string> user) {
		auto g = gs.find(user.second);
		if (g != gs.end()) {
			return g->second->adduser(user.first);
		}
		else
		{
			auto n=gs.emplace(user.second, new group(user.second,path));
			return (n.first->second->adduser(user.first));
		}
	}

	group* getgroup(string groupId){
		auto g = gs.find(groupId);
		if (g != gs.end()) 
			return g->second;
		else
			return nullptr;
	}

	int finduser(pair<string, string> user) {
		auto g = getgroup(user.second);
		if (g != nullptr)
			return g->finduser(user.first);
		return 0;
	}

	unordered_map<string,group*>::iterator erasegroup(string groupid) {
		auto g = gs.find(groupid);
		if (g != gs.end()) {
			delete g->second;
			return gs.erase(g);
		}
		else
			return g;
	}

	
private:
	unordered_map<string,group*> gs;
	string &path;			//传入服务器对应文件夹路径
};


