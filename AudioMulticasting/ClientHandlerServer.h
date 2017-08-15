#pragma once
#include "Common.h"
class ClientHandlerServer
{
public:
	SOCKET socket;
	int Id;
	char* msg;
	sockaddr_in Remaddr;
public:
	ClientHandlerServer();
	ClientHandlerServer(SOCKET sc, int id);
	void Run();
	~ClientHandlerServer();
};

