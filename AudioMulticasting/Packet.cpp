#include "stdafx.h"
#include "Packet.h"


Packet::Packet()
{
	id = -1;
	memset(rawData, 0, 4000);
	dataLength = 0;
	count = 0;
	time = 0;
	valid = true;
}
Packet::Packet(long Id, BYTE* data, long length){
	id = Id;
	memset(rawData, 0 , 4000);
	memcpy(rawData, data, length);
	dataLength = length;
	count = 0;
	time = 0;
	valid = true;
}
Packet::Packet(const Packet &p2){
	id = p2.id;
	memcpy(rawData, p2.rawData, p2.dataLength);
	dataLength = p2.dataLength;
	count = p2.count;
	time = p2.time;
	valid = p2.valid;
}
BYTE* Packet::toByte(int * len){
	int s0 = 1;//i
	int s1 = sizeof(long);
	int s2 = 1;//l
	int s3 = sizeof(long);
	int s4 = 1;//d
	int s5 = dataLength;


	int total = s0 + s1 + s2 + s3 + s4 + s5;

	*len = total;
	int i, j, k;
	BYTE* data = new BYTE[total];
	data[0] = 'i';

	
	BYTE* idb = (BYTE*)&id;
	BYTE a, b, c;
	for (i = 0; i < s1; i++){
		a = idb[i];
		data[i + 1] = a;
	}
	data[i + 1] = 'l';
	BYTE* ldb = (BYTE*)&s5;
	for (j = 0; j < s3; j++){
		b = ldb[j];
		data[(i + 1) + j + 1] = ldb[j];
	}
	data[(i + 1) + j + 1] = 'd';
	for (k = 0; k < s5; k++){
		c = rawData[k];
		data[(i + 1) + (j + 1) + k + 1] = rawData[k];
	}
	return data;
}

Packet::~Packet()
{
	
}
