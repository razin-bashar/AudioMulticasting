#pragma once
#include "Common.h"

#define BUFFER_LENGTH 3000
class OutputDataFrame
{
public:
	WAVEHDR* frame;
	BYTE assBuffer[BUFFER_LENGTH];

	OutputDataFrame(BYTE* assBuffer, long length);
	~OutputDataFrame();
};

