#include "stdafx.h"
#include "Storage.h"

#include <iostream>





bool Storage::instanceFlag = false;
Storage* Storage::single = NULL;

Storage* Storage::getInstance()
{
	if (!instanceFlag)
	{
		single = new Storage();
		instanceFlag = true;
		return single;
	}
	else
	{
		return single;
	}
}


void Storage::addAudioData(Packet data){
	if (size == 1000)size = 0;
	if (CapturedAudioData[size].valid == false)CapturedAudioData[size++] = data;
}
Packet Storage::getAudioData(int id){

	Packet pk = CapturedAudioData[id];
	int firstvalid = 0;
	bool first = true;
	if (pk.valid == true){
		pk.count++;
		return pk;
	}
	else{
		for (int i = id + 1; i < 1000; i++){
			if (CapturedAudioData[i].valid){
				if (first){
					firstvalid = i;
					first = false;
				}
				CapturedAudioData[i].count++;
				if (CapturedAudioData[i].count >= 16)CapturedAudioData[i].valid = false;
				return CapturedAudioData[i];
			}
		}
	}
	CapturedAudioData[firstvalid].count++;
	if (CapturedAudioData[firstvalid].count >= 16)CapturedAudioData[firstvalid].valid = false;
	return CapturedAudioData[firstvalid];
}
