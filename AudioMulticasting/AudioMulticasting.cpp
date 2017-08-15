// AudioMulticasting.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Common.h"
#include <initguid.h>
#include <stdio.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>

#include <stdlib.h>  
#include <audioclient.h>
#include <avrt.h>
#include <functiondiscoverykeys_devpkey.h>
#include <thread>
#define DEFAULT_FILE L"loopback-capture.wav"
#define LOG(format, ...) wprintf(format L"\n", __VA_ARGS__)
#define ERR(format, ...) LOG(L"Error: " format, __VA_ARGS__)
#define BUFSIZE 20000

void usage(LPCWSTR exe);
HRESULT get_default_device(IMMDevice **ppMMDevice);
HRESULT list_devices();
HRESULT get_specific_device(LPCWSTR szLongName, IMMDevice **ppMMDevice);
HRESULT open_file(LPCWSTR szFileName, HMMIO *phFile);
int do_everything(int argc, LPCWSTR argv[]);
DWORD WINAPI LoopbackCaptureThreadFunction(LPVOID pContext);
HRESULT WriteWaveHeader(HMMIO hFile, LPCWAVEFORMATEX pwfx, MMCKINFO *pckRIFF, MMCKINFO *pckData);
HRESULT FinishWaveFile(HMMIO hFile, MMCKINFO *pckRIFF, MMCKINFO *pckData);
HRESULT LoopbackCapture(
	IMMDevice *pMMDevice,
	HMMIO hFile,
	bool bInt16,
	HANDLE hStartedEvent,
	HANDLE hStopEvent,
	PUINT32 pnFrames
	);
HRESULT InitiateOutputDevice();
void PlayRecord();

WAVEFORMATEX pFormat;
UINT32 sampleRate = 48000;
char waveIn[BUFSIZE];
WAVEHDR WaveHdr;
HWAVEOUT hWaveOut;


class CPrefs {
public:
	IMMDevice *m_pMMDevice;
	HMMIO m_hFile;
	bool m_bInt16;
	PWAVEFORMATEX m_pwfx;
	LPCWSTR m_szFilename;

	// set hr to S_FALSE to abort but return success
	CPrefs(int argc, LPCWSTR argv[], HRESULT &hr);
	~CPrefs();

};
// cleanup.h
#pragma region clean
class AudioClientStopOnExit {
public:
	AudioClientStopOnExit(IAudioClient *p) : m_p(p) {}
	~AudioClientStopOnExit() {
		HRESULT hr = m_p->Stop();
		if (FAILED(hr)) {
			ERR(L"IAudioClient::Stop failed: hr = 0x%08x", hr);
		}
	}

private:
	IAudioClient *m_p;
};

class AvRevertMmThreadCharacteristicsOnExit {
public:
	AvRevertMmThreadCharacteristicsOnExit(HANDLE hTask) : m_hTask(hTask) {}
	~AvRevertMmThreadCharacteristicsOnExit() {
		if (!AvRevertMmThreadCharacteristics(m_hTask)) {
			ERR(L"AvRevertMmThreadCharacteristics failed: last error is %d", GetLastError());
		}
	}
private:
	HANDLE m_hTask;
};

class CancelWaitableTimerOnExit {
public:
	CancelWaitableTimerOnExit(HANDLE h) : m_h(h) {}
	~CancelWaitableTimerOnExit() {
		if (!CancelWaitableTimer(m_h)) {
			ERR(L"CancelWaitableTimer failed: last error is %d", GetLastError());
		}
	}
private:
	HANDLE m_h;
};

class CloseHandleOnExit {
public:
	CloseHandleOnExit(HANDLE h) : m_h(h) {}
	~CloseHandleOnExit() {
		if (!CloseHandle(m_h)) {
			ERR(L"CloseHandle failed: last error is %d", GetLastError());
		}
	}

private:
	HANDLE m_h;
};

class CoTaskMemFreeOnExit {
public:
	CoTaskMemFreeOnExit(PVOID p) : m_p(p) {}
	~CoTaskMemFreeOnExit() {
		CoTaskMemFree(m_p);
	}

private:
	PVOID m_p;
};

class CoUninitializeOnExit {
public:
	~CoUninitializeOnExit() {
		CoUninitialize();
	}
};

class PropVariantClearOnExit {
public:
	PropVariantClearOnExit(PROPVARIANT *p) : m_p(p) {}
	~PropVariantClearOnExit() {
		HRESULT hr = PropVariantClear(m_p);
		if (FAILED(hr)) {
			ERR(L"PropVariantClear failed: hr = 0x%08x", hr);
		}
	}

private:
	PROPVARIANT *m_p;
};

class ReleaseOnExit {
public:
	ReleaseOnExit(IUnknown *p) : m_p(p) {}
	~ReleaseOnExit() {
		m_p->Release();
	}

private:
	IUnknown *m_p;
};

class SetEventOnExit {
public:
	SetEventOnExit(HANDLE h) : m_h(h) {}
	~SetEventOnExit() {
		if (!SetEvent(m_h)) {
			ERR(L"SetEvent failed: last error is %d", GetLastError());
		}
	}
private:
	HANDLE m_h;
};

class WaitForSingleObjectOnExit {
public:
	WaitForSingleObjectOnExit(HANDLE h) : m_h(h) {}
	~WaitForSingleObjectOnExit() {
		DWORD dwWaitResult = WaitForSingleObject(m_h, INFINITE);
		if (WAIT_OBJECT_0 != dwWaitResult) {
			ERR(L"WaitForSingleObject returned unexpected result 0x%08x, last error is %d", dwWaitResult, GetLastError());
		}
	}

private:
	HANDLE m_h;
};

struct LoopbackCaptureThreadFunctionArguments {
	IMMDevice *pMMDevice;
	bool bInt16;
	HMMIO hFile;
	HANDLE hStartedEvent;
	HANDLE hStopEvent;
	UINT32 nFrames;
	HRESULT hr;
};

#pragma endregion

