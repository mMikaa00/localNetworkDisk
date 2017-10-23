#include "server.h"

void main() {
	server s;
	if (!s.InitSocket())
		return;
	s.createAcceptThread();
}