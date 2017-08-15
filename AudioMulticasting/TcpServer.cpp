#include "stdafx.h"
#include "ClientHandlerServer.h"
#include "TcpServer.h"
#include <thread>

TcpServer::TcpServer()
{
}

int TcpServer::Initialize(char* port){
	int iResult;



	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, port, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	return 0;
}

SOCKET TcpServer::WaitForConnect(){

	sockaddr* temp = NULL;
	ClientSocket = accept(ListenSocket, temp, 0);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return ClientSocket;
	}
	return ClientSocket;
	// No longer need server socket
	//closesocket(ListenSocket);
}

void TcpServer::Run(char* port) {
	printf("Server");


	Initialize(myport);

	int id = 0;
	while (true) {
		SOCKET sc = WaitForConnect();
		std::thread* x = new std::thread(&ClientHandlerServer::Run, ClientHandlerServer(sc, ++id));
		x->join();

	}
}
TcpServer::~TcpServer()
{
}