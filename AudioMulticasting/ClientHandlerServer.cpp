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
//void ClientHandlerServer::Run(){
//	char fresult[4000];
//	int iResult;
//	long *id = new long();
//
//	Storage* storage = Storage::getInstance();
//	while (true){
//		memset(fresult, 0, 4000);
//
//		iResult = recv(socket, fresult, 2048, 0);
//		if (iResult == SOCKET_ERROR) {
//			printf("ClientHandlerServer recv failed with error: %d\n", WSAGetLastError());
//			
//			shutdown(socket, 2);
//			closesocket(socket);
//		//	WSACleanup();
//			return;
//		}
//		if (iResult>0){
//			id = (long*)fresult;
//		}
//		 Packet* packet = storage->getAudioData(*id);
//		 char *data = "";
//		 int len = 1;
//		 if (packet != NULL){
//			 data =(char*)packet->toByte(&len);
//		 }
//		iResult = send(socket, data, len,0);
//		if (iResult == SOCKET_ERROR) {
//			printf("ClientHandlerServer send failed with error: %d\n", WSAGetLastError());
//			
//			shutdown(socket, 2);
//			closesocket(socket);
//			//WSACleanup();
//			return;
//		}
//	}
//}



void ClientHandlerServer::Run(){
	Storage* storage = Storage::getInstance();
	int b = storage->Buffer->writeIndex - 2;
	int PreviousPacketIndex = ( b<0)?BUFFER_LENGTH-2:b;
	int PreviouspacketId = -1;
	int NextPacketIndex = 0;
	int iResult;
	char fresult[20];
	

	while (true){
		memset(fresult, 0, 20);
		Packet packet = storage->Buffer->next(PreviousPacketIndex, PreviouspacketId, NextPacketIndex);
		if (packet.id == -1 || packet.rawData == (BYTE*)"") continue;
		iResult = recv(socket, fresult, 2048, 0);

		if (iResult == SOCKET_ERROR) {
			printf("receved failed with error: %d\n", WSAGetLastError());
			closesocket(socket);
			shutdown(socket, 2);
			//WSACleanup();
			return;
		}

	

		PreviousPacketIndex = NextPacketIndex;
		PreviouspacketId = packet.id;

		char *data = "";
		int len = 1;
		if (packet.id != -1){
			data = (char*)packet.toByte(&len);
		}
		iResult = send(socket, data, len, 0);
		if (iResult == SOCKET_ERROR) {
			printf("ClientHandlerServer send failed with error: %d\n", WSAGetLastError());

			shutdown(socket, 2);
			closesocket(socket);
			//WSACleanup();
			return;
		}
		delete data;
	}
}


ClientHandlerServer::~ClientHandlerServer()
{
}
