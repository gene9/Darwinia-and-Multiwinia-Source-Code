#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/preferences.h"

#include "lib/sound/sound_library_2d_sdl.h"
#include "lib/sound/sound_library_2d_mode.h"

#include <SDL/SDL.h>

static SDL_AudioSpec s_audioSpec;
static int s_audioStarted = 0;

#include "app.h"
#include "lib/sound/soundsystem.h"
#include "lib/hi_res_time.h"

#define G_SL2D static_cast<SoundLibrary2dSDL *>(g_soundLibrary2d)

static void sdlAudioCallback(void *userdata, Uint8 *stream, int len)
{
	if (!s_audioStarted || !g_soundLibrary2d) 
		return;
	
	
	G_SL2D->AudioCallback( (StereoSample *) stream, len / sizeof(StereoSample) );
}

void SoundLibrary2dSDL::AudioCallback(StereoSample *stream, unsigned numSamples)
{
	m_callbackLock.Lock();
	if (!m_callback)
	{
		m_callbackLock.Unlock();
		return;
	}
			
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	m_callback(stream, numSamples);
#else
	m_buffer[1] = m_buffer[0];
	m_buffer[0].stream = stream;
	m_buffer[0].len = numSamples;
	m_bufferIsThirsty++;
	
	if (m_bufferIsThirsty > 2)
		m_bufferIsThirsty = 2;
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
			int samplesPerSecond = g_preferences->GetInt("SoundMixFreq", 44100);
			int samplesPerUpdate = (int)((double)samplesPerSecond / 20.0);
			G_SL2D->m_callback(buf, samplesPerUpdate);
			fwrite(buf, samplesPerUpdate, sizeof(StereoSample), m_wavOutput);
			nextOutputTime += 1.0 / 20.0;
		}
	}
	else {
#ifndef INVOKE_CALLBACK_FROM_SOUND_THREAD
		SDL_LockAudio();
		for (int i = 0; i < m_bufferIsThirsty; i++) {
			m_callback( m_buffer[i].stream, m_buffer[i].len );
		}
		m_bufferIsThirsty = 0;
		SDL_UnlockAudio();
#endif
	}
}

SoundLibrary2dSDL::SoundLibrary2dSDL()
:	m_bufferIsThirsty(0), 
	m_callback(NULL),
	m_wavOutput(NULL)
{
	AppReleaseAssert(!g_soundLibrary2d, "SoundLibrary2dSDL already exists");

	m_freq = g_preferences->GetInt("SoundMixFreq", 44100);
	m_samplesPerBuffer = g_preferences->GetInt("SoundBufferSize", 2000);

	// Initialise the output device

	SDL_AudioSpec desired;
	
	desired.freq = m_freq;
	desired.format = AUDIO_S16SYS;
	desired.samples = m_samplesPerBuffer;
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
	
	m_samplesPerBuffer = s_audioSpec.samples;
	
	// Start the callback function
	if (g_preferences->GetInt("Sound", 1)) 
		SDL_PauseAudio(0);
}

unsigned SoundLibrary2dSDL::GetSamplesPerBuffer()
{
	return m_samplesPerBuffer;
}

unsigned SoundLibrary2dSDL::GetFreq()
{
	return m_freq;
}

SoundLibrary2dSDL::~SoundLibrary2dSDL()
{
	AppDebugOut ( "Destructing SoundLibrary2dSDL class... " );
	Stop();
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
