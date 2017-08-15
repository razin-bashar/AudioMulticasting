
#pragma once
#include "Common.h"
#include <stdlib.h>
#include <stdio.h>
class TcpServer
{
private:
	char* myport;
	WSADATA wsaData;
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct sockaddr_in Remaddr;
	struct sockaddr_in server;

	struct addrinfo *result = NULL;
	struct addrinfo hints;
public:
	TcpServer();
	int Initialize(char* port);
	SOCKET WaitForConnect();
	void Run(char* port);
	~TcpServer();
};

