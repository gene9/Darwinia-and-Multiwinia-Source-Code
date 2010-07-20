#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>

#include <sys/utsname.h>
#include <mach/mach.h>
#include <mach-o/arch.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#define AudioInfo MacOsXAudioInfo
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
#include <CoreAudio/CoreAudio.h>
#undef AudioInfo

#include "lib/debug_utils.h"
#include "lib/string_utils.h"
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
	// 10.3 Only code
	m_localeInfo.m_language = NewStr("Any");
	m_localeInfo.m_country = NewStr("Any");
}


void SystemInfo::GetAudioDetails()
{
	m_audioInfo.m_numDevices = 1;
	m_audioInfo.m_preferredDevice = 0;
	m_audioInfo.m_deviceNames = new char *[m_audioInfo.m_numDevices];

	for (int i = 0; i < m_audioInfo.m_numDevices; i++) {
		m_audioInfo.m_deviceNames[i] = NewStr("Mac OS X Audio");
	}

	DarwiniaReleaseAssert(m_audioInfo.m_numDevices > 0, "No Audio Hardware Found");
}

void SystemInfo::GetDirectXVersion()
{
	m_directXVersion = 0;
}
