#include "client.h"

void main() {
	InitMember();
	if (!InitSocket())
		return;
	while (true)
	{
		try {
			while (1) {
				if (ConnectServer()) {
					cout << "connection established!" << endl;
					break;
				}
				else {
					cout << "connection failed!" << endl;
					Sleep(3000);
				}
			}
			getUserInfo();
			if (checkUserid()) {
				InitStdinThread();
				getinfo();
			}
			else {
				cout << "check userid failed!";
				break;
			}
		}
		catch (const std::exception& k) {
			cout << "Server failed, try to connect every 3s" << endl;
			resetSocket();
		}
	}
	system("pause");
	ExitClient();
}