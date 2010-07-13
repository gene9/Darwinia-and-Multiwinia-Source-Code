#include "lib/universal_include.h"

#include <stdlib.h>
#include <string.h>

#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>

#include "lib/filesys/binary_stream_readers.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/debug_utils.h"

#include "soundsystem.h"
#include "sound_sample_decoder.h"



SoundSampleDecoder::SoundSampleDecoder(BinaryReader *_in)
:	m_in(_in),
	m_bits(0),
	m_numChannels(0),
	m_freq(0),
	m_numSamples(0),
	m_fileType(TypeUnknown),
    m_vorbisFile(NULL),
    m_sampleCache(NULL)
{
	char *fileType = _in->GetFileType();
	if (stricmp(fileType, "wav") == 0)
	{
		m_fileType = TypeWav;
		ReadWavHeader();
	}
	else if (stricmp(fileType, "ogg") == 0)
	{
		m_fileType = TypeOgg;
		ReadOggHeader();
	}
	else
	{
		AppReleaseAssert(0, "Unknown sound file format %s", m_in->m_filename);
	}


    // We assume a single sample is ALWAYS mono
    // therefore a stereo file has twice as many samples inside as it says it does

    m_numSamples *= m_numChannels;
	m_amountCached = 0;
}


SoundSampleDecoder::~SoundSampleDecoder()
{
    if( m_in )
    {
	    delete m_in;
    }

    if( m_vorbisFile )
    {
        ov_clear( m_vorbisFile );
        delete m_vorbisFile;
    }

    if( m_sampleCache )
    {
        delete m_sampleCache;
    }
}


void SoundSampleDecoder::ReadWavHeader()
{
	char buffer[25];
	int chunkLength;
	
	// Check RIFF header 
	m_in->ReadBytes(12, (unsigned char *)buffer);
	if (memcmp(buffer, "RIFF", 4) || memcmp(buffer + 8, "WAVE", 4))
	{
		return;
	}
	
	while (!m_in->m_eof) 
	{
		if (m_in->ReadBytes(4, (unsigned char*)buffer) != 4)
		{
			break;
		}
		
		chunkLength = m_in->ReadS32();   // read chunk length 
		
		if (memcmp(buffer, "fmt ", 4) == 0)
		{
			int i = m_in->ReadS16();     // should be 1 for PCM data 
			chunkLength -= 2;
			if (i != 1) 
			{
				return;
			}
			
			m_numChannels = m_in->ReadS16();// mono or stereo data 
			chunkLength -= 2;
			if ((m_numChannels != 1) && (m_numChannels != 2))
			{
				return;
			}
			
			m_freq = m_in->ReadS32();    // sample frequency 
			chunkLength -= 4;
			
			m_in->ReadS32();             // skip six bytes 
			m_in->ReadS16();
			chunkLength -= 6;
			
			m_bits = (unsigned char) m_in->ReadS16();    // 8 or 16 bit data? 
			chunkLength -= 2;
			if ((m_bits != 8) && (m_bits != 16))
			{
				return;
			}
		}
		else if (memcmp(buffer, "data", 4) == 0)
		{
			int bytesPerSample = m_numChannels * m_bits / 8;
			m_numSamples = chunkLength / bytesPerSample;

			m_samplesRemaining = m_numSamples;

			return;
		}
		
		// Skip the remainder of the chunk 
		while (chunkLength > 0) 
		{             
			if (m_in->ReadU8() == EOF)
			{
				break;
			}
			
			chunkLength--;
		}
	}
}

static size_t ReadFunc(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	BinaryReader *in = (BinaryReader *)datasource;
	return in->ReadBytes(size * nmemb, (unsigned char*)ptr);
}


static int SeekFunc(void *datasource, ogg_int64_t _offset, int _origin)
{
	BinaryReader *in = (BinaryReader *)datasource;
	int rv = in->Seek((int)_offset, _origin);
	return rv; // -1 indicates seek not supported
}


static int CloseFunc(void *datasource)
{
	return 0;
}


static long TellFunc(void *datasource)
{
	BinaryReader *in = (BinaryReader *)datasource;
	int rv = in->Tell();
	return rv;
}


