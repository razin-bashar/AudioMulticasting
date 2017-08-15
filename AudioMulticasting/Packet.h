#pragma once
#include "Common.h"
class Packet
{
public:
	long id;
	BYTE* rawData;
	long dataLength;
	int time;
	bool valid;
	int count;
	Packet();
	Packet(long Id, BYTE* data, long length);
	BYTE* toByte();
	~Packet();
};

