#ifndef INCLUDED_SOUND_STREAM_DECODER_H
#define INCLUDED_SOUND_STREAM_DECODER_H


class BinaryReader;
struct OggVorbis_File;


//*****************************************************************************
// Class SoundStreamDecoder
//*****************************************************************************

// Reads sound data from a wav or ogg file. The output is always 16 bit signed.
// If the file contains 8 bit data it gets converted into 16 bit data. It 
// assumes that 8 bit wav files are unsigned and that 16 bit wav files are signed.

class SoundStreamDecoder
{
protected:
	BinaryReader	*m_in;

	OggVorbis_File	*m_vorbisFile;		// Ogg only
	
	unsigned int	m_samplesRemaining;	// Wav only
	unsigned int	m_dataStartOffset;	// Wav only - bytes from start of file

	unsigned char	m_bits;				// 8 or 16 - Indicates source file format - output is always 16 bit
	int				m_fileType;

	void ReadWavHeader();
	void ReadOggHeader();
	unsigned ReadWavData(signed short *_data, unsigned int _numSamples);
	unsigned ReadOggData(signed short *_data, unsigned int _numSamples);

public:
	unsigned int	m_numChannels;
	unsigned int	m_freq;
	unsigned int	m_numSamples;
	
	enum
	{
		TypeUnknown,
		TypeWav,
		TypeOgg
	};

	SoundStreamDecoder(BinaryReader *_in);
	~SoundStreamDecoder();

	unsigned int Read(signed short *_data, unsigned int _numSamples);
	void Restart();
};



#endif
