#pragma once
#include "Packet.h"
#define BUFFER_LENGTH 10
class CircularArray
{

private:
	Packet CircularBuffer[BUFFER_LENGTH];
	
public:
	int writeIndex = 0;
	CircularArray();
	Packet next(int indexPosition, int Id,int &NextPacketIndex);
	int nextPos(int nextPosition, int Id);
	void add(Packet packet);
	~CircularArray();
};

