#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>
#include <map>

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

// General purpose newStr() variant for creating ASCII C-strings from Apple's CFStrings.
// Always succeeds, but may substitute '?' chars for non-ASCII chars in cfStr.
char *newStr(CFStringRef cfStr)
{
	char *cppStr = NULL;
	CFIndex cfStrLen, cppStrLen;

	if (cfStr)
	{
		cfStrLen = CFStringGetLength(cfStr);
		cppStrLen = CFStringGetMaximumSizeForEncoding(cfStrLen, kCFStringEncodingASCII);
		cppStr = new char[cppStrLen+1];
		CFStringGetBytes(cfStr, CFRangeMake(0, cfStrLen), kCFStringEncodingASCII, '?', false,
						 (UInt8 *)cppStr, cppStrLen, &cppStrLen);
		cppStr[cppStrLen] = '\0';
	}
	
	return cppStr;
}

SystemInfo *g_systemInfo = NULL;

SystemInfo::SystemInfo()
: m_directXVersion(0)
{
	GetLocaleDetails();
	GetAudioDetails();
}

SystemInfo::~SystemInfo()
{

}

// Grab the language name from the user defaults. This is the
// Apple-recommended solution for 10.4.
void SystemInfo::GetLocaleDetails()
{
	CFArrayRef systemLanguages;
	std::map<std::string, char*> langs;
	
	// Mapping from ISO language codes to names, in order to match the Win32 output
	langs["en"] = "english";
	langs["fr"] = "french";
	langs["de"] = "german";
	langs["it"] = "italian";
	langs["es"] = "spanish";
	
	systemLanguages =(CFArrayRef)CFPreferencesCopyAppValue(CFSTR("AppleLanguages"), kCFPreferencesCurrentApplication);
	if (systemLanguages && CFArrayGetCount(systemLanguages))
	{
		char *langCode = newStr((CFStringRef)CFArrayGetValueAtIndex(systemLanguages, 0));
		if (langs.find(langCode) != langs.end())
			m_localeInfo.m_language = newStr(langs[langCode]);
		else
			m_localeInfo.m_language = newStr("Any");
		delete langCode;
	}
	else
		m_localeInfo.m_language = newStr("Any");
	
	if (systemLanguages)
		CFRelease(systemLanguages);
		
	// We could get the ISO country code similarly, using the "AppleLocale" key, but
	// we'd need a huge mapping function to match the Windows output.
	m_localeInfo.m_country = newStr("Any");
}


void SystemInfo::GetAudioDetails()
{
	m_audioInfo.m_numDevices = 1;
	m_audioInfo.m_preferredDevice = 0;
	m_audioInfo.m_deviceNames = new char *[m_audioInfo.m_numDevices];
		
	for (int i = 0; i < m_audioInfo.m_numDevices; i++) {
		m_audioInfo.m_deviceNames[i] = newStr("Mac OS X Audio");
	}
	
	AppReleaseAssert(m_audioInfo.m_numDevices > 0, "No Audio Hardware Found");	
}
