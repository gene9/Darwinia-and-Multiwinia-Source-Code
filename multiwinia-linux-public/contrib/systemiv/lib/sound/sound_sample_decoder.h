#ifndef INCLUDED_SOUND_SAMPLE_DECODER_H
#define INCLUDED_SOUND_SAMPLE_DECODER_H

//*****************************************************************************
// Class SoundSampleDecoder
//*****************************************************************************

// Reads sound data from a wav or ogg file. 
// Stores the data in a cache - ie decoding only occurs first time through.
// The output is always 16 bit signed.
// If the file contains 8 bit data it gets converted into 16 bit data. It 
// assumes that 8 bit wav files are unsigned and that 16 bit wav files are signed.


class BinaryReader;
struct OggVorbis_File;



class SoundSampleDecoder
{
protected:
	BinaryReader	*m_in;
	OggVorbis_File	*m_vorbisFile;		// Ogg only	
	unsigned int	m_samplesRemaining;	// Wav only
    unsigned char	m_bits;				// 8 or 16 - Indicates source file format - output is always 16 bit
	int				m_fileType;

	enum
	{
		TypeUnknown,
		TypeWav,
		TypeOgg
	};

	void        ReadWavHeader   ();
	void        ReadOggHeader   ();
	unsigned    ReadWavData     (signed short *_data, unsigned int _numSamples);
	unsigned    ReadOggData     (signed short *_data, unsigned int _numSamples);

	unsigned int ReadToCache    (unsigned int _numSamples);
    void        EnsureCached    (unsigned int _endSample);

public:
	unsigned int	m_numChannels;
	unsigned int	m_freq;
	unsigned int	m_numSamples;
	
    signed short	*m_sampleCache;             // Cache of all data read
	unsigned int	m_amountCached;				// Zero at first, ranging up to m_numSamples once sample has been read fully
	
public:
	SoundSampleDecoder(BinaryReader *_in);
	~SoundSampleDecoder();

    void InterpolateSamples(signed short *_data, unsigned int _startSample, 
                            unsigned int _numSamples, bool _stereo, float _relFreq);

	void Read(signed short *_data, unsigned int _startSample, 
              unsigned int _numSamples, bool _stereo, float _relFreq);
};




#endif