int _cdecl wmain(int argc, LPCWSTR argv[])
{

	//std::thread server(&IndependentServer::UdpRun, IndependentServer());
	//	std::thread client(&IndependentClient::UdpRun, IndependentClient());
	/*Sleep(10000);
	AudioChatRoomController AR1;
	AR1.add("192.168.29.121", "27060");*/




	HRESULT hr = S_OK;
	LPCWSTR argv1[4];
	argv1[1] = L"--device";
	argv1[2] = L"Speakers (Realtek High Definition Audio)";
	argv1[3] = L"-int-16";


	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		ERR(L"CoInitialize failed: hr = 0x%08x", hr);
		return -__LINE__;
	}
	CoUninitializeOnExit cuoe;

	do_everything(3, argv1);

	//server.join();
	//	client.join();

}
int do_everything(int argc, LPCWSTR argv[]) {

	HRESULT hr = S_OK;

	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		ERR(L"CoInitialize failed: hr = 0x%08x", hr);
		return -__LINE__;
	}
	CoUninitializeOnExit cuoe;


	// parse command line
	CPrefs prefs(argc, argv, hr);
	if (FAILED(hr)) {
		ERR(L"CPrefs::CPrefs constructor failed: hr = 0x%08x", hr);
		return -__LINE__;
	}
	if (S_FALSE == hr) {
		// nothing to do
		return 0;
	}

	// create a "loopback capture has started" event
	HANDLE hStartedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == hStartedEvent) {
		ERR(L"CreateEvent failed: last error is %u", GetLastError());
		return -__LINE__;
	}
	CloseHandleOnExit closeStartedEvent(hStartedEvent);

	// create a "stop capturing now" event
	HANDLE hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == hStopEvent) {
		ERR(L"CreateEvent failed: last error is %u", GetLastError());
		return -__LINE__;
	}
	CloseHandleOnExit closeStopEvent(hStopEvent);

	// create arguments for loopback capture thread
	LoopbackCaptureThreadFunctionArguments threadArgs;
	threadArgs.hr = E_UNEXPECTED; // thread will overwrite this
	threadArgs.pMMDevice = prefs.m_pMMDevice;
	threadArgs.bInt16 = prefs.m_bInt16;
	threadArgs.hFile = prefs.m_hFile;
	threadArgs.hStartedEvent = hStartedEvent;
	threadArgs.hStopEvent = hStopEvent;
	threadArgs.nFrames = 0;

	HANDLE hThread = CreateThread(
		NULL, 0,
		LoopbackCaptureThreadFunction, &threadArgs,
		0, NULL
		);
	if (NULL == hThread) {
		ERR(L"CreateThread failed: last error is %u", GetLastError());
		return -__LINE__;
	}
	CloseHandleOnExit closeThread(hThread);

	// wait for either capture to start or the thread to end
	HANDLE waitArray[2] = { hStartedEvent, hThread };
	DWORD dwWaitResult;
	dwWaitResult = WaitForMultipleObjects(
		ARRAYSIZE(waitArray), waitArray,
		FALSE, INFINITE
		);

	if (WAIT_OBJECT_0 + 1 == dwWaitResult) {
		ERR(L"Thread aborted before starting to loopback capture: hr = 0x%08x", threadArgs.hr);
		return -__LINE__;
	}

	if (WAIT_OBJECT_0 != dwWaitResult) {
		ERR(L"Unexpected WaitForMultipleObjects return value %u", dwWaitResult);
		return -__LINE__;
	}

	// at this point capture is running
	// wait for the user to press a key or for capture to error out
	{
		WaitForSingleObjectOnExit waitForThread(hThread);
		SetEventOnExit setStopEvent(hStopEvent);
		HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

		if (INVALID_HANDLE_VALUE == hStdIn) {
			ERR(L"GetStdHandle returned INVALID_HANDLE_VALUE: last error is %u", GetLastError());
			return -__LINE__;
		}

		LOG(L"%s", L"Press Enter to quit...");

		HANDLE rhHandles[2] = { hThread, hStdIn };

		bool bKeepWaiting = true;
		while (bKeepWaiting) {

			dwWaitResult = WaitForMultipleObjects(2, rhHandles, FALSE, INFINITE);

			switch (dwWaitResult) {

			case WAIT_OBJECT_0: // hThread
				ERR(L"%s", L"The thread terminated early - something bad happened");
				bKeepWaiting = false;
				break;

			case WAIT_OBJECT_0 + 1: // hStdIn
				// see if any of them was an Enter key-up event
				INPUT_RECORD rInput[128];
				DWORD nEvents;
				if (!ReadConsoleInput(hStdIn, rInput, ARRAYSIZE(rInput), &nEvents)) {
					ERR(L"ReadConsoleInput failed: last error is %u", GetLastError());
					bKeepWaiting = false;
				}
				else {
					for (DWORD i = 0; i < nEvents; i++) {
						if (
							KEY_EVENT == rInput[i].EventType &&
							VK_RETURN == rInput[i].Event.KeyEvent.wVirtualKeyCode &&
							!rInput[i].Event.KeyEvent.bKeyDown
							) {
							LOG(L"%s", L"Stopping capture...");
							bKeepWaiting = false;
							break;
						}
					}
					// if none of them were Enter key-up events,
					// continue waiting
				}
				break;

			default:
				ERR(L"WaitForMultipleObjects returned unexpected value 0x%08x", dwWaitResult);
				bKeepWaiting = false;
				break;
			} // switch
		} // while
	} // naked scope

	// at this point the thread is definitely finished

	DWORD exitCode;
	if (!GetExitCodeThread(hThread, &exitCode)) {
		ERR(L"GetExitCodeThread failed: last error is %u", GetLastError());
		return -__LINE__;
	}

	if (0 != exitCode) {
		ERR(L"Loopback capture thread exit code is %u; expected 0", exitCode);
		return -__LINE__;
	}

	if (S_OK != threadArgs.hr) {
		ERR(L"Thread HRESULT is 0x%08x", threadArgs.hr);
		return -__LINE__;
	}

	// everything went well... fixup the fact chunk in the file
	MMRESULT result = mmioClose(prefs.m_hFile, 0);
	prefs.m_hFile = NULL;
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioClose failed: MMSYSERR = %u", result);
		return -__LINE__;
	}

	// reopen the file in read/write mode
	MMIOINFO mi = { 0 };
	prefs.m_hFile = mmioOpen(const_cast<LPWSTR>(prefs.m_szFilename), &mi, MMIO_READWRITE);
	if (NULL == prefs.m_hFile) {
		ERR(L"mmioOpen(\"%ls\", ...) failed. wErrorRet == %u", prefs.m_szFilename, mi.wErrorRet);
		return -__LINE__;
	}

	// descend into the RIFF/WAVE chunk
	MMCKINFO ckRIFF = { 0 };
	ckRIFF.ckid = MAKEFOURCC('W', 'A', 'V', 'E'); // this is right for mmioDescend
	result = mmioDescend(prefs.m_hFile, &ckRIFF, NULL, MMIO_FINDRIFF);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioDescend(\"WAVE\") failed: MMSYSERR = %u", result);
		return -__LINE__;
	}

	// descend into the fact chunk
	MMCKINFO ckFact = { 0 };
	ckFact.ckid = MAKEFOURCC('f', 'a', 'c', 't');
	result = mmioDescend(prefs.m_hFile, &ckFact, &ckRIFF, MMIO_FINDCHUNK);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioDescend(\"fact\") failed: MMSYSERR = %u", result);
		return -__LINE__;
	}

	// write the correct data to the fact chunk
	LONG lBytesWritten = mmioWrite(
		prefs.m_hFile,
		reinterpret_cast<PCHAR>(&threadArgs.nFrames),
		sizeof(threadArgs.nFrames)
		);
	if (lBytesWritten != sizeof(threadArgs.nFrames)) {
		ERR(L"Updating the fact chunk wrote %u bytes; expected %u", lBytesWritten, (UINT32)sizeof(threadArgs.nFrames));
		return -__LINE__;
	}

	// ascend out of the fact chunk
	result = mmioAscend(prefs.m_hFile, &ckFact, 0);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioAscend(\"fact\") failed: MMSYSERR = %u", result);
		return -__LINE__;
	}

	// let prefs' destructor call mmioClose

	return 0;
}
void usage(LPCWSTR exe) {
	LOG(
		L"%ls -?\n"
		L"%ls --list-devices\n"
		L"%ls [--device \"Device long name\"] [--file \"file name\"] [--int-16]\n"
		L"\n"
		L"    -? prints this message.\n"
		L"    --list-devices displays the long names of all active playback devices.\n"
		L"    --device captures from the specified device (default if omitted)\n"
		L"    --file saves the output to a file (%ls if omitted)\n"
		L"    --int-16 attempts to coerce data to 16-bit integer format",
		exe, exe, exe, DEFAULT_FILE
		);
}

