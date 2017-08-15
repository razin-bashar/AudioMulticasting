#pragma once
#include "OutputDataFrame.h"
#include <queue>
#include "stdafx.h"
#include <audioclient.h>
class OutputDevice
{

private:
	static bool instanceFlag;
	static OutputDevice *single;
	std::queue<OutputDataFrame>* endBuffer;
	WAVEFORMATEXTENSIBLE wfe;
	HWAVEOUT hWaveOut;
	static void CALLBACK  VoicePlayNotification(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
	OutputDevice(){
		endBuffer = new std::queue<OutputDataFrame>();
		in = true;
	}
public:
	bool in;

	void play();
	void initialize(int nChannel, int sampleSize, int sampleRate);
	void UpdateBuffer(bool AddnotDelete, OutputDataFrame frame);
	static OutputDevice* getInstance();

	~OutputDevice(){
		instanceFlag = false;
		delete endBuffer;
	}
};

