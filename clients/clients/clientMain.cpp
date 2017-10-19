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
	sendUserid();
	InitStdinThread();
	getinfo();
	system("pause");
	ExitClient();
}