CPrefs::CPrefs(int argc, LPCWSTR argv[], HRESULT &hr)
: m_pMMDevice(NULL)
, m_hFile(NULL)
, m_bInt16(false)
, m_pwfx(NULL)
, m_szFilename(NULL)
{
	switch (argc) {
	case 2:
		if (0 == _wcsicmp(argv[1], L"-?") || 0 == _wcsicmp(argv[1], L"/?")) {
			// print usage but don't actually capture
			hr = S_FALSE;
			usage(argv[0]);
			return;
		}
		else if (0 == _wcsicmp(argv[1], L"--list-devices")) {
			// list the devices but don't actually capture
			hr = list_devices();

			// don't actually play
			if (S_OK == hr) {
				hr = S_FALSE;
				return;
			}
		}
		// intentional fallthrough

	default:
		// loop through arguments and parse them
		for (int i = 1; i < argc; i++) {

			// --device
			if (0 == _wcsicmp(argv[i], L"--device")) {
				if (NULL != m_pMMDevice) {
					ERR(L"%s", L"Only one --device switch is allowed");
					hr = E_INVALIDARG;
					return;
				}

				if (i++ == argc) {
					ERR(L"%s", L"--device switch requires an argument");
					hr = E_INVALIDARG;
					return;
				}

				hr = get_specific_device(argv[i], &m_pMMDevice);
				if (FAILED(hr)) {
					return;
				}

				continue;
			}

			// --file
			if (0 == _wcsicmp(argv[i], L"--file")) {
				if (NULL != m_szFilename) {
					ERR(L"%s", L"Only one --file switch is allowed");
					hr = E_INVALIDARG;
					return;
				}

				if (i++ == argc) {
					ERR(L"%s", L"--file switch requires an argument");
					hr = E_INVALIDARG;
					return;
				}

				m_szFilename = argv[i];
				continue;
			}

			// --int-16
			if (0 == _wcsicmp(argv[i], L"--int-16")) {
				if (m_bInt16) {
					ERR(L"%s", L"Only one --int-16 switch is allowed");
					hr = E_INVALIDARG;
					return;
				}

				m_bInt16 = true;
				continue;
			}

			ERR(L"Invalid argument %ls", argv[i]);
			hr = E_INVALIDARG;
			return;
		}

		// open default device if not specified
		if (NULL == m_pMMDevice) {
			hr = get_default_device(&m_pMMDevice);
			if (FAILED(hr)) {
				return;
			}
		}

		// if no filename specified, use default
		if (NULL == m_szFilename) {
			m_szFilename = DEFAULT_FILE;
		}

		// open file
		hr = open_file(m_szFilename, &m_hFile);
		if (FAILED(hr)) {
			return;
		}
	}
}

CPrefs::~CPrefs() {
	if (NULL != m_pMMDevice) {
		m_pMMDevice->Release();
	}

	if (NULL != m_hFile) {
		mmioClose(m_hFile, 0);
	}

	if (NULL != m_pwfx) {
		CoTaskMemFree(m_pwfx);
	}
}

HRESULT get_default_device(IMMDevice **ppMMDevice) {
	HRESULT hr = S_OK;
	IMMDeviceEnumerator *pMMDeviceEnumerator;

	// activate a device enumerator
	hr = CoCreateInstance(
		__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		(void**)&pMMDeviceEnumerator
		);
	if (FAILED(hr)) {
		ERR(L"CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x", hr);
		return hr;
	}
	ReleaseOnExit releaseMMDeviceEnumerator(pMMDeviceEnumerator);

	// get the default render endpoint
	hr = pMMDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, ppMMDevice);
	if (FAILED(hr)) {
		ERR(L"IMMDeviceEnumerator::GetDefaultAudioEndpoint failed: hr = 0x%08x", hr);
		return hr;
	}

	return S_OK;
}

HRESULT list_devices() {
	HRESULT hr = S_OK;

	// get an enumerator
	IMMDeviceEnumerator *pMMDeviceEnumerator;

	hr = CoCreateInstance(
		__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		(void**)&pMMDeviceEnumerator
		);
	if (FAILED(hr)) {
		ERR(L"CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x", hr);
		return hr;
	}
	ReleaseOnExit releaseMMDeviceEnumerator(pMMDeviceEnumerator);

	IMMDeviceCollection *pMMDeviceCollection;

	// get all the active render endpoints
	hr = pMMDeviceEnumerator->EnumAudioEndpoints(
		eRender, DEVICE_STATE_ACTIVE, &pMMDeviceCollection
		);
	if (FAILED(hr)) {
		ERR(L"IMMDeviceEnumerator::EnumAudioEndpoints failed: hr = 0x%08x", hr);
		return hr;
	}
	ReleaseOnExit releaseMMDeviceCollection(pMMDeviceCollection);

	UINT count;
	hr = pMMDeviceCollection->GetCount(&count);
	if (FAILED(hr)) {
		ERR(L"IMMDeviceCollection::GetCount failed: hr = 0x%08x", hr);
		return hr;
	}
	LOG(L"Active render endpoints found: %u", count);

	for (UINT i = 0; i < count; i++) {
		IMMDevice *pMMDevice;

		// get the "n"th device
		hr = pMMDeviceCollection->Item(i, &pMMDevice);
		if (FAILED(hr)) {
			ERR(L"IMMDeviceCollection::Item failed: hr = 0x%08x", hr);
			return hr;
		}
		ReleaseOnExit releaseMMDevice(pMMDevice);

		// open the property store on that device
		IPropertyStore *pPropertyStore;
		hr = pMMDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
		if (FAILED(hr)) {
			ERR(L"IMMDevice::OpenPropertyStore failed: hr = 0x%08x", hr);
			return hr;
		}
		ReleaseOnExit releasePropertyStore(pPropertyStore);

		// get the long name property
		PROPVARIANT pv; PropVariantInit(&pv);
		hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
		if (FAILED(hr)) {
			ERR(L"IPropertyStore::GetValue failed: hr = 0x%08x", hr);
			return hr;
		}
		PropVariantClearOnExit clearPv(&pv);

		if (VT_LPWSTR != pv.vt) {
			ERR(L"PKEY_Device_FriendlyName variant type is %u - expected VT_LPWSTR", pv.vt);
			return E_UNEXPECTED;
		}

		LOG(L"    %ls", pv.pwszVal);
	}

	return S_OK;
}

HRESULT get_specific_device(LPCWSTR szLongName, IMMDevice **ppMMDevice) {
	HRESULT hr = S_OK;

	*ppMMDevice = NULL;

	// get an enumerator
	IMMDeviceEnumerator *pMMDeviceEnumerator;

	hr = CoCreateInstance(
		__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		(void**)&pMMDeviceEnumerator
		);
	if (FAILED(hr)) {
		ERR(L"CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x", hr);
		return hr;
	}
	ReleaseOnExit releaseMMDeviceEnumerator(pMMDeviceEnumerator);

	IMMDeviceCollection *pMMDeviceCollection;

	// get all the active render endpoints
	hr = pMMDeviceEnumerator->EnumAudioEndpoints(
		eRender, DEVICE_STATE_ACTIVE, &pMMDeviceCollection
		);
	if (FAILED(hr)) {
		ERR(L"IMMDeviceEnumerator::EnumAudioEndpoints failed: hr = 0x%08x", hr);
		return hr;
	}
	ReleaseOnExit releaseMMDeviceCollection(pMMDeviceCollection);

	UINT count;
	hr = pMMDeviceCollection->GetCount(&count);
	if (FAILED(hr)) {
		ERR(L"IMMDeviceCollection::GetCount failed: hr = 0x%08x", hr);
		return hr;
	}

	for (UINT i = 0; i < count; i++) {
		IMMDevice *pMMDevice;

		// get the "n"th device
		hr = pMMDeviceCollection->Item(i, &pMMDevice);
		if (FAILED(hr)) {
			ERR(L"IMMDeviceCollection::Item failed: hr = 0x%08x", hr);
			return hr;
		}
		ReleaseOnExit releaseMMDevice(pMMDevice);

		// open the property store on that device
		IPropertyStore *pPropertyStore;
		hr = pMMDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
		if (FAILED(hr)) {
			ERR(L"IMMDevice::OpenPropertyStore failed: hr = 0x%08x", hr);
			return hr;
		}
		ReleaseOnExit releasePropertyStore(pPropertyStore);

		// get the long name property
		PROPVARIANT pv; PropVariantInit(&pv);
		hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
		if (FAILED(hr)) {
			ERR(L"IPropertyStore::GetValue failed: hr = 0x%08x", hr);
			return hr;
		}
		PropVariantClearOnExit clearPv(&pv);

		if (VT_LPWSTR != pv.vt) {
			ERR(L"PKEY_Device_FriendlyName variant type is %u - expected VT_LPWSTR", pv.vt);
			return E_UNEXPECTED;
		}

		// is it a match?
		if (0 == _wcsicmp(pv.pwszVal, szLongName)) {
			// did we already find it?
			if (NULL == *ppMMDevice) {
				*ppMMDevice = pMMDevice;
				pMMDevice->AddRef();
			}
			else {
				ERR(L"Found (at least) two devices named %ls", szLongName);
				return E_UNEXPECTED;
			}
		}
	}

	if (NULL == *ppMMDevice) {
		ERR(L"Could not find a device named %ls", szLongName);
		return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	}

	return S_OK;
}

HRESULT open_file(LPCWSTR szFileName, HMMIO *phFile) {
	MMIOINFO mi = { 0 };

	*phFile = mmioOpen(
		// some flags cause mmioOpen write to this buffer
		// but not any that we're using
		const_cast<LPWSTR>(szFileName),
		&mi,
		MMIO_WRITE | MMIO_CREATE
		);

	if (NULL == *phFile) {
		ERR(L"mmioOpen(\"%ls\", ...) failed. wErrorRet == %u", szFileName, mi.wErrorRet);
		return E_FAIL;
	}

	return S_OK;
}



//void CALLBACK VoicePlayNotification(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
//{
//
//	try {
//		//char* data = reinterpret_cast<char*>(dwInstance);
//		/*if (!isRunning) {
//		Logger("VoiceRecordNotification without enter");
//		return;
//		}*/
//
//		switch (uMsg)
//		{
//			case WOM_DONE:
//						{
//
//
//						}
//						break;
//			case WOM_CLOSE:
//						printf("voicePlayNotification with WIM_CLOSE");
//						break;
//			case WOM_OPEN:
//						printf("voicePlayNotification with WIM_OPEN");
//						break;
//		}
//	}
//	catch (int e) {
//		printf("..........................................");
//		return;
//	}
//	//Logger("VoiceRecordNotification exit");
//}
//
//
//
//void PlayRecord()
//{
//
//	waveOutWrite(hWaveOut, &WaveHdr, sizeof(WaveHdr)); // Playing the data
//
//}
//
//
//HRESULT InitiateOutputDevice(WAVEFORMATEX* ex){
//
//	//pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
//	//pEx->Samples.wValidBitsPerSample = 16;
//
//	//pwfx->nChannels = 2;
//	//pwfx->wBitsPerSample = 16;
//	//pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
//	//pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
//
//
//
//
//	pFormat.wFormatTag = ex->wFormatTag;
//	pFormat.nChannels = ex->nChannels;
//	pFormat.wBitsPerSample = ex->wBitsPerSample;
//	pFormat.nSamplesPerSec = ex->nSamplesPerSec;
//	pFormat.nBlockAlign = ex->nBlockAlign;
//	pFormat.nAvgBytesPerSec = ex->nAvgBytesPerSec;
//	pFormat.cbSize = ex->cbSize;
//
//
//	WaveHdr.lpData = (LPSTR)waveIn;
//	WaveHdr.dwBufferLength = BUFSIZE;
//	WaveHdr.dwBytesRecorded = 0;
//	WaveHdr.dwUser = 0L;
//	WaveHdr.dwFlags = 0L;
//	WaveHdr.dwLoops = 0L;
//
//
//	HRESULT result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &pFormat, 0L, 0L, WAVE_FORMAT_DIRECT);
//
//	//result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &pFormat, (DWORD_PTR)VoicePlayNotification, (DWORD_PTR)WaveHdr.lpData, CALLBACK_FUNCTION);
//	return result;
//
//
//}




DWORD WINAPI LoopbackCaptureThreadFunction(LPVOID pContext) {
	LoopbackCaptureThreadFunctionArguments *pArgs =
		(LoopbackCaptureThreadFunctionArguments*)pContext;

	pArgs->hr = CoInitialize(NULL);
	if (FAILED(pArgs->hr)) {
		ERR(L"CoInitialize failed: hr = 0x%08x", pArgs->hr);
		return 0;
	}
	CoUninitializeOnExit cuoe;

	pArgs->hr = LoopbackCapture(
		pArgs->pMMDevice,
		pArgs->hFile,
		pArgs->bInt16,
		pArgs->hStartedEvent,
		pArgs->hStopEvent,
		&pArgs->nFrames
		);

	return 0;
}

#include "WASAPIRenderer.h"
#include <Strsafe.h>
bool UseConsoleDevice=false;
bool UseCommunicationsDevice=false;
bool UseMultimediaDevice=false;
LPCWSTR OutputEndpoint = NULL;
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}
LPWSTR GetDeviceName(IMMDeviceCollection *DeviceCollection, UINT DeviceIndex)
{
	IMMDevice *device;
	LPWSTR deviceId;
	HRESULT hr;

	hr = DeviceCollection->Item(DeviceIndex, &device);
	if (FAILED(hr))
	{
		printf("Unable to get device %d: %x\n", DeviceIndex, hr);
		return NULL;
	}
	hr = device->GetId(&deviceId);
	if (FAILED(hr))
	{
		printf("Unable to get device %d id: %x\n", DeviceIndex, hr);
		return NULL;
	}

	IPropertyStore *propertyStore;
	hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
	SafeRelease(&device);
	if (FAILED(hr))
	{
		printf("Unable to open device %d property store: %x\n", DeviceIndex, hr);
		return NULL;
	}

	PROPVARIANT friendlyName;
	PropVariantInit(&friendlyName);
	hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
	SafeRelease(&propertyStore);

	if (FAILED(hr))
	{
		printf("Unable to retrieve friendly name for device %d : %x\n", DeviceIndex, hr);
		return NULL;
	}

	wchar_t deviceName[128];
	hr = StringCbPrintf(deviceName, sizeof(deviceName), L"%s (%s)", friendlyName.vt != VT_LPWSTR ? L"Unknown" : friendlyName.pwszVal, deviceId);
	if (FAILED(hr))
	{
		printf("Unable to format friendly name for device %d : %x\n", DeviceIndex, hr);
		return NULL;
	}

	PropVariantClear(&friendlyName);
	CoTaskMemFree(deviceId);

	wchar_t *returnValue = _wcsdup(deviceName);
	if (returnValue == NULL)
	{
		printf("Unable to allocate buffer for return\n");
		return NULL;
	}
	return returnValue;
}
//
//  Based on the input switches, pick the specified device to use.
//
bool PickDevice(IMMDevice **DeviceToUse, bool *IsDefaultDevice, ERole *DefaultDeviceRole)
{
	HRESULT hr;
	bool retValue = true;
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	IMMDeviceCollection *deviceCollection = NULL;

	*IsDefaultDevice = false;   // Assume we're not using the default device.

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(hr))
	{
		printf("Unable to instantiate device enumerator: %x\n", hr);
		retValue = false;
		goto Exit;
	}

	IMMDevice *device = NULL;

	//
	//  First off, if none of the console switches was specified, use the console device.
	//
	if (!UseConsoleDevice && !UseCommunicationsDevice && !UseMultimediaDevice && OutputEndpoint == NULL)
	{
		//
		//  The user didn't specify an output device, prompt the user for a device and use that.
		//
		hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
		if (FAILED(hr))
		{
			printf("Unable to retrieve device collection: %x\n", hr);
			retValue = false;
			goto Exit;
		}

		printf("Select an output device:\n");
		printf("    0:  Default Console Device\n");
		printf("    1:  Default Communications Device\n");
		printf("    2:  Default Multimedia Device\n");
		UINT deviceCount;
		hr = deviceCollection->GetCount(&deviceCount);
		if (FAILED(hr))
		{
			printf("Unable to get device collection length: %x\n", hr);
			retValue = false;
			goto Exit;
		}
		for (UINT i = 0; i < deviceCount; i += 1)
		{
			LPWSTR deviceName;

			deviceName = GetDeviceName(deviceCollection, i);
			if (deviceName == NULL)
			{
				retValue = false;
				goto Exit;
			}
			printf("    %d:  %S\n", i + 3, deviceName);
			free(deviceName);
		}
		wchar_t choice[10];
		_getws_s(choice);   // Note: Using the safe CRT version of _getws.

		long deviceIndex;
		wchar_t *endPointer;

		deviceIndex = wcstoul(choice, &endPointer, 0);
		if (deviceIndex == 0 && endPointer == choice)
		{
			printf("unrecognized device index: %S\n", choice);
			retValue = false;
			goto Exit;
		}
		switch (deviceIndex)
		{
		case 0:
			UseConsoleDevice = 1;
			break;
		case 1:
			UseCommunicationsDevice = 1;
			break;
		case 2:
			UseMultimediaDevice = 1;
			break;
		default:
			hr = deviceCollection->Item(deviceIndex - 3, &device);
			if (FAILED(hr))
			{
				printf("Unable to retrieve device %d: %x\n", deviceIndex - 3, hr);
				retValue = false;
				goto Exit;
			}
			break;
		}
	}
	else if (OutputEndpoint != NULL)
	{
		hr = deviceEnumerator->GetDevice(OutputEndpoint, &device);
		if (FAILED(hr))
		{
			printf("Unable to get endpoint for endpoint %S: %x\n", OutputEndpoint, hr);
			retValue = false;
			goto Exit;
		}
	}

	if (device == NULL)
	{
		ERole deviceRole = eConsole;    // Assume we're using the console role.
		if (UseConsoleDevice)
		{
			deviceRole = eConsole;
		}
		else if (UseCommunicationsDevice)
		{
			deviceRole = eCommunications;
		}
		else if (UseMultimediaDevice)
		{
			deviceRole = eMultimedia;
		}
		hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, deviceRole, &device);
		if (FAILED(hr))
		{
			printf("Unable to get default device for role %d: %x\n", deviceRole, hr);
			retValue = false;
			goto Exit;
		}
		*IsDefaultDevice = true;
		*DefaultDeviceRole = deviceRole;
	}

	*DeviceToUse = device;
	retValue = true;
