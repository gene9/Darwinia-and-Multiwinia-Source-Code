#include "lib/universal_include.h"

#include <stdlib.h>
#include <string.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>

#include "lib/filesys/binary_stream_readers.h"
#include "lib/debug_utils.h"

#include "sound/sound_stream_decoder.h"



SoundStreamDecoder::SoundStreamDecoder(BinaryReader *_in)
:	m_in(_in),
	m_bits(0),
	m_numChannels(0),
	m_freq(0),
	m_numSamples(0),
	m_fileType(TypeUnknown),
    m_vorbisFile(NULL)
{
	const char *fileType = _in->GetFileType();
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
		AppReleaseAssert(0, "Unknown sound file format %s", m_in->m_filename.c_str());
	}
}


SoundStreamDecoder::~SoundStreamDecoder()
{
	delete m_in;

    if( m_vorbisFile )
    {
        ov_clear( m_vorbisFile );
        delete m_vorbisFile;
    }
}


void SoundStreamDecoder::ReadWavHeader()
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
			
			m_bits = m_in->ReadS16();    // 8 or 16 bit data? 
			chunkLength -= 2;
			if ((m_bits != 8) && (m_bits != 16))
			{
				return;
			}
		}
		else if (memcmp(buffer, "data", 4) == 0)
		{
			m_dataStartOffset = m_in->Tell();

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
	int rv = in->Seek(_offset, _origin);
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


void SoundStreamDecoder::ReadOggHeader()
{
	ov_callbacks callbacks;
	callbacks.read_func = ReadFunc;
	callbacks.seek_func = SeekFunc;
	callbacks.close_func = CloseFunc;
	callbacks.tell_func = TellFunc;

	m_vorbisFile = new OggVorbis_File;
	int result = ov_open_callbacks(m_in, m_vorbisFile, NULL, 0, callbacks);
	AppReleaseAssert(result == 0, "Ogg file corrupt %s (result = %d)", m_in->m_filename.c_str(), result);

	vorbis_info *vi = ov_info(m_vorbisFile, -1);

	m_numSamples = ov_pcm_total(m_vorbisFile, -1);
	AppReleaseAssert(m_numSamples > 0, "Ogg file contains no data %s", m_in->m_filename.c_str());
	m_freq = vi->rate;
	m_numChannels = vi->channels;
}


unsigned int SoundStreamDecoder::ReadWavData(signed short *_data, unsigned int _numSamples)
{
	if (_numSamples > m_samplesRemaining)
	{
		_numSamples = m_samplesRemaining;
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
//		float prevVal = 0.0f;
//		float smooth = iv_sqrt(0.9f);
//		float oneMinusSmooth = 1.0f - smooth;
//		for (int i = 0; i < _numSamples; ++i)
//		{
//			float newVal = sfrand(64000.0f);
//			newVal = newVal * oneMinusSmooth + prevVal * smooth;
//			_data[i] = Round(newVal);
//			prevVal = newVal;
//		}
	}

	m_samplesRemaining -= _numSamples;
	return _numSamples;
}

static int host_is_big_endian() 
{
	// (from vorbis_file.cpp)
	int pattern = 0xfeedface; /* deadbeef */
	unsigned char *bytewise = (unsigned char *)&pattern;
	if (bytewise[0] == 0xfe) return 1;
	return 0;
}

unsigned int SoundStreamDecoder::ReadOggData(signed short *_data, unsigned int _numSamples)
{
	bool eof = false;
	int samplesLeftToRead = _numSamples;
	char *buf = (char *)_data;

	while (samplesLeftToRead > 0 && !eof)
	{
		int currentSection;
		int numBytesToRead = samplesLeftToRead * 2;
		int bytesRead = ov_read(m_vorbisFile, buf, numBytesToRead, host_is_big_endian(),
								2 /*16 bit*/, 1 /*signed*/, &currentSection);
		AppReleaseAssert(bytesRead != OV_HOLE && bytesRead != OV_EBADLINK,
			"Ogg file corrupt %s", m_in->m_filename.c_str());

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


unsigned int SoundStreamDecoder::Read(signed short *_data, unsigned int _numSamples)
{
	switch( m_fileType )
	{
	case TypeUnknown:
		AppReleaseAssert(0, "Unknown format of sound file %s", m_in->m_filename.c_str());
	case TypeWav:
		return ReadWavData(_data, _numSamples);
	case TypeOgg:
		return ReadOggData(_data, _numSamples);
	}

	AppDebugAssert(0);
	return 0;
}


void SoundStreamDecoder::Restart()
{
	if (m_fileType == TypeWav)
	{
		m_in->Seek(m_dataStartOffset, SEEK_SET);
		m_samplesRemaining = m_numSamples;
	}
	else
	{
		ov_raw_seek(m_vorbisFile, 0);
	}
}
