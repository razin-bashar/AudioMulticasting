#include "stdafx.h"
#include "ClientHandlerServer.h"
#include "Storage.h"

ClientHandlerServer::ClientHandlerServer()
{
}

ClientHandlerServer::ClientHandlerServer(SOCKET sc, int id){
	socket = sc;
	Id = id;
}
void ClientHandlerServer::Run(){
	char fresult[4000];
	int iResult;
	long *id;

	Storage* storage = Storage::getInstance();
	while (true){
		memset(fresult, 0, 4000);

		iResult = recv(socket, fresult, 2048, 0);
		if (iResult>0){
			id = (long*)fresult;
		}

		char* data = (char*)storage->getAudioData(*id).toByte();
		iResult = sendto(socket, data, sizeof(id), 0, (sockaddr*)&Remaddr, sizeof(Remaddr));
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(socket);
			WSACleanup();
			return;
		}
	}
}
ClientHandlerServer::~ClientHandlerServer()
{
}
