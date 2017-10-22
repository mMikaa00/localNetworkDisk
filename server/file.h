#pragma once
#include <string>
using namespace std;

class file {
public:
	file() = default;
	file(string a, string b) :fileId(a), path(b),version(0) {};
	const string& getfileId() const{
		return fileId;
	}

	const string& getpath() const{
		return path;
	}

	const int& getversion() const {
		return version;
	}

	void setfileId(string a) {
		fileId = a;
	}

	void setpath(string a) {
		path = a;
	}

	void setversion(int a) {
		version = a;
	}

private:
	string fileId; 
	string path;
	int version;
};