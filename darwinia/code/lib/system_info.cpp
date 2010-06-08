#include "lib/universal_include.h"

#include <mmsystem.h>
//#include <dxdiag.h>
#include <stdio.h>

#include "lib/debug_utils.h"
#include "lib/system_info.h"


SystemInfo *g_systemInfo = NULL;


SystemInfo::SystemInfo()
{
	GetLocaleDetails();
	GetAudioDetails();
	GetDirectXVersion();
}


SystemInfo::~SystemInfo()
{
}


void SystemInfo::GetLocaleDetails()
{
    int size;
    bool languageSuccess = false;

    if( !languageSuccess )
    {
	    size = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, NULL, 0);
	    m_localeInfo.m_language = new char[size + 1];
	    DarwiniaReleaseAssert(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, m_localeInfo.m_language, size),
				      "Couldn't get locale details");
    }
    

	size = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGCOUNTRY, NULL, 0);
	m_localeInfo.m_country = new char[size + 1];
	DarwiniaReleaseAssert(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGCOUNTRY, m_localeInfo.m_country, size),
				  "Couldn't get country details");
}


void SystemInfo::GetAudioDetails()
{
	unsigned int numDevs = waveOutGetNumDevs();
	m_audioInfo.m_numDevices = numDevs;
	m_audioInfo.m_preferredDevice = -1;
	int bestScore = -1000;

	m_audioInfo.m_deviceNames = new char *[numDevs];
	for (unsigned int i = 0; i < numDevs; ++i)
	{
		WAVEOUTCAPS caps;
		waveOutGetDevCaps(i, &caps, sizeof(WAVEOUTCAPS));
		m_audioInfo.m_deviceNames[i] = strdup(caps.szPname);
		
		int score = 0;
		if (caps.dwFormats & WAVE_FORMAT_1M08) score++;
		if (caps.dwFormats & WAVE_FORMAT_1M16) score++;
		if (caps.dwFormats & WAVE_FORMAT_1S08) score++;
		if (caps.dwFormats & WAVE_FORMAT_1S16) score++;
		if (caps.dwFormats & WAVE_FORMAT_2M08) score++;
		if (caps.dwFormats & WAVE_FORMAT_2M16) score++;
		if (caps.dwFormats & WAVE_FORMAT_2S08) score++;
		if (caps.dwFormats & WAVE_FORMAT_2S16) score++;
		if (caps.dwFormats & WAVE_FORMAT_4M08) score++;
		if (caps.dwFormats & WAVE_FORMAT_4M16) score++;
		if (caps.dwFormats & WAVE_FORMAT_4S08) score++;
		if (caps.dwFormats & WAVE_FORMAT_4S16) score++;
		if (caps.dwSupport & WAVECAPS_SYNC) score -= 10;
		if (caps.dwSupport & WAVECAPS_LRVOLUME) score++;
		if (caps.dwSupport & WAVECAPS_PITCH) score++;
		if (caps.dwSupport & WAVECAPS_PLAYBACKRATE) score++;
		if (caps.dwSupport & WAVECAPS_VOLUME) score++;
		if (caps.dwSupport & WAVECAPS_SAMPLEACCURATE) score++;

		if (score > bestScore)
		{
			m_audioInfo.m_preferredDevice = i;
		}
	}

	DarwiniaReleaseAssert(m_audioInfo.m_preferredDevice != -1, "No suitable audio hardware found");
}


void SystemInfo::GetDirectXVersion()
{
	HKEY hkey;
	long errCode;
	unsigned long bufLen = 256; 
	unsigned char buf[256];

	m_directXVersion = -1;

	errCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\DirectX", 0, KEY_READ, &hkey);
	DarwiniaReleaseAssert(errCode == ERROR_SUCCESS, "Failed to get DirectX Version");
	errCode = RegQueryValueEx(hkey, "InstalledVersion", NULL, NULL, buf, &bufLen);

    if( errCode == ERROR_SUCCESS )
    {
		m_directXVersion = buf[3];
	}
	else
	{
		// NOTE by Chris : This value doesn't exist on Windows98
		// However the key "Version" does exist
		errCode = RegQueryValueEx(hkey, "Version", NULL, NULL, buf, &bufLen );
		if( errCode == ERROR_SUCCESS )
		{
			m_directXVersion = buf[3];
		}
    }

	RegCloseKey(hkey);

//	if (m_directXVersion == -1)
//	{
//		long hr;
//		bool bCleanupCOM = false;
//		bool bSuccessGettingMajor = false;
//    
//		// Init COM.  COM may fail if its already been inited with a different 
//		// concurrency model.  And if it fails you shouldn't release it.
//		hr = CoInitialize(NULL);
//		bCleanupCOM = SUCCEEDED(hr);
//
//		// Get an IDxDiagProvider
//		bool bGotDirectXVersion = false;
//		IDxDiagProvider* pDxDiagProvider = NULL;
//		hr = CoCreateInstance( CLSID_DxDiagProvider,
//							   NULL,
//							   CLSCTX_INPROC_SERVER,
//							   IID_IDxDiagProvider,
//							   (LPVOID*) &pDxDiagProvider );
//		if( SUCCEEDED(hr) )
//		{
//			// Fill out a DXDIAG_INIT_PARAMS struct
//			DXDIAG_INIT_PARAMS dxDiagInitParam;
//			ZeroMemory( &dxDiagInitParam, sizeof(DXDIAG_INIT_PARAMS) );
//			dxDiagInitParam.dwSize                  = sizeof(DXDIAG_INIT_PARAMS);
//			dxDiagInitParam.dwDxDiagHeaderVersion   = DXDIAG_DX9_SDK_VERSION;
//			dxDiagInitParam.bAllowWHQLChecks        = false;
//			dxDiagInitParam.pReserved               = NULL;
//
//			// Init the m_pDxDiagProvider
//			hr = pDxDiagProvider->Initialize( &dxDiagInitParam ); 
//			if( SUCCEEDED(hr) )
//			{
//				IDxDiagContainer* pDxDiagRoot = NULL;
//				IDxDiagContainer* pDxDiagSystemInfo = NULL;
//
//				// Get the DxDiag root container
//				hr = pDxDiagProvider->GetRootContainer( &pDxDiagRoot );
//				if( SUCCEEDED(hr) ) 
//				{
//					// Get the object called DxDiag_SystemInfo
//					hr = pDxDiagRoot->GetChildContainer( L"DxDiag_SystemInfo", &pDxDiagSystemInfo );
//					if( SUCCEEDED(hr) )
//					{
//						VARIANT var;
//						VariantInit( &var );
//
//						// Get the "dwDirectXVersionMajor" property
//						hr = pDxDiagSystemInfo->GetProp( L"dwDirectXVersionMajor", &var );
//						if( SUCCEEDED(hr) && var.vt == VT_UI4 )
//						{
//							m_directXVersion = var.ulVal; 
//							bSuccessGettingMajor = true;
//						}
//						VariantClear( &var );
//
//						// If it all worked right, then mark it down
//						if( bSuccessGettingMajor )
//							bGotDirectXVersion = true;
//
//						pDxDiagSystemInfo->Release();
//					}
//
//					pDxDiagRoot->Release();
//				}
//			}
//
//			pDxDiagProvider->Release();
//		}
//
//		if( bCleanupCOM )
//			CoUninitialize();		
//	}
}
