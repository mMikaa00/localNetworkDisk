#pragma once
#include <string>
#include <unordered_map>
#include <set>
#include <algorithm>
using namespace std;

/*class user {
public:
	user() = default;
	user(string a, string b) :userId(a), groupId(b) {};
	string getUserId() {
		return userId;
	}
	string getGroupId() {
		return groupId;
	}
private:
	string userId;
	string groupId;
};*/

class file {
public:
	file() = default;
	file(string a,string b) :fileId(a),content(b) {};
	string getfileId() {
		return fileId;
	}
	
	string getcontent() {
		return content;
	}

	void setfileId(string a) {
		fileId = a;
	}

	void setcontent(string a) {
		content = a;
	}

private:
	string fileId;
	string content;
};

class group {
public:
	group() = default;
	group(string a) :groupId(a) {};
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

	vector<file> &getfile() {
		return fileFolder;
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

private:
	string groupId;
	unordered_map<string, int> users;             //保存userId及连接后的socket，若无连接，则该值为-1
	vector<file> fileFolder;
};

class userManager {
public:
	bool adduser(pair<string,string> user) {
		auto g = gs.find(user.second);
		if (g != gs.end()) {
			return g->second.adduser(user.first);
		}
		else
		{
			auto n=gs.emplace(user.second, user.second);
			return (n.first->second.adduser(user.first));
		}
		
	}

	group* getgroup(string groupId){
		auto g = gs.find(groupId);
		if (g != gs.end()) 
			return &g->second;
		else
			return nullptr;
	}

	int finduser(pair<string, string> user) {
		auto g = getgroup(user.second);
		if (g != nullptr)
			return g->finduser(user.first);
		return 0;
	}

	vector<file> &getfile(string groupId)  {
		
		return (getgroup(groupId)->getfile());
	}
	
private:
	unordered_map<string,group> gs;
};


