#pragma once
#include "Log.h"
#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <functiondiscoverykeys_devpkey.h>
class Config {
public:
	IMMDevice *m_pMMDevice;
	HMMIO m_hFile;
	bool m_bInt16;
	PWAVEFORMATEX m_pwfx;
	LPCWSTR m_szFilename;

	// set hr to S_FALSE to abort but return success
	void usage(LPCWSTR exe);
	HRESULT get_default_device(IMMDevice **ppMMDevice);
	HRESULT list_devices();
	HRESULT get_specific_device(LPCWSTR szLongName, IMMDevice **ppMMDevice);
	HRESULT open_file(LPCWSTR szFileName, HMMIO *phFile);
	Config(int argc, LPCWSTR argv[], HRESULT &hr);
	~Config();

};
