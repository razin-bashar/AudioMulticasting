#pragma once
#include "Common.h"

class PacketReceiver
{

private:
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	struct sockaddr_in Remaddr;
	struct sockaddr_in server;

public:


	PacketReceiver(char* ip, char* port);
	int Connect();
	int OpenUdpConnection(char* ip, int port);
	int OpenTcpConnection(char* ip, char* port);
	void tcpReceiver();
	void udpReceiver();
	void parse(long &ik, long &dk, BYTE* datak, BYTE* recvdata);
	~PacketReceiver();
};

