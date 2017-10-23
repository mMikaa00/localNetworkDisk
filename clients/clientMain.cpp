#include "client.h"

void main() {
	client c;
	if (!c.InitSocket())
		return;
	while (true)
	{
		try {
			while (1) {
				if (c.ConnectServer()) {
					cout << "connection established!" << endl;
					break;
				}
				else {
					cout << "connection failed!" << endl;
					Sleep(3000);
				}
			}
			c.getUserInfo();
			if (c.checkUserid()) {
				c.InitStdinThread();
				c.getinfo();
			}
			else {
				cout << "check userid failed!";
				break;
			}
		}
		catch (const std::exception& k) {
			cout << "Server failed, try to connect every 3s" << endl;
			c.resetSocket();
		}
	}
	system("pause");
}