Exit:
	SafeRelease(&deviceCollection);
	SafeRelease(&deviceEnumerator);

	return retValue;
}

HRESULT LoopbackCapture(
	IMMDevice *pMMDevice,
	HMMIO hFile,
	bool bInt16,
	HANDLE hStartedEvent,
	HANDLE hStopEvent,
	PUINT32 pnFrames
	) {
	bool isDefaultDevice;
	ERole role;

	if (!PickDevice(&pMMDevice, &isDefaultDevice, &role))
	{
		
		exit;
	}

	CWASAPIRenderer *renderer = new (std::nothrow) CWASAPIRenderer(pMMDevice, isDefaultDevice, role);
	if (renderer == NULL)
	{
		printf("Unable to allocate renderer\n");
		return -1;
	}

	//9876
	RenderBuffer *renderQueue =NULL;
	RenderBuffer **currentBufferTail = &renderQueue;
	int TargetLatency = 30;
	if (renderer->Initialize(TargetLatency))
	{
		//
		//  We've initialized the renderer.  Once we've done that, we know some information about the
		//  mix format and we can allocate the buffer that we're going to render.
		//
		//
		//  The buffer is going to contain "TargetDuration" seconds worth of PCM data.  That means 
		//  we're going to have TargetDuration*samples/second frames multiplied by the frame size.
		//
	//	UINT32 renderBufferSizeInBytes = (renderer->BufferSizePerPeriod()  * renderer->FrameSize());
	//	size_t renderDataLength = (renderer->SamplesPerSecond() * TargetDurationInSec * renderer->FrameSize()) + (renderBufferSizeInBytes - 1);
	//	size_t renderBufferCount = renderDataLength / (renderBufferSizeInBytes);
		//
		//  Render buffer queue. Because we need to insert each buffer at the end of the linked list instead of at the head, 
		//  we keep a pointer to a pointer to the variable which holds the tail of the current list in currentBufferTail.
		//
		RenderBuffer *renderQueue = NULL;
		RenderBuffer **currentBufferTail = &renderQueue;
	}
	
	HRESULT hr;
	// activate an IAudioClient
	IAudioClient *pAudioClient;
	hr = pMMDevice->Activate(
		__uuidof(IAudioClient),
		CLSCTX_ALL, NULL,
		(void**)&pAudioClient
		);
	if (FAILED(hr)) {
		ERR(L"IMMDevice::Activate(IAudioClient) failed: hr = 0x%08x", hr);
		return hr;
	}
	ReleaseOnExit releaseAudioClient(pAudioClient);

	// get the default device periodicity
	REFERENCE_TIME hnsDefaultDevicePeriod;
	hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, NULL);
	if (FAILED(hr)) {
		ERR(L"IAudioClient::GetDevicePeriod failed: hr = 0x%08x", hr);
		return hr;
	}

	// get the default device format
	WAVEFORMATEX *pwfx;
	hr = pAudioClient->GetMixFormat(&pwfx);
	if (FAILED(hr)) {
		ERR(L"IAudioClient::GetMixFormat failed: hr = 0x%08x", hr);
		return hr;
	}
	CoTaskMemFreeOnExit freeMixFormat(pwfx);

	if (bInt16) {
		// coerce int-16 wave format
		// can do this in-place since we're not changing the size of the format
		// also, the engine will auto-convert from float to int for us
		switch (pwfx->wFormatTag) {
		case WAVE_FORMAT_IEEE_FLOAT:
			pwfx->wFormatTag = WAVE_FORMAT_PCM;
			pwfx->wBitsPerSample = 16;
			pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
			pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
			break;

		case WAVE_FORMAT_EXTENSIBLE:
		{
									   // naked scope for case-local variable
									   PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
									   if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) {

										   pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
										   pEx->Samples.wValidBitsPerSample = 16;

										   pwfx->nChannels = 2;
										   pwfx->wBitsPerSample = 16;
										   pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
										   pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;


									   }
									   else {
										   ERR(L"%s", L"Don't know how to coerce mix format to int-16");
										   return E_UNEXPECTED;
									   }
		}
			break;

		default:
			ERR(L"Don't know how to coerce WAVEFORMATEX with wFormatTag = 0x%08x to int-16", pwfx->wFormatTag);
			return E_UNEXPECTED;
		}
	}
	//InitiateOutputDevice(pwfx);
	//RTCPlayer rtcplayer(pwfx);

	MMCKINFO ckRIFF = { 0 };
	MMCKINFO ckData = { 0 };
	hr = WriteWaveHeader(hFile, pwfx, &ckRIFF, &ckData);
	if (FAILED(hr)) {
		// WriteWaveHeader does its own logging
		return hr;
	}

	// create a periodic waitable timer
	HANDLE hWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);
	if (NULL == hWakeUp) {
		DWORD dwErr = GetLastError();
		ERR(L"CreateWaitableTimer failed: last error = %u", dwErr);
		return HRESULT_FROM_WIN32(dwErr);
	}
	CloseHandleOnExit closeWakeUp(hWakeUp);

	UINT32 nBlockAlign = pwfx->nBlockAlign;
	*pnFrames = 0;

	// call IAudioClient::Initialize
	// note that AUDCLNT_STREAMFLAGS_LOOPBACK and AUDCLNT_STREAMFLAGS_EVENTCALLBACK
	// do not work together...
	// the "data ready" event never gets set
	// so we're going to do a timer-driven loop
	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_LOOPBACK,
		0, 0, pwfx, 0
		);
	if (FAILED(hr)) {
		ERR(L"IAudioClient::Initialize failed: hr = 0x%08x", hr);
		return hr;
	}

	// activate an IAudioCaptureClient
	IAudioCaptureClient *pAudioCaptureClient;
	hr = pAudioClient->GetService(
		__uuidof(IAudioCaptureClient),
		(void**)&pAudioCaptureClient
		);
	if (FAILED(hr)) {
		ERR(L"IAudioClient::GetService(IAudioCaptureClient) failed: hr = 0x%08x", hr);
		return hr;
	}
	ReleaseOnExit releaseAudioCaptureClient(pAudioCaptureClient);

	// register with MMCSS
	DWORD nTaskIndex = 0;
	HANDLE hTask = AvSetMmThreadCharacteristics(L"Audio", &nTaskIndex);
	if (NULL == hTask) {
		DWORD dwErr = GetLastError();
		ERR(L"AvSetMmThreadCharacteristics failed: last error = %u", dwErr);
		return HRESULT_FROM_WIN32(dwErr);
	}
	AvRevertMmThreadCharacteristicsOnExit unregisterMmcss(hTask);

	// set the waitable timer
	LARGE_INTEGER liFirstFire;
	liFirstFire.QuadPart = -hnsDefaultDevicePeriod / 2; // negative means relative time
	LONG lTimeBetweenFires = (LONG)hnsDefaultDevicePeriod / 2 / (10 * 1000); // convert to milliseconds
	BOOL bOK = SetWaitableTimer(
		hWakeUp,
		&liFirstFire,
		lTimeBetweenFires,
		NULL, NULL, FALSE
		);
	if (!bOK) {
		DWORD dwErr = GetLastError();
		ERR(L"SetWaitableTimer failed: last error = %u", dwErr);
		return HRESULT_FROM_WIN32(dwErr);
	}
	CancelWaitableTimerOnExit cancelWakeUp(hWakeUp);

	// call IAudioClient::Start
	hr = pAudioClient->Start();
	if (FAILED(hr)) {
		ERR(L"IAudioClient::Start failed: hr = 0x%08x", hr);
		return hr;
	}
	AudioClientStopOnExit stopAudioClient(pAudioClient);

	SetEvent(hStartedEvent);

	// loopback capture loop
	HANDLE waitArray[2] = { hStopEvent, hWakeUp };
	DWORD dwWaitResult;

	bool bDone = false;
	bool bFirstPacket = true;
	for (UINT32 nPasses = 0; !bDone; nPasses++) {
		// drain data while it is available
		UINT32 nNextPacketSize;
		for (
			hr = pAudioCaptureClient->GetNextPacketSize(&nNextPacketSize);
			SUCCEEDED(hr) && nNextPacketSize > 0;
		hr = pAudioCaptureClient->GetNextPacketSize(&nNextPacketSize)
			) {
			// get the captured data
			BYTE *pData;
			UINT32 nNumFramesToRead;
			DWORD dwFlags;

			hr = pAudioCaptureClient->GetBuffer(
				&pData,
				&nNumFramesToRead,
				&dwFlags,
				NULL,
				NULL
				);
			if (FAILED(hr)) {
				ERR(L"IAudioCaptureClient::GetBuffer failed on pass %u after %u frames: hr = 0x%08x", nPasses, *pnFrames, hr);
				return hr;
			}

			if (bFirstPacket && AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY == dwFlags) {
				LOG(L"%s", L"Probably spurious glitch reported on first packet");
			}
			else if (0 != dwFlags) {
				LOG(L"IAudioCaptureClient::GetBuffer set flags to 0x%08x on pass %u after %u frames", dwFlags, nPasses, *pnFrames);
				//   return E_UNEXPECTED;
				//	continue;
			}

			if (0 == nNumFramesToRead) {
				ERR(L"IAudioCaptureClient::GetBuffer said to read 0 frames on pass %u after %u frames", nPasses, *pnFrames);
				return E_UNEXPECTED;
			}

			LONG lBytesToWrite = nNumFramesToRead * nBlockAlign;
#pragma prefast(suppress: __WARNING_INCORRECT_ANNOTATION, "IAudioCaptureClient::GetBuffer SAL annotation implies a 1-byte buffer")





			LONG lBytesWritten = mmioWrite(hFile, reinterpret_cast<PCHAR>(pData), lBytesToWrite);
			mmioFlush(hFile, MMIO_EMPTYBUF);
			printf("\n%d\n", lBytesWritten);
			//printf(reinterpret_cast<PCHAR>(pData));
			//	rtcplayer.play(reinterpret_cast<PCHAR>(pData), lBytesToWrite);

			//servermaker.UdpWrite(reinterpret_cast<PCHAR>(pData));
			//memset(waveIn, 0, sizeof(waveIn));
			//memcpy(waveIn, reinterpret_cast<PCHAR>(pData), lBytesToWrite);
			//WaveHdr.dwBytesRecorded = lBytesToWrite;
			//WaveHdr.dwFlags = 3;
			//PlayRecord();

			/*for (int i = 0; i < 2000; i++){
			if (i >= 1920)printf("\n--%d\n", (unsigned char)pData[i]);
			else printf("\n%d\n", (unsigned char)pData[i]);
			}*/

			int str = strlen(reinterpret_cast<PCHAR>(pData));

			//12345

			RenderBuffer *renderBuffer = new (std::nothrow) RenderBuffer();
			if (renderBuffer == NULL)
			{
				printf("Unable to allocate render buffer\n");
				return -1;
			}
			renderBuffer->_BufferLength = lBytesWritten;
			renderBuffer->_Buffer = new (std::nothrow) BYTE[lBytesWritten];
			if (renderBuffer->_Buffer == NULL)
			{
				printf("Unable to allocate render buffer\n");
				return -1;
			}
			memcpy(renderBuffer->_Buffer, (BYTE*)reinterpret_cast<PCHAR>(pData), lBytesWritten);
			*currentBufferTail = renderBuffer;
			currentBufferTail = &renderBuffer->_Next;



			if (lBytesToWrite != lBytesWritten) {
				ERR(L"mmioWrite wrote %u bytes on pass %u after %u frames: expected %u bytes", lBytesWritten, nPasses, *pnFrames, lBytesToWrite);
				return E_UNEXPECTED;
			}

			*pnFrames += nNumFramesToRead;

			hr = pAudioCaptureClient->ReleaseBuffer(nNumFramesToRead);
			if (FAILED(hr)) {
				ERR(L"IAudioCaptureClient::ReleaseBuffer failed on pass %u after %u frames: hr = 0x%08x", nPasses, *pnFrames, hr);
				return hr;
			}

			bFirstPacket = false;
		}

		if (FAILED(hr)) {
			ERR(L"IAudioCaptureClient::GetNextPacketSize failed on pass %u after %u frames: hr = 0x%08x", nPasses, *pnFrames, hr);
			return hr;
		}

		dwWaitResult = WaitForMultipleObjects(
			ARRAYSIZE(waitArray), waitArray,
			FALSE, INFINITE
			);

		if (WAIT_OBJECT_0 == dwWaitResult) {
			LOG(L"Received stop event after %u passes and %u frames", nPasses, *pnFrames);
			bDone = true;
			continue; // exits loop
		}

		if (WAIT_OBJECT_0 + 1 != dwWaitResult) {
			ERR(L"Unexpected WaitForMultipleObjects return value %u on pass %u after %u frames", dwWaitResult, nPasses, *pnFrames);
			return E_UNEXPECTED;
		}
	} // capture loop


	if (renderer->Start(renderQueue))
	{
		do
		{
			printf(".");
			Sleep(1000);
		} while (true);
		printf("\n");

		renderer->Stop();
		renderer->Shutdown();
		SafeRelease(&renderer);
	}

	hr = FinishWaveFile(hFile, &ckData, &ckRIFF);
	if (FAILED(hr)) {
		// FinishWaveFile does it's own logging
		return hr;
	}

	return hr;
}

