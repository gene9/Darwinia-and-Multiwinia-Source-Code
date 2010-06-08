#include "lib/universal_include.h"

#include "lib/resource.h"
#include "sound/sample_cache.h"
#include "sound/sound_stream_decoder.h"
#include "app.h"


CachedSampleManager g_cachedSampleManager;
bool g_deletingCachedSampleHandle = false;


//*****************************************************************************
// Class CachedSample
//*****************************************************************************

CachedSample::CachedSample(char const *_sampleName)
:	m_amountCached(0)
{
    char fullPath[512] = "sounds/";
	strcat(fullPath, _sampleName);

	m_soundStreamDecoder = g_app->m_resource->GetSoundStreamDecoder(fullPath);
    DarwiniaReleaseAssert( m_soundStreamDecoder, "Failed to open sound stream decoder : %s", fullPath );

	m_numChannels = m_soundStreamDecoder->m_numChannels;
	m_numSamples = m_soundStreamDecoder->m_numSamples;
	m_freq = m_soundStreamDecoder->m_freq;

	m_rawSampleData = new signed short[m_numChannels * m_numSamples];
}


CachedSample::~CachedSample()
{
	delete m_soundStreamDecoder; m_soundStreamDecoder = NULL;
	delete [] m_rawSampleData; m_rawSampleData = NULL;
}


void CachedSample::Read(signed short *_data, unsigned int _startSample, unsigned int _numSamples)
{
	if (m_soundStreamDecoder)
	{
		int highestSampleRequested = _startSample + _numSamples - 1;
		if (highestSampleRequested >= m_amountCached)
		{
			int amountToRead = highestSampleRequested - m_amountCached + 1;
			unsigned int amountRead = m_soundStreamDecoder->Read(
										&m_rawSampleData[m_amountCached],
										amountToRead);
			m_amountCached += amountRead;
			DarwiniaDebugAssert(m_amountCached <= m_numSamples);
			if (m_amountCached == m_numSamples)
			{
				delete m_soundStreamDecoder; m_soundStreamDecoder = NULL;
			}
		}
	}

	memcpy(_data, &m_rawSampleData[_startSample], sizeof(signed short) * m_numChannels * _numSamples);
}



//*****************************************************************************
// Class CachedSampleHandle
//*****************************************************************************

CachedSampleHandle::CachedSampleHandle(CachedSample *_sample)
:	m_nextSampleIndex(0),
	m_cachedSample(_sample)
{
}


CachedSampleHandle::~CachedSampleHandle()
{
	m_cachedSample = NULL;
	m_nextSampleIndex = 0xffffffff;
}


unsigned int CachedSampleHandle::Read(signed short *_data, unsigned int _numSamples)
{
	int samplesRemaining = m_cachedSample->m_numSamples - m_nextSampleIndex;
	if (_numSamples > samplesRemaining)
	{
		_numSamples = samplesRemaining;
	}

	m_cachedSample->Read(_data, m_nextSampleIndex, _numSamples);
	m_nextSampleIndex += _numSamples;

	return _numSamples;
}


void CachedSampleHandle::Restart()
{
	m_nextSampleIndex = 0;
}



//*****************************************************************************
// Class CachedSampleManager
//*****************************************************************************

CachedSampleManager::~CachedSampleManager()
{
	for (int i = 0; i < m_cache.Size(); ++i)
	{
		delete m_cache.GetData(i);
	}
}


CachedSampleHandle *CachedSampleManager::GetSample(char const *_sampleName)
{
	CachedSample *cachedSample = m_cache.GetData(_sampleName);

	if (!cachedSample)
	{
		cachedSample = new CachedSample(_sampleName);
		m_cache.PutData(_sampleName, cachedSample);
    }

	CachedSampleHandle *rv = new CachedSampleHandle(cachedSample);
	return rv;
}


void CachedSampleManager::EmptyCache()
{
    m_cache.EmptyAndDelete();
}


int CachedSampleManager::GetMemoryUsage()
{
    int memoryUsage = 0;

    for( int i = 0; i < m_cache.Size(); ++i )
    {
        if( m_cache.ValidIndex(i) )
        {
            CachedSample *sample = m_cache.GetData(i);
            int sampleSize = sizeof(signed short) * sample->m_numChannels * sample->m_numSamples;
            memoryUsage += sampleSize;
        }
    }

    return memoryUsage;
}

