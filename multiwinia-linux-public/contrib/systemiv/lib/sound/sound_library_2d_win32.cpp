#include "lib/universal_include.h"

#include <MMSYSTEM.H>

#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "lib/system_info.h"

#include "lib/sound/sound_library_2d_win32.h"

#define g_soundLibrary2d static_cast<SoundLibrary2dWin32 *>(g_soundLibrary2d)

static HWAVEOUT	s_device;


#include "app.h"
#include "lib/hi_res_time.h"
#include "sound/soundsystem.h"


//*****************************************************************************
// Class SoundLib2dBuf
//*****************************************************************************

SoundLib2dBuf::SoundLib2dBuf()
{
	// Allocate the buffer
	unsigned int numSamples = g_prefsManager->GetInt("SoundBufferSize", 2000);
	m_buffer = new StereoSample[numSamples];

	// Clear the buffer
	memset(m_buffer, 0, numSamples * sizeof(StereoSample));

	// Register the buffer with Windows
	memset(&m_header, 0, sizeof(WAVEHDR));
	m_header.lpData = (char*)m_buffer;
	int blockAlign = 4;		// 2 channels * 2 bytes per sample
	m_header.dwBufferLength = numSamples * blockAlign;
	m_header.dwFlags = WHDR_DONE;
	int result = waveOutPrepareHeader(s_device, &m_header, sizeof(WAVEHDR));
	DarwiniaReleaseAssert(result == MMSYSERR_NOERROR, "Couldn't init buffer");

	// Play the buffer
	static int count = 0;
	if (count < 4)
	{
		count++;
		result = waveOutWrite(s_device, &m_header, sizeof(WAVEHDR));
		DarwiniaReleaseAssert(result == MMSYSERR_NOERROR, "Couldn't send sound data");
	}
}


SoundLib2dBuf::~SoundLib2dBuf()
{
	waveOutUnprepareHeader(s_device, &m_header, sizeof(WAVEHDR));
	delete [] m_buffer;			m_buffer = NULL;
}



//*****************************************************************************
// Class SoundLibrary2dWin32
//*****************************************************************************

void CALLBACK WaveOutProc(HWAVEOUT _dev, UINT _msg, DWORD _userData, DWORD _param1, DWORD _param2)
{
	if (_msg != WOM_DONE) return;
	if (!s_device)	return;
	if (!g_soundLibrary2d || !g_soundLibrary2d->m_callback) return;

	g_soundLibrary2d->m_fillsRequested++;
}


SoundLibrary2dWin32::SoundLibrary2dWin32()
:	m_callback(NULL),
	m_wavOutput(NULL),
	m_numBuffers(10),
	m_nextBuffer(0),
	m_fillsRequested(0)
{
	DarwiniaReleaseAssert(!g_soundLibrary2d, "SoundLibrary2dWin32 already exists");

	m_freq = g_prefsManager->GetInt("SoundMixFreq", 44100);
	m_samplesPerBuffer = g_prefsManager->GetInt("SoundBufferSize", 2000);


	//
	// Initialise the output device

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


	// 
	// Create the sound buffers

	m_buffers = new SoundLib2dBuf[m_numBuffers];
}


SoundLibrary2dWin32::~SoundLibrary2dWin32()
{
	waveOutReset(s_device);
	delete [] m_buffers;		m_buffers = NULL;
	waveOutClose(s_device);		s_device = NULL;
}


void SoundLibrary2dWin32::SetCallback(void (*_callback)(StereoSample *, unsigned int))
{
	m_callback = _callback;
}


void SoundLibrary2dWin32::TopupBuffer()
{
	if (m_wavOutput)
	{
		static double nextOutputTime = -1.0;
		if (nextOutputTime < 0.0) nextOutputTime = GetHighResTime();
		
		if (GetHighResTime() > nextOutputTime)
		{
			StereoSample buf[5000];
			int samplesPerSecond = g_prefsManager->GetInt("SoundMixFreq", 44100);
			int samplesPerUpdate = (int)((double)samplesPerSecond / 20.0);
			g_soundLibrary2d->m_callback(buf, samplesPerUpdate);
			fwrite(buf, samplesPerUpdate, sizeof(StereoSample), m_wavOutput);
			nextOutputTime += 1.0 / 20.0;
		}
	}
	else
	{
		while (m_fillsRequested)
		{
			SoundLib2dBuf *buf = &m_buffers[m_nextBuffer];

			g_soundLibrary2d->m_callback(buf->m_buffer, g_soundLibrary2d->m_samplesPerBuffer);

			int result = waveOutWrite(s_device, &buf->m_header, sizeof(WAVEHDR));
			if (result != MMSYSERR_NOERROR)
			{
				break;
			}
			else
			{
				m_nextBuffer++;
				m_nextBuffer %= m_numBuffers;
				m_fillsRequested--;
			}
		}
	}
}


void SoundLibrary2dWin32::Stop()
{
//	#warning "WARNING: Write me, called in Finalise()"
	// this is suppose to stop the callback from being invoked.
	// (important on mac and linux where the sound is in a separate thread).
}


void SoundLibrary2dWin32::StartRecordToFile(char const *_filename)
{
	m_wavOutput = fopen(_filename, "wb");
	DarwiniaReleaseAssert(m_wavOutput != NULL, "Couldn't create wave outout file %s", _filename);
}


void SoundLibrary2dWin32::EndRecordToFile()
{
	fclose(m_wavOutput);
	m_wavOutput = NULL;
}
