#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/preferences.h"

#include "sound/sound_library_2d_sdl.h"

#include <SDL/SDL.h>

static SDL_AudioSpec s_audioSpec;
static int s_audioStarted = 0;

#include "app.h"
#include "lib/hi_res_time.h"

#include "soundsystem.h"

#define G_SL2D static_cast<SoundLibrary2dSDL *>(g_soundLibrary2d)

static void sdlAudioCallback(void *userdata, Uint8 *stream, int len)
{
	if (!s_audioStarted || !g_soundLibrary2d) 
		return;
	
	
	G_SL2D->AudioCallback( (StereoSample *) stream, len );
}

void SoundLibrary2dSDL::AudioCallback(StereoSample *stream, unsigned int numBytes)
{
	m_callbackLock.Lock();
	if (!m_callback)
	{
		m_callbackLock.Unlock();
		return;
	}
			
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( g_app->m_soundSystem->m_mutex );
	m_callback(stream, numBytes / sizeof(StereoSample));
#else
	m_ringBuffer->FetchAudio((short *)stream, numBytes);
#endif
	m_callbackLock.Unlock();
}

void SoundLibrary2dSDL::TopupBuffer()
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
			m_callback(buf, samplesPerUpdate);
			fwrite(buf, samplesPerUpdate, sizeof(StereoSample), m_wavOutput);
			nextOutputTime += 1.0 / 20.0;
		}
	}
	else
	{
#ifndef INVOKE_CALLBACK_FROM_SOUND_THREAD
		double now = GetHighResTime();
		if (!isnan(m_lastSampleTime))
		{
			unsigned int totalSamplesRequested = (now - m_lastSampleTime) * m_freq;
			unsigned int samplesStored = 0;
			while (samplesStored < totalSamplesRequested)
			{
				unsigned int samplesRequested = min(m_freq, totalSamplesRequested - samplesStored);
				
				m_callback(m_callbackBuffer, samplesRequested);
				m_ringBuffer->StoreAudio((short *)(m_callbackBuffer+samplesStored),
										 samplesRequested*sizeof(StereoSample));			
				samplesStored += samplesRequested;
			}
		}
		m_lastSampleTime = now;
#endif
	}
}

SoundLibrary2dSDL::SoundLibrary2dSDL()
:	m_callback(NULL),
	m_wavOutput(NULL)
{
	AppReleaseAssert(!g_soundLibrary2d, "SoundLibrary2dSDL already exists");

	m_freq = min(g_prefsManager->GetInt("SoundMixFreq", 44100), 44100);
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	m_samplesPerBuffer = g_prefsManager->GetInt("SoundBufferSize", 2000);
#else
	m_ringBuffer = new ASWSDLRingBuffer();
	m_ringBuffer->Allocate(2, 4, m_freq / 15);	// we only allow up to 1/15 second of lag time
	m_samplesPerBuffer = m_freq;
	m_lastSampleTime = NAN;
	
	// add some lag to protect ourselves from skipping
	int bufferAhead = m_freq / 10;
	short *silence = (short *)calloc(bufferAhead, sizeof(short)*2);
	m_ringBuffer->StoreAudio(silence, bufferAhead * sizeof(short)*2);
	free(silence);
#endif

	// Initialise the output device

	SDL_AudioSpec desired;
	
	desired.freq = m_freq;
	desired.format = AUDIO_S16SYS;
	desired.samples = 512;
	desired.channels = 2;
	desired.userdata = 0;
	desired.callback = sdlAudioCallback;
	
	AppDebugOut("Initialising SDL Audio\n");
	if (SDL_OpenAudio(&desired, &s_audioSpec) < 0) {
		const char *errString = SDL_GetError();
		AppReleaseAssert(false, "Failed to open audio output device: \"%s\"", errString);
	}
	else {
		AppDebugOut("Frequency: %d\nFormat: %d\nChannels: %d\nSamples: %d\n", s_audioSpec.freq, s_audioSpec.format, s_audioSpec.channels, s_audioSpec.samples);
		AppDebugOut("Size of Stereo Sample: %u\n", sizeof(StereoSample));
	}
	
	s_audioStarted = 1;
		
	// Start the callback function
	if (g_prefsManager->GetInt("Sound", 1)) 
		SDL_PauseAudio(0);
}

SoundLibrary2dSDL::~SoundLibrary2dSDL()
{
	AppDebugOut ( "Destructing SoundLibrary2dSDL class... " );
	Stop();
#ifndef INVOKE_CALLBACK_FROM_SOUND_THREAD
	delete m_ringBuffer;
#endif
	AppDebugOut ( "done.\n" );
}


void SoundLibrary2dSDL::Stop()
{
	SDL_CloseAudio();
	s_audioStarted = 0;
}

// Lock ensures that old callback won't still be running once this method exits
void SoundLibrary2dSDL::SetCallback(void (*_callback)(StereoSample *, unsigned int))
{
	m_callbackLock.Lock();
	m_callback = _callback;
	m_callbackLock.Unlock();
}

void SoundLibrary2dSDL::StartRecordToFile(char const *_filename)
{
	m_wavOutput = fopen(_filename, "wb");
	AppReleaseAssert(m_wavOutput != NULL, "Couldn't create wave outout file %s", _filename);
}


void SoundLibrary2dSDL::EndRecordToFile()
{
	fclose(m_wavOutput);
	m_wavOutput = NULL;
}
