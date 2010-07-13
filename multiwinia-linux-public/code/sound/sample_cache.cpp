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
:	m_amountCached(0),
	m_lockedCount(0),
	m_lastUsedTime(0),
	m_sampleName(_sampleName)
#ifdef TARGET_OS_MACOSX
	,m_loadState(kSampleLoadState_NotLoaded)
#endif
{
	m_name = strdup(_sampleName);

    char fullPath[512] = "sounds/";
	strcat(fullPath, _sampleName);
	m_soundStreamDecoder = g_app->m_resource->GetSoundStreamDecoder(fullPath);

    if( !m_soundStreamDecoder )
    {
        sprintf( fullPath, "mwsounds/%s", _sampleName );
        m_soundStreamDecoder = g_app->m_resource->GetSoundStreamDecoder(fullPath);
    }

    AppReleaseAssert( m_soundStreamDecoder, "Failed to open sound stream decoder : %s\n", fullPath );

	m_numChannels = m_soundStreamDecoder->m_numChannels;
	m_numSamples = m_soundStreamDecoder->m_numSamples;
	m_freq = m_soundStreamDecoder->m_freq;

    AppReleaseAssert( 0 < m_numChannels && m_numChannels <= 8 &&
                      0 < m_numSamples && m_numSamples <= 268435456 &&
                      0 < m_freq && m_freq <= 192000 &&
                      ( m_numChannels * m_numSamples ) <= ( 1073741824 / sizeof(signed short) ),
                      "Values out of ranges for sound sample (%s), probably a corrupted file\n"
                      "Num Channels(%u), Num Samples(%u), Freq(%u), Max Raw Sample Size(%u)",
                      _sampleName, m_numChannels, m_numSamples, m_freq, 1073741824 / sizeof(signed short) );

	m_rawSampleData = new signed short[m_numChannels * m_numSamples];
}


CachedSample::~CachedSample()
{
	if( m_lockedCount > 0 )
	{
		AppDebugOut( "Warning: Deleting locked cached sample %s\n", m_sampleName.c_str() );
	}

	delete m_soundStreamDecoder; m_soundStreamDecoder = NULL;
	delete [] m_rawSampleData; m_rawSampleData = NULL;
}

unsigned CachedSample::GetMemoryUsage() const
{
	return sizeof(signed short) * m_numChannels * m_numSamples;
}

void CachedSample::Lock()
{
	m_lockedCount++;
}

void CachedSample::Unlock()
{
	if( m_lockedCount <= 0 )
	{
		AppDebugOut("Warning: Attempt to unlock cached sample %s more times than it has been locked\n", m_sampleName.c_str() );
		return;
	}

	m_lockedCount--;

	if( m_lockedCount == 0 )
	{
		// Return the last used time
		m_lastUsedTime = GetHighResTime();
	}
}


double CachedSample::LastUsedTime() const
{
	return m_lastUsedTime;
}

bool CachedSample::Locked() const
{
	return m_lockedCount > 0;
}

#ifdef TARGET_OS_MACOSX
void CachedSample::Preload(bool lowcpu)
{
	m_loadState = kSampleLoadState_Loading;
	unsigned int startSample = 0;
	unsigned int numSamples = 2048;	// load the sound in 2048 samples at a time...
	if(lowcpu)
	{
		numSamples = 512;
	}
	if(m_numSamples)// < m_freq * 5)
	{
#ifdef VERBOSE_DEBUG
		AppDebugOut("Preloading sound: %s, %s\n", m_sampleName.c_str(), m_name);
#endif
		while(m_soundStreamDecoder)
		{
			Read(NULL, startSample, numSamples);
			startSample += numSamples;
			if(lowcpu)
			{
				pthread_yield_np();	// we yield to other threads (mainly we're worried about the rendering thread)
			}
		}
	}
	else
	{
		AppDebugOut("Skipping long sound: %s, %s\n", m_sampleName.c_str(), m_name);	
	}
	m_loadState = kSampleLoadState_Loaded;
}
#endif

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
			AppDebugAssert(m_amountCached <= m_numSamples);
			if (m_amountCached == m_numSamples)
			{
				delete m_soundStreamDecoder; m_soundStreamDecoder = NULL;
			}
		}
	}

	AppReleaseAssert(m_numChannels == 1, "%s is not mono audio\n", m_name);
	if(_data)
		memcpy(_data, &m_rawSampleData[_startSample], sizeof(signed short) * 1 * _numSamples);
}



//*****************************************************************************
// Class CachedSampleHandle
//*****************************************************************************

CachedSampleHandle::CachedSampleHandle(CachedSample *_sample)
:	m_nextSampleIndex(0),
	m_cachedSample(_sample)
{
	m_cachedSample->Lock();
}


CachedSampleHandle::~CachedSampleHandle()
{
	m_cachedSample->Unlock();
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
	EmptyCache();
}


CachedSampleHandle *CachedSampleManager::GetSample(char const *_sampleName)
{
	NetLockMutex lock( m_lock );
	CachedSample *&cachedSample = m_cache[_sampleName];

	if( !cachedSample )
	{
		cachedSample = new CachedSample(_sampleName);
    }

	return new CachedSampleHandle( cachedSample );
}

void CachedSampleManager::CleanUp()
{
	// Largest song takes 13 MB
#ifdef TARGET_OS_MACOSX
	// LRU is disabled for mac build
	const unsigned maxSize = 0;//60 * 1024 * 1024; 
#else
	const unsigned maxSize = 60 * 1024 * 1024; 
#endif
	
	if(maxSize)
	{
		unsigned totalMemory, unusedMemory;
		NetLockMutex lock( m_lock );
		
		GetMemoryUsage( totalMemory, unusedMemory );
		while( totalMemory > maxSize )
		{
			CSHashTable::iterator sampleToEvict = PickSampleToEvict();
			if( sampleToEvict == m_cache.end() )
				return;
			
			CachedSample *sample = sampleToEvict->second;
			
			totalMemory -= sample->GetMemoryUsage(); 
			delete sample; 
			
			m_cache.erase( sampleToEvict );
		}
	}
}

CachedSampleManager::CSHashTable::iterator CachedSampleManager::PickSampleToEvict()
{
	// Choose a sample to evict based on least recently used
	double timeNow = GetHighResTime();
	double bestLastUsedTime = timeNow;
	
	CSHashTable::iterator end = m_cache.end();
	CSHashTable::iterator bestSampleIndex = end;
	for(CSHashTable::iterator i = m_cache.begin(); i != end; i++ )
	{
        CachedSample *sample = i->second;
		if( !sample->Locked() )
		{
			if( sample->LastUsedTime() < bestLastUsedTime )
			{
				bestLastUsedTime = sample->LastUsedTime();
				bestSampleIndex = i;
			}
		}
	}
	return bestSampleIndex;
}


void CachedSampleManager::EmptyCache()
{
	NetLockMutex lock( m_lock );
	
	CSHashTable::iterator end = m_cache.end();
	for(CSHashTable::iterator i = m_cache.begin(); i != end; i++ )
	{
		delete i->second;
	}
	m_cache.clear();
}


void CachedSampleManager::GetMemoryUsage( unsigned &_totalMemoryUsage, unsigned &_unusedMemoryUsage  )
{
    unsigned totalMemoryUsage = 0;
	unsigned unusedMemoryUsage = 0;
	NetLockMutex lock( m_lock );

	CSHashTable::iterator end = m_cache.end();
	for(CSHashTable::iterator i = m_cache.begin(); i != end; i++ )
	{
        CachedSample *sample = i->second;
			
        unsigned sampleSize = sample->GetMemoryUsage();
        totalMemoryUsage += sampleSize;           
		if( !sample->Locked() )
		{
			unusedMemoryUsage += sampleSize;
		}
	}

    _totalMemoryUsage = totalMemoryUsage;
	_unusedMemoryUsage = unusedMemoryUsage;
}
