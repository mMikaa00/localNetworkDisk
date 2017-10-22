#include "client.h"

void main() {
	InitMember();
	if (!InitSocket())
		return;

	if (ConnectServer())
		cout << "connection established!" << endl;
	else {
		cout << "connection failed!" << endl;
		//return;
	}
	if (checkUserid()) {
		InitStdinThread();
		getinfo();
		
	}
	else
		cout << "check userid failed!";
	system("pause");
	ExitClient();
}