#include "lib/universal_include.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>


#include "lib/debug_utils.h"
#include "lib/string_utils.h"
#include "lib/system_info.h"


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

char *ReAllocStr(const char *in)
{
	char *out = (char*)malloc(strlen(in)+1);
	return strcpy(out,in);
}

void SystemInfo::GetLocaleDetails()
{
	
	std::map<std::string, char*> langs;

	// Mapping from ISO language codes to names, in order to match the Win32 output
	langs["en"] = "english";
	langs["fr"] = "french";
	langs["de"] = "german";
	langs["it"] = "italian";
	langs["es"] = "spanish";
	
	char *tmp = getenv("LANGUAGE");
	if (tmp && strlen(tmp))
	{
		char *langCode = ReAllocStr(tmp);
		langCode[2] = '\0';
		if (langs.find(langCode) != langs.end())
			m_localeInfo.m_language = ReAllocStr(langs[langCode]);
		else
			m_localeInfo.m_language = ReAllocStr("Any");
		free(langCode);
	}
	else
		m_localeInfo.m_language = ReAllocStr("Any");
	
	//Doing country stuff would be too hard.
	m_localeInfo.m_country = ReAllocStr("Any");
}


void SystemInfo::GetAudioDetails()
{
	m_audioInfo.m_numDevices = 1;
	m_audioInfo.m_preferredDevice = 0;
	m_audioInfo.m_deviceNames = new char *[m_audioInfo.m_numDevices];
		
	for (int i = 0; i < m_audioInfo.m_numDevices; i++) {
		m_audioInfo.m_deviceNames[i] = ReAllocStr("A Giant Stub for the Linux Audio Mess");
	}
	
	AppReleaseAssert(m_audioInfo.m_numDevices > 0, "No Audio Hardware Found");	
}
