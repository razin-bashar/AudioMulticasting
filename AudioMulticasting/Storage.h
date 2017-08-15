#pragma once
#include "Packet.h"
#include <queue>
#pragma warning(disable:4996)

class Storage
{
private:
	static bool instanceFlag;
	static Storage *single;
	Packet CapturedAudioData[10000];
	int size;
	Storage()
	{
		ip = getMyIp();
		size = 0;
	}

public:


	int iport;
	char* cport;
	char* ip;




	static Storage* getInstance();
	char* getMyIp() {
		WSADATA wsa_Data;
		int wsa_ReturnCode = WSAStartup(0x101, &wsa_Data);

		// Get the local hostname
		char szHostName[255];
		gethostname(szHostName, 255);
		struct hostent *host_entry;
		host_entry = gethostbyname(szHostName);
		char * szLocalIP;
		szLocalIP = inet_ntoa(*(struct in_addr *)*host_entry->h_addr_list);
		WSACleanup();
		return szLocalIP;
	}

	void addAudioData(Packet data);
	Packet getAudioData(int id);
	~Storage()
	{
		instanceFlag = false;

	}
};
