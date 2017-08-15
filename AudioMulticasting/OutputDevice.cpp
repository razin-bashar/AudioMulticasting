#include "stdafx.h"
#include "OutputDevice.h"


bool OutputDevice::instanceFlag = false;
OutputDevice* OutputDevice::single = NULL;

void CALLBACK  OutputDevice::VoicePlayNotification(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2){

	try {
		OutputDevice* data = reinterpret_cast<OutputDevice*>(dwInstance);


		switch (uMsg)
		{
		case WOM_DONE:
		{
						 data->in = true;
						 // OutputDataFrame frame((BYTE*)"",0);
						 //data->UpdateBuffer(false,frame);

		}
			break;
		case WOM_CLOSE:
			printf("voicePlayNotification with WIM_CLOSE");
			break;
		case WOM_OPEN:
			printf("voicePlayNotification with WIM_OPEN");
			break;
		}
	}
	catch (int e) {
		printf("..........................................");
		return;
	}
	//Logger("VoiceRecordNotification exit");
}

void OutputDevice::play(){
	printf("\nPlayer\n");
	while (true)
	{
		if (!endBuffer->empty() && in == true){
			in = false;
			WAVEHDR* frame = endBuffer->front().frame;
			waveOutPrepareHeader(hWaveOut, frame, sizeof(WAVEHDR));
			waveOutWrite(hWaveOut, frame, sizeof(frame));
		}
	}
}

void OutputDevice::initialize(int nChannel, int sampleSize, int sampleRate){

	wfe.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	wfe.Format.nChannels = nChannel;
	wfe.Format.wBitsPerSample = sampleSize;
	wfe.Format.nSamplesPerSec = sampleRate;
	wfe.Format.nBlockAlign = (wfe.Format.nChannels*wfe.Format.wBitsPerSample) / 8;
	wfe.Format.nAvgBytesPerSec = wfe.Format.nSamplesPerSec*wfe.Format.nBlockAlign;
	wfe.Format.cbSize = sizeof(wfe)-sizeof(WAVEFORMATEX);
	wfe.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
	wfe.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
	wfe.Samples.wValidBitsPerSample = sampleSize;

	HRESULT result = waveOutOpen(&hWaveOut, WAVE_MAPPER, (LPWAVEFORMATEX)&wfe, 0L, 0L, WAVE_FORMAT_DIRECT);
	result = waveOutOpen(&hWaveOut, WAVE_MAPPER, (LPWAVEFORMATEX)&wfe, (DWORD_PTR)OutputDevice::VoicePlayNotification, (DWORD)this, CALLBACK_FUNCTION);

}
void OutputDevice::UpdateBuffer(bool AddnotDelete, OutputDataFrame frame){
	if (AddnotDelete){
		endBuffer->push(frame);
	}
	else{
		if (endBuffer->empty())return;
		endBuffer->pop();
	}
}

OutputDevice* OutputDevice::getInstance()
{
	if (!instanceFlag)
	{
		single = new OutputDevice();
		instanceFlag = true;
		return single;
	}
	else
	{
		return single;
	}
}


