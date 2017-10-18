#pragma once
#include <string>
using namespace std;

class file {
public:
	file() = default;
	file(string a, string b) :fileId(a), content(b),version(15) {};
	const string& getfileId() const{
		return fileId;
	}

	const string& getcontent() const{
		return content;
	}

	const int& getversion() const {
		return version;
	}

	void setfileId(string a) {
		fileId = a;
	}

	void setcontent(string a) {
		content = a;
	}

	void setversion(int a) {
		version = a;
	}

private:
	string fileId;
	string content;
	int version;
};