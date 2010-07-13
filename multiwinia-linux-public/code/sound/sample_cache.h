#ifndef INCLUDED_SAMPLE_CACHE_H
#define INCLUDED_SAMPLE_CACHE_H

//#include "lib/tosser/hash_table.h"
#ifdef TARGET_OS_MACOSX
	#include "hash_map_darwin.h"
#else
#ifdef TARGET_OS_LINUX
	#include "lib/tosser/hash_map_darwin.h"
#else
	#include <hash_map>
	using stdext::hash_map;
#endif
#endif

class SoundStreamDecoder;

#ifdef TARGET_OS_MACOSX
enum
{
	kSampleLoadState_NotLoaded = 0,
	kSampleLoadState_Loading,
	kSampleLoadState_Loaded
} typedef SampleLoadState;
#endif

//*****************************************************************************
// Class CachedSample
//*****************************************************************************

class CachedSample
{
protected:
	SoundStreamDecoder *m_soundStreamDecoder;	// NULL once sample has been read fully once
	signed short	*m_rawSampleData;
	unsigned int	m_amountCached;				// Zero at first, ranging up to m_numSamples once sample has been read fully
	char			*m_name;

	int				m_lockedCount;
	double			m_lastUsedTime;
	std::string		m_sampleName;

#ifdef TARGET_OS_MACOSX
	SampleLoadState				m_loadState;
#endif
public:
	
	void		Lock();							// Increases usage count
	void		Unlock();						// Decreases usage count

	bool		Locked() const;
	double		LastUsedTime() const;

	unsigned	GetMemoryUsage() const;

	unsigned int	m_numChannels;
	unsigned int	m_freq;
	unsigned int	m_numSamples;

#ifdef TARGET_OS_MACOSX
	void Preload(bool lowcpu);
	inline SampleLoadState LoadState()	{	return m_loadState;	}
#endif
		
	CachedSample(char const *_sampleName);
	~CachedSample();

	void Read(signed short *_data, unsigned int _startSample, unsigned int _numSamples);
};


//*****************************************************************************
// Class CachedSampleHandle
//*****************************************************************************

class CachedSampleHandle
{
protected:
	unsigned int	m_nextSampleIndex;			// Index of first sample to return when Read is called

public:
	CachedSample	*m_cachedSample;

	CachedSampleHandle(CachedSample *_sample);
	~CachedSampleHandle();

	unsigned int Read(signed short *_data, unsigned int _numSamples);
	void Restart();
};


//*****************************************************************************
// Class CachedSampleManager
//*****************************************************************************

class CachedSampleManager
{
protected:
	//HashTable <CachedSample *>	m_cache;

	typedef hash_map<std::string, CachedSample *> CSHashTable;
	CSHashTable m_cache;
	NetMutex m_lock;

	//int PickSampleToEvict();

	CSHashTable::iterator PickSampleToEvict();


public:
	~CachedSampleManager();

	CachedSampleHandle *GetSample(char const *_sampleName);	// Delete the returned object when you are finished with it

	void CleanUp();

    void EmptyCache();              // Deletes all cached sample data

    void GetMemoryUsage( unsigned &_totalMemoryUsage, unsigned &_unusedMemoryUsage  );
};


extern CachedSampleManager g_cachedSampleManager;

extern bool g_deletingCachedSampleHandle;


#endif
