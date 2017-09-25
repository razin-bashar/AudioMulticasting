#include "stdafx.h"
#include "CircularArray.h"


CircularArray::CircularArray()
{
	
}

Packet CircularArray::next(int indexPosition, int Id, int &NextPacketIndex){
	Packet pp(-1, (BYTE*)"", 1);
	int nextIndex = nextPos(indexPosition, Id);
	if (nextIndex == indexPosition)return pp;
	else {

		NextPacketIndex = nextIndex;
		return CircularBuffer[nextIndex];

	}
}


int CircularArray::nextPos(int indexPosition, int Id){
	int i = 0;
	for (i = indexPosition + 1; i != indexPosition; i++){
		if (CircularBuffer[i%BUFFER_LENGTH].id > Id)return i%BUFFER_LENGTH;
	}
	return indexPosition;
}
void CircularArray::add(Packet packet){
	int index = (writeIndex++) % BUFFER_LENGTH;
	writeIndex %= BUFFER_LENGTH;
	CircularBuffer[index] = packet;
}
CircularArray::~CircularArray()
{
	
}
