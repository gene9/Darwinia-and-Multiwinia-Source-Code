#ifndef INCLUDED_SAMPLE_CACHE_H
#define INCLUDED_SAMPLE_CACHE_H

#include "lib/hash_table.h"


class SoundStreamDecoder;


//*****************************************************************************
// Class CachedSample
//*****************************************************************************

class CachedSample
{
protected:
	SoundStreamDecoder *m_soundStreamDecoder;	// NULL once sample has been read fully once
	signed short	*m_rawSampleData;
	unsigned int	m_amountCached;				// Zero at first, ranging up to m_numSamples once sample has been read fully

public:
	unsigned int	m_numChannels;
	unsigned int	m_freq;
	unsigned int	m_numSamples;

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
	HashTable <CachedSample *>	m_cache;

public:
	~CachedSampleManager();

	CachedSampleHandle *GetSample(char const *_sampleName);	// Delete the returned object when you are finished with it

    void EmptyCache();              // Deletes all cached sample data

    int GetMemoryUsage();
};


extern CachedSampleManager g_cachedSampleManager;

extern bool g_deletingCachedSampleHandle;


#endif
