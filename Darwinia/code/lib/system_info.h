#ifndef INCLUDED_SYSTEM_INFO_H
#define INCLUDED_SYSTEM_INFO_H


//*****************************************************************************
// Class LocaleInfo
//*****************************************************************************

class LocaleInfo
{
public:
	char *m_language;
	char *m_country;

	LocaleInfo(): m_language(NULL), m_country(NULL) {}
	~LocaleInfo() { delete [] m_language; delete [] m_country; }
};



//*****************************************************************************
// Class AudioInfo
//*****************************************************************************

class AudioInfo
{
public:
	char **m_deviceNames;
	unsigned int m_numDevices;
	int m_preferredDevice;

	AudioInfo(): m_deviceNames(NULL), m_numDevices(0) {}
	~AudioInfo()
	{
		for (unsigned int i = 0; i < m_numDevices; ++i)
			free(m_deviceNames[i]);
		delete[] m_deviceNames;
	}
};



//*****************************************************************************
// Class SystemInfo
//*****************************************************************************

class SystemInfo
{
private:
	void GetLocaleDetails();
	void GetAudioDetails();
	void GetDirectXVersion();

public:
	LocaleInfo	m_localeInfo;
	AudioInfo	m_audioInfo;
	int			m_directXVersion;

	SystemInfo();
	~SystemInfo();
};


extern SystemInfo *g_systemInfo;


#endif