HRESULT WriteWaveHeader(HMMIO hFile, LPCWAVEFORMATEX pwfx, MMCKINFO *pckRIFF, MMCKINFO *pckData) {
	MMRESULT result;

	// make a RIFF/WAVE chunk
	pckRIFF->ckid = MAKEFOURCC('R', 'I', 'F', 'F');
	pckRIFF->fccType = MAKEFOURCC('W', 'A', 'V', 'E');

	result = mmioCreateChunk(hFile, pckRIFF, MMIO_CREATERIFF);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioCreateChunk(\"RIFF/WAVE\") failed: MMRESULT = 0x%08x", result);
		return E_FAIL;
	}

	// make a 'fmt ' chunk (within the RIFF/WAVE chunk)
	MMCKINFO chunk;
	chunk.ckid = MAKEFOURCC('f', 'm', 't', ' ');
	result = mmioCreateChunk(hFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioCreateChunk(\"fmt \") failed: MMRESULT = 0x%08x", result);
		return E_FAIL;
	}

	// write the WAVEFORMATEX data to it
	LONG lBytesInWfx = sizeof(WAVEFORMATEX)+pwfx->cbSize;
	LONG lBytesWritten =
		mmioWrite(
		hFile,
		reinterpret_cast<PCHAR>(const_cast<LPWAVEFORMATEX>(pwfx)),
		lBytesInWfx
		);
	if (lBytesWritten != lBytesInWfx) {
		ERR(L"mmioWrite(fmt data) wrote %u bytes; expected %u bytes", lBytesWritten, lBytesInWfx);
		return E_FAIL;
	}

	// ascend from the 'fmt ' chunk
	result = mmioAscend(hFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioAscend(\"fmt \" failed: MMRESULT = 0x%08x", result);
		return E_FAIL;
	}

	// make a 'fact' chunk whose data is (DWORD)0
	chunk.ckid = MAKEFOURCC('f', 'a', 'c', 't');
	result = mmioCreateChunk(hFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioCreateChunk(\"fmt \") failed: MMRESULT = 0x%08x", result);
		return E_FAIL;
	}

	// write (DWORD)0 to it
	// this is cleaned up later
	DWORD frames = 0;
	lBytesWritten = mmioWrite(hFile, reinterpret_cast<PCHAR>(&frames), sizeof(frames));
	if (lBytesWritten != sizeof(frames)) {
		ERR(L"mmioWrite(fact data) wrote %u bytes; expected %u bytes", lBytesWritten, (UINT32)sizeof(frames));
		return E_FAIL;
	}

	// ascend from the 'fact' chunk
	result = mmioAscend(hFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioAscend(\"fact\" failed: MMRESULT = 0x%08x", result);
		return E_FAIL;
	}

	// make a 'data' chunk and leave the data pointer there
	pckData->ckid = MAKEFOURCC('d', 'a', 't', 'a');
	result = mmioCreateChunk(hFile, pckData, 0);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioCreateChunk(\"data\") failed: MMRESULT = 0x%08x", result);
		return E_FAIL;
	}
	mmioFlush(hFile, MMIO_EMPTYBUF);
	return S_OK;
}

HRESULT FinishWaveFile(HMMIO hFile, MMCKINFO *pckRIFF, MMCKINFO *pckData) {
	MMRESULT result;

	result = mmioAscend(hFile, pckData, 0);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioAscend(\"data\" failed: MMRESULT = 0x%08x", result);
		return E_FAIL;
	}

	result = mmioAscend(hFile, pckRIFF, 0);
	if (MMSYSERR_NOERROR != result) {
		ERR(L"mmioAscend(\"RIFF/WAVE\" failed: MMRESULT = 0x%08x", result);
		return E_FAIL;
	}

	return S_OK;
}

