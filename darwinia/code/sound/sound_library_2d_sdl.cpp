#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "lib/system_info.h"

#include "sound/sound_library_2d.h"
#include "sound/sound_library_2d_mode.h"

#include <SDL.h>


static SDL_AudioSpec s_audioSpec;
static int s_audioStarted = 0;

SoundLibrary2d *g_soundLibrary2d = NULL;

#include "app.h"
#include "sound/soundsystem.h"
#include "lib/hi_res_time.h"

static void sdlAudioCallback(void *userdata, Uint8 *stream, int len)
{
	if (!s_audioStarted || !g_soundLibrary2d || !g_app || !g_app->m_soundSystem) 
		return;
		
	g_soundLibrary2d->AudioCallback( (StereoSample *) stream, len / sizeof(StereoSample) );
}

void SoundLibrary2d::AudioCallback(StereoSample *stream, unsigned numSamples)
{
	if (!m_callback)
		return;
			
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
}

void SoundLibrary2d::TopupBuffer()
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

SoundLibrary2d::SoundLibrary2d()
:	m_bufferIsThirsty(0), 
	m_callback(NULL),
	m_wavOutput(NULL)
{
	DarwiniaReleaseAssert(!g_soundLibrary2d, "SoundLibrary2d already exists");

	m_freq = g_prefsManager->GetInt("SoundMixFreq", 44100);
	m_samplesPerBuffer = g_prefsManager->GetInt("SoundBufferSize", 2000);

	// Initialise the output device

	SDL_AudioSpec desired;
	
	desired.freq = m_freq;
	desired.format = AUDIO_S16SYS;
	desired.samples = m_samplesPerBuffer;
	desired.channels = 2;
	desired.userdata = 0;
	desired.callback = sdlAudioCallback;
	
	printf("Initialising SDL Audio\n");
	if (SDL_OpenAudio(&desired, &s_audioSpec) < 0) {
		const char *errString = SDL_GetError();
		DarwiniaReleaseAssert(false, "Failed to open audio output device: \"%s\"", errString);
	}
	else {
		printf("Frequency: %d\nFormat: %d\nChannels: %d\nSamples: %d\n", s_audioSpec.freq, s_audioSpec.format, s_audioSpec.channels, s_audioSpec.samples);
		printf("Size of Stereo Sample: %u\n", sizeof(StereoSample));
	}
	
	s_audioStarted = 1;
	
	m_samplesPerBuffer = s_audioSpec.samples;
	
	// Start the callback function
	if (g_prefsManager->GetInt("Sound", 1)) 
		SDL_PauseAudio(0);
}


SoundLibrary2d::~SoundLibrary2d()
{
	Stop();
	g_soundLibrary2d = NULL;
}


void SoundLibrary2d::Stop()
{
	if (s_audioStarted) {
		SDL_CloseAudio();
		s_audioStarted = 0;
	}
}

void SoundLibrary2d::SetCallback(void (*_callback)(StereoSample *, unsigned int))
{
	m_callback = _callback;
}

void SoundLibrary2d::StartRecordToFile(char const *_filename)
{
	m_wavOutput = fopen(_filename, "wb");
	DarwiniaReleaseAssert(m_wavOutput != NULL, "Couldn't create wave outout file %s", _filename);
}


void SoundLibrary2d::EndRecordToFile()
{
	fclose(m_wavOutput);
	m_wavOutput = NULL;
}
