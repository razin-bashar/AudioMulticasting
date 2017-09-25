
#pragma once
#include "Common.h"
#include <stdlib.h>
#include <stdio.h>
class TcpServer
{
private:
	char* port;
	WSADATA wsaData;
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct sockaddr_in Remaddr;
	struct sockaddr_in server;

	struct addrinfo *result = NULL;
	struct addrinfo hints;
public:
	TcpServer(char* pport);
	int Initialize();
	SOCKET WaitForConnect();
	void Run();
	~TcpServer();
};

