#ifndef INCLUDED_SOUND_LIBRARY_2D_SDL
#define INCLUDED_SOUND_LIBRARY_2D_SDL

#include "sound_library_2d.h"
#include "lib/netlib/net_mutex.h"

//*****************************************************************************
// Class SoundLibrary2dSDL
//*****************************************************************************

class SoundLibrary2dSDL : public SoundLibrary2d
{
private:
	class Buffer {
	public:
		Buffer()
			: stream(NULL), len(0)
		{
		}
		
		StereoSample *stream;
		int len;
	};
	
	int				m_bufferIsThirsty;
	Buffer			m_buffer[2];
    void			(*m_callback) (StereoSample *buf, unsigned int numSamples);
	FILE            *m_wavOutput;

public:

	NetMutex		m_callbackLock;

	void			AudioCallback(StereoSample *buf, unsigned int numSamples);
	
	void			Stop();
	
public:
	SoundLibrary2dSDL();
	~SoundLibrary2dSDL();

	void			SetCallback(void (*_callback) (StereoSample *, unsigned int));
	void			TopupBuffer();
	
	void            StartRecordToFile(char const *_filename);
	void            EndRecordToFile();
	
	unsigned		GetSamplesPerBuffer();
	unsigned		GetFreq();

	
};


extern SoundLibrary2d *g_soundLibrary2d;


#endif
