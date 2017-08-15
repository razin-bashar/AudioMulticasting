#include "stdafx.h"
#include "OutputDataFrame.h"


OutputDataFrame::OutputDataFrame(BYTE* assBuffer, long length)
{
	frame = new WAVEHDR();
	frame->lpData = (LPSTR)&assBuffer;
	frame->dwBufferLength = BUFFER_LENGTH;
	frame->dwBytesRecorded = length;
	frame->dwFlags = 0L;
	frame->dwLoops = 0L;
	frame->dwUser = 0L;
	memcpy(&this->assBuffer, assBuffer, length);
}


OutputDataFrame::~OutputDataFrame()
{
	delete frame;
}
