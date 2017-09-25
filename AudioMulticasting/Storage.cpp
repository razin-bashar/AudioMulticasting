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

