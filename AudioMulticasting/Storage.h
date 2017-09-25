#pragma once
#include "Packet.h"
#include "CircularArray.h"
#include <queue>
#pragma warning(disable:4996)
class Storage
{
private:
	static bool instanceFlag;
	static Storage *single;

	
	int size;
	Storage()
	{
		ip = getMyIp();
		Buffer = new CircularArray();
		size = 0;
		lowbitrate = 0;
		
	}

public:


	int iport;
	char* cport;
	char* ip;

	CircularArray* Buffer;
	bool SampleRateint32 = true;
	int lowbitrate;

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


	~Storage()
	{
		instanceFlag = false;

	}
};
