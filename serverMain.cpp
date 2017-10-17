#include "server.h"

void main() {
	InitMember();
	if (!InitSocket())
		return;
	createAcceptThread();
}