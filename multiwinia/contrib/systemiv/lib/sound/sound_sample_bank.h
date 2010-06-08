#ifndef INCLUDED_SOUND_SAMPLE_BANK_H
#define INCLUDED_SOUND_SAMPLE_BANK_H

/*
 *	A collection of all the sound samples that have been loaded.
 *  Apps should use the SoundSampleBank as the primary method for
 *  opening Sound Samples.
 *
 */

#include "lib/tosser/hash_table.h"

class SoundSampleHandle;
class SoundSampleDecoder;



//*****************************************************************************
// Class SoundSampleBank
//*****************************************************************************

class SoundSampleBank
{
public:
	HashTable <SoundSampleDecoder *> m_cache;

public:
    SoundSampleBank     ();
	~SoundSampleBank    ();

    void EmptyCache     ();                                             // Deletes all cached sample data
    int GetMemoryUsage  ();

	SoundSampleHandle  *GetSample(char *_sampleName);	                // Delete the returned object when you are finished with it

    void DiscardSample(char *_sampleName);                              // Be SURE nobody has a SoundSampleHandle pointing to the sample
};




//*****************************************************************************
// Class SoundSampleHandle
//*****************************************************************************

class SoundSampleHandle
{
protected:
	unsigned int m_nextSampleIndex;			                            // Index of first sample to return when Read is called

public:
	SoundSampleDecoder *m_soundSample;

public:
	SoundSampleHandle(SoundSampleDecoder *_sample);
	~SoundSampleHandle();

	unsigned int Read(signed short *_data, unsigned int _numSamples, bool _stereo, float _relFreq);

    int NumSamplesRemaining();
    void Restart();
};



extern SoundSampleBank *g_soundSampleBank;


#endif