void SoundSampleDecoder::ReadOggHeader()
{
	ov_callbacks callbacks;
	callbacks.read_func = ReadFunc;
	callbacks.seek_func = SeekFunc;
	callbacks.close_func = CloseFunc;
	callbacks.tell_func = TellFunc;

	m_vorbisFile = new OggVorbis_File();
	int result = ov_open_callbacks(m_in, m_vorbisFile, NULL, 0, callbacks);
	AppReleaseAssert(result == 0, "Ogg file corrupt %s (result = %d)", m_in->m_filename, result);

	vorbis_info *vi = ov_info(m_vorbisFile, -1);

	m_numSamples = (unsigned int) ov_pcm_total(m_vorbisFile, -1);
	AppReleaseAssert(m_numSamples > 0, "Ogg file contains no data %s", m_in->m_filename);
	m_freq = vi->rate;
	m_numChannels = vi->channels;
}


unsigned int SoundSampleDecoder::ReadWavData(signed short *_data, unsigned int _numSamples)
{
    if( m_amountCached + _numSamples > m_numSamples )
    {
        _numSamples = m_numSamples - m_amountCached;
    }

	if (m_bits == 8)
	{
		for (unsigned int i = 0; i < _numSamples; ++i)
		{
			signed short c = m_in->ReadU8() - 128;
			c <<= 8;
			_data[i] = c;

			if (m_in->m_eof)
			{
				_numSamples = i;
				break;
			}
		}
	}
	else 
	{
		int bytesPerSample = 2 * m_numChannels;
		_numSamples = m_in->ReadBytes(_numSamples * bytesPerSample, (unsigned char*)_data);
        _numSamples /= 2;
	}

	return _numSamples;
}

#ifdef _BIG_ENDIAN
#define IS_BIG_ENDIAN 1
#else
#define IS_BIG_ENDIAN 0
#endif

unsigned int SoundSampleDecoder::ReadOggData(signed short *_data, unsigned int _numSamples)
{
	bool eof = false;
	int samplesLeftToRead = _numSamples;
	char *buf = (char *)_data;

	while (samplesLeftToRead > 0 && !eof)
	{
		int currentSection;
		int numBytesToRead = samplesLeftToRead * 2;
		int bytesRead = ov_read(m_vorbisFile, buf, numBytesToRead, IS_BIG_ENDIAN,
								2 /*16 bit*/, 1 /*signed*/, &currentSection);
		AppReleaseAssert(bytesRead != OV_HOLE && bytesRead != OV_EBADLINK,
			"Ogg file corrupt %s", m_in->m_filename);

		if (bytesRead == 0)
		{
			eof = true;
		}

		int samplesRead = bytesRead / 2;
		samplesLeftToRead -= samplesRead;

		buf += bytesRead;
	}

	return _numSamples - samplesLeftToRead;
}


unsigned int SoundSampleDecoder::ReadToCache(unsigned int _numSamples)
{
    if( !m_sampleCache ) m_sampleCache = new signed short[m_numSamples];

    int samplesRead = 0;

    switch( m_fileType )
    {
	    case TypeUnknown:   AppReleaseAssert(0, "Unknown format of sound file %s", m_in->m_filename);
	    case TypeWav:       samplesRead = ReadWavData(&m_sampleCache[m_amountCached], _numSamples);         break;
	    case TypeOgg:       samplesRead = ReadOggData(&m_sampleCache[m_amountCached], _numSamples);         break;
    }

    m_amountCached += samplesRead;	
	return samplesRead;
}


void SoundSampleDecoder::EnsureCached(unsigned int _endSample)
{
    if (_endSample >= m_amountCached)
	{
		int amountToRead = _endSample - m_amountCached;
		unsigned int amountRead = ReadToCache(amountToRead);                    

		AppDebugAssert(m_amountCached <= m_numSamples);

		if (m_amountCached == m_numSamples)
		{
	        delete m_in;
            m_in = NULL;

            if( m_vorbisFile )
            {
                ov_clear( m_vorbisFile );
                delete m_vorbisFile;
                m_vorbisFile = NULL;
            }		
		}
	}
}


void SoundSampleDecoder::InterpolateSamples( signed short *_data, unsigned int _startSample, unsigned int _numSamples, bool _stereo, float _relFreq)
{
    /*
     *  We are trying to copy the sample data into the buffer, but at a certain Relative Frequency
     *  We are NOT using Direct Sound's channel frequency code, which means we have to do the frequency work ourselves.
     * 
     *  Doing a simple "nearest sample" calculation results in serious artifacts in the audio - like a Pixel Resize.
     *  Here we attempt to do a filtered approach - by taking the nearest sample, the previous sample, and the next sample
     *  and merging them together in the correct ratio.
     *
     *  It still doesn't sound as good as Direct Sound.
     *
     */

    if( _stereo )
    {
        for( int i = 0; i < _numSamples; ++i )
        {
            float samplePoint = (float)i * _relFreq;
            int sampleIndex = int(samplePoint);
            if( i % 2 == 0 && sampleIndex % 2 == 1 ) 
            {
                sampleIndex--;
                samplePoint -= 1.0f;
            }

            if( i % 2 == 1 && sampleIndex % 2 == 0 ) 
            {
                sampleIndex++;
                samplePoint += 1.0f;
            }

            float fraction = samplePoint - sampleIndex;
            float sample1, sample2, sample3;

            if( fraction > 0.5f )
            {
                sample1 = m_sampleCache[_startSample + sampleIndex - 2] * fraction +
                          m_sampleCache[_startSample + sampleIndex] * (1.0f-fraction);

                sample2 = m_sampleCache[_startSample + sampleIndex + 2] * (1.0f - fraction) +
                          m_sampleCache[_startSample + sampleIndex] * fraction;

                sample3 = m_sampleCache[_startSample + sampleIndex + 4] * (1.0f - fraction) +
                          m_sampleCache[_startSample + sampleIndex + 2] * fraction;
            }
            else
            {
                sample1 = m_sampleCache[_startSample + sampleIndex - 4] * fraction +
                          m_sampleCache[_startSample + sampleIndex - 2] * (1.0f-fraction);

                sample2 = m_sampleCache[_startSample + sampleIndex] * (1.0f - fraction) +
                          m_sampleCache[_startSample + sampleIndex - 2] * fraction;

                sample3 = m_sampleCache[_startSample + sampleIndex + 2] * (1.0f - fraction) +
                          m_sampleCache[_startSample + sampleIndex] * fraction;
            }

            float combinedSample = sample1 * 0.2f + sample2 * 0.6f + sample3 * 0.2f;

            _data[i] = (signed short)combinedSample;
        }
    }
    else
    {        
        memcpy( _data, &m_sampleCache[_startSample], sizeof(signed short) * _numSamples);                
    }
}

void SoundSampleDecoder::Read(signed short *_data, unsigned int _startSample, unsigned int _numSamples, bool _stereo, float _relFreq)
{
    /*
     *	STEREO SAMPLE SUPPORT
     *  
     *  Its all a bit confusing really
     *  but basically, if you request 10 samples of a mono file you'll get the next 10 samples
     *  if you request 10 samples of a stereo sample you'll still get 10 samples - but they will
     *  be left-right-left-right etc, ie only 5 stereo samples worth.
     *
     *  Direct Sound expects the data in this format, so its cool.
     *
     *  If you try to play a mono sample onto a stereo channel the code below will copy the
     *  mono sample into the left-right-left-right stream (ie double up the data)
     *
     */

    unsigned int highestSampleRequested = _startSample + _numSamples;
    EnsureCached( highestSampleRequested );

    bool stereoMatch = ( m_numChannels == 1 && !_stereo ) ||
                       ( m_numChannels == 2 && _stereo );

    if( stereoMatch )
    {
#ifdef SOUND_USE_DSOUND_FREQUENCY_STUFF
        memcpy( _data, &m_sampleCache[_startSample], sizeof(signed short) * _numSamples);               
#else
        InterpolateSamples( _data, _startSample, _numSamples, _stereo, _relFreq );
#endif
    }
    else if( m_numChannels == 1 && _stereo )
    {
        // Mix our mono sample data into a stereo buffer
        for( int i = 0; i < _numSamples; ++i )
        {
            _data[i] = m_sampleCache[int((_startSample+i)/2)];     
        }
    }
    else if( m_numChannels == 2 && !_stereo )
    {
        // Mix our stereo sample data into a mono buffer

        highestSampleRequested = _startSample*2 + _numSamples*2 - 1;
        EnsureCached( highestSampleRequested );

        for( int i = 0; i < _numSamples; i++ )
        {
            signed short sample1 = m_sampleCache[_startSample*2+i*2];
            signed short sample2 = m_sampleCache[_startSample*2+i*2+1];
            signed short sampleCombined = (signed short)( ( (int)sample1 + (int)sample2 ) / 2.0f );            

            _data[i] = sampleCombined;            
        }        
    }
}

