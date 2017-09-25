#pragma once
#include "Common.h"
class Packet
{
public:
	long id;
	BYTE rawData[4000];
	long dataLength;
	int time;
	bool valid;
	int count;
	Packet();
	Packet(long Id, BYTE* data, long length);
	Packet(const Packet &p2);
	BYTE* toByte(int* len);
	~Packet();
};

