#include "stdafx.h"
#include "Packet.h"


Packet::Packet()
{

}
Packet::Packet(long Id, BYTE* data, long length){
	id = Id;
	rawData = new BYTE[length];
	memcpy(rawData, data, length);
	dataLength = length;
	count = 0;
	time = 0;
	valid = true;
}

BYTE* Packet::toByte(){
	int s0 = 1;//i
	int s1 = sizeof(long);
	int s2 = 1;//l
	int s3 = sizeof(long);
	int s4 = 1;//d
	int s5 = dataLength;

	int total = s0 + s1 + s2 + s3 + s4 + s5;
	int i, j, k;
	BYTE* data = new BYTE[total];
	data[0] = 'i';
	BYTE* idb = (BYTE*)id;
	for (i = 0; i < s1; i++){
		data[i + 1] = idb[i];
	}
	data[i + 1] = 'l';
	BYTE* ldb = (BYTE*)dataLength;
	for (j = 0; i < s3; j++){
		data[(i + 1) + j + 1] = ldb[j];
	}
	data[(i + 1) + j + 1] = 'd';
	for (k = 0; k < s5; k++){
		data[(i + 1) + (j + 1) + k + 1] = rawData[k];
	}
	return data;
}

Packet::~Packet()
{
	delete rawData;
}
