#pragma once
#include <string>
#include <unordered_map>
#include <Windows.h>
using namespace std;

class file {
public:
	file() = default;
	file(string a, string b,int c) :fileId(a), path(b),size(c),version(0) {};
	const string& getfileId() const{
		return fileId;
	}

	const string& getpath() const{
		return path;
	}

	const int& getversion() const {
		return version;
	}

	const int& getsize() const {
		return size;
	}

	void setfileId(string a) {
		fileId = a;
	}

	void setpath(string a) {
		path = a;
	}

	void setsize(int a) {
		size = a;
	}

	void setversion(int a) {
		version = a;
	}

private:
	string fileId; 
	string path;
	int version;
	int size;
};

void initFileFolder(const char *, unordered_map<string, file> &);