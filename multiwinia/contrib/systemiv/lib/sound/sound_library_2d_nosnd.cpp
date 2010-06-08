#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "lib/system_info.h"

#include "sound/sound_library_2d.h"

/*
static HWAVEOUT	s_device;
static WAVEHDR s_buffer1Header;
static WAVEHDR s_buffer2Header;
*/

static int s_nextBuffer;


double tim;

SoundLibrary2d *g_soundLibrary2d = NULL;


//FILE *out = NULL;
#include "app.h"
#include "sound/soundsystem.h"
#include "lib/hi_res_time.h"

/*
void CALLBACK WaveOutProc(HWAVEOUT _dev, UINT _msg, DWORD _userData, DWORD _param1, DWORD _param2)
{
	if (_msg != WOM_DONE) return;
	if (!s_device)	return;
	if (!g_soundLibrary2d || !g_soundLibrary2d->m_callback) return;

	g_soundLibrary2d->m_bufferIsThirsty++;
	tim = GetHighResTime();

	if (g_app->m_soundSystem && !g_app->m_soundSystem->m_advancing)
	{
		g_soundLibrary2d->TopupBuffer();
	}
}
*/

void SoundLibrary2d::TopupBuffer()
{
	while (m_bufferIsThirsty)
	{


		if (s_nextBuffer == 1)
		{
			g_soundLibrary2d->m_callback(g_soundLibrary2d->m_buffer1, g_soundLibrary2d->m_samplesPerBuffer);
			/*
			int result = waveOutWrite(s_device, &s_buffer1Header, sizeof(WAVEHDR));
			DarwiniaReleaseAssert(result == MMSYSERR_NOERROR, "Couldn't send sound data 2");
			*/
			s_nextBuffer = 2;
		}
		else
		{
			g_soundLibrary2d->m_callback(g_soundLibrary2d->m_buffer2, g_soundLibrary2d->m_samplesPerBuffer);
			/*
			int result = waveOutWrite(s_device, &s_buffer2Header, sizeof(WAVEHDR));
			DarwiniaReleaseAssert(result == MMSYSERR_NOERROR, "Couldn't send sound data 2");
			*/
			s_nextBuffer = 1;
		}

		m_bufferIsThirsty--;
	}
}


SoundLibrary2d::SoundLibrary2d()
:	m_callback(NULL),
	m_bufferIsThirsty(0)
{
	DarwiniaReleaseAssert(!g_soundLibrary2d, "SoundLibrary2d already exists");

	m_freq = g_prefsManager->GetInt("SoundMixFreq", 44100);
	m_samplesPerBuffer = g_prefsManager->GetInt("SoundBufferSize", 2000);

	m_buffer1 = new StereoSample[m_samplesPerBuffer];
	m_buffer2 = new StereoSample[m_samplesPerBuffer];
	memset(m_buffer1, 0, m_samplesPerBuffer * sizeof(StereoSample));
	memset(m_buffer2, 0, m_samplesPerBuffer * sizeof(StereoSample));


	//
	// Initialise the output device

	/*
	WAVEFORMATEX format;
	memset(&format, 0, sizeof(WAVEFORMATEX));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.nSamplesPerSec = m_freq;
	format.wBitsPerSample = 16;
	format.nBlockAlign = 4;		// 2 channels * 2 bytes per sample
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	int result = waveOutOpen(&s_device, WAVE_MAPPER, &format, (DWORD)&WaveOutProc, 0, CALLBACK_FUNCTION);
	char const *errString = NULL;
	switch (result)
	{
		case MMSYSERR_ALLOCATED:	errString = "Specified resource is already allocated";	break;
		case MMSYSERR_BADDEVICEID:	errString = "Specified device ID is out of range";		break;
		case MMSYSERR_NODRIVER:		errString = "No device driver is present";				break;
		case MMSYSERR_NOMEM:		errString = "Unable to allocate or lock memory";		break;
		case WAVERR_BADFORMAT:		errString = "Attempted to open with an unsupported waveform-audio format";	break;
		case WAVERR_SYNC:			errString = "Device is synchronous but waveOutOpen called without WAVE_ALLOWSYNC flag";	break;
	}
	DarwiniaReleaseAssert(result == MMSYSERR_NOERROR, "Failed to open audio output device: \"%s\"", errString);
	*/

	//
	// Setup buffer 1

	/*
	memset(&s_buffer1Header, 0, sizeof(WAVEHDR));
	s_buffer1Header.lpData = (char*)m_buffer1;
	s_buffer1Header.dwBufferLength = m_samplesPerBuffer * format.nBlockAlign;
	s_buffer1Header.dwFlags = WHDR_DONE;
	result = waveOutPrepareHeader(s_device, &s_buffer1Header, sizeof(WAVEHDR));
	DarwiniaReleaseAssert(result == MMSYSERR_NOERROR, "Couldn't init buffer 1");
	*/

	//
	// Setup buffer 2

	/*
	memset(&s_buffer2Header, 0, sizeof(WAVEHDR));
	s_buffer2Header.lpData = (char*)m_buffer2;
	s_buffer2Header.dwBufferLength = m_samplesPerBuffer * format.nBlockAlign;
	s_buffer2Header.dwFlags = WHDR_DONE;
	result = waveOutPrepareHeader(s_device, &s_buffer2Header, sizeof(WAVEHDR));
	DarwiniaReleaseAssert(result == MMSYSERR_NOERROR, "Couldn't init buffer 2");
	*/

	//
	// Send both empty buffers to the soundcard to get things started

	/*
	result = waveOutWrite(s_device, &s_buffer1Header, sizeof(WAVEHDR));
	DarwiniaReleaseAssert(result == MMSYSERR_NOERROR, "Couldn't send sound data");
	result = waveOutWrite(s_device, &s_buffer2Header, sizeof(WAVEHDR));
	DarwiniaReleaseAssert(result == MMSYSERR_NOERROR, "Couldn't send sound data");
	*/
	
	s_nextBuffer = 1;
}


SoundLibrary2d::~SoundLibrary2d()
{
	/* waveOutClose(s_device);		s_device = NULL; */
	delete m_buffer1;			m_buffer1 = NULL;
	delete m_buffer2;			m_buffer2 = NULL;
	g_soundLibrary2d = NULL;
}


void SoundLibrary2d::SetCallback(void (*_callback)(StereoSample *, unsigned int))
{
	m_callback = _callback;
}
