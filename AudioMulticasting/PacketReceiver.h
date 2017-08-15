#pragma once
#include "OutputDevice.h"

class PacketReceiver
{

private:
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	struct sockaddr_in Remaddr;
	struct sockaddr_in server;
	OutputDevice* outputDevice;
public:


	PacketReceiver();
	int Connect();
	int OpenUdpConnection(char* ip, int port);
	int OpenTcpConnection(char* ip, char* port);
	void tcpReceiver();
	void udpReceiver();
	void parse(long *ik, long *dk, BYTE* datak, BYTE* recvdata);
	~PacketReceiver();
};

