#ifndef INCLUDED_SOUND_LIBRARY_2D
#define INCLUDED_SOUND_LIBRARY_2D


#include <stdio.h>
#include <MMSystem.h>


//*****************************************************************************
// Class StereoSample
//*****************************************************************************

class StereoSample
{
public:
	signed short m_left;
	signed short m_right;

	StereoSample(): m_left(0), m_right(0) {}
};


//*****************************************************************************
// Class SoundLib2dBuf
//*****************************************************************************

class SoundLib2dBuf
{
public:
	StereoSample	*m_buffer;
	WAVEHDR			m_header;

	SoundLib2dBuf();
	~SoundLib2dBuf();
};


//*****************************************************************************
// Class SoundLibrary2d
//*****************************************************************************

class SoundLibrary2d
{
protected:
	FILE			*m_wavOutput;
	SoundLib2dBuf	*m_buffers;
	unsigned int	m_numBuffers;
	unsigned int	m_nextBuffer;		// Index of next buffer to send to sound card

public:
	unsigned int	m_fillsRequested;	// Number of outstanding requests for more sound data that Windows has issued
	unsigned int	m_freq;
	unsigned int	m_samplesPerBuffer;
    void			(*m_callback) (StereoSample *buf, unsigned int numSamples);

	void			Stop();
	
public:
	SoundLibrary2d();
	~SoundLibrary2d();

    void			SetCallback(void (*_callback) (StereoSample *, unsigned int));
	void			TopupBuffer();

	void			StartRecordToFile(char const *_filename);
	void			EndRecordToFile();
};


extern SoundLibrary2d *g_soundLibrary2d;


#endif
