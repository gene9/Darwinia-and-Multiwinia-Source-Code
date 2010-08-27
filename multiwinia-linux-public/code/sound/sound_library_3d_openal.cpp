#include "lib/universal_include.h"

#ifdef HAVE_OPENAL
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"

#include "sound/sound_filter.h"
#include "sound/sound_library_3d_openal.h"

#include "app.h"

#ifdef TARGET_OS_LINUX
#include <sched.h>
#endif

#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
#include "soundsystem.h"		// For the damn mutex
#endif

//*****************************************************************************
// Class Misc Static Data
//*****************************************************************************


//We keep a ring of buffers, updating them as their playback completes.
static const int s_numChannelBuffers = 2;
static bool s_listenerDirty = false;
static volatile bool s_shouldExit = false;

static char s_dxErrorMsg[512];

#ifdef _DEBUG
#define SOUNDASSERT(x, y, ...)  { if(x != DS_OK && x != S_FALSE) {  \
                                sprintf( s_dxErrorMsg, "%s %s", \
                                DXGetErrorString9(x),   \
                                DXGetErrorDescription9(x) );  \
                                char msg[512];             \
                                sprintf( msg, "OpenAL ERROR\n%s line %d\n\n%s\nError Code(%d) : %s\n%s",  \
                                __FILE__, __LINE__,   \
                                y,                    \
                                x,                    \
                                DXGetErrorString9(x),   \
                                DXGetErrorDescription9(x) );  \
                                AppReleaseAssert( false, msg, __VA_ARGS__ );  } }
#else
#define SOUNDASSERT(x, y, ...)  SOUNDDEBUG(x, y, __VA_ARGS__)
#endif // _DEBUG

#define SOUNDDEBUG(x, y, ...)   { if(x != DS_OK && x != S_FALSE) {  \
                                sprintf( s_dxErrorMsg, "%s %s", \
                                DXGetErrorString9(x),   \
                                DXGetErrorDescription9(x) );  \
                                char msg[512];             \
                                sprintf( msg, "OpenAL ERROR, %s line %d, %s, Error Code(%d) : %s, %s\n",  \
                                __FILE__, __LINE__,   \
                                y,                    \
                                x,                    \
                                DXGetErrorString9(x),   \
                                DXGetErrorDescription9(x) );  \
                                AppDebugOut( msg, __VA_ARGS__ );  } }



//*****************************************************************************
// Class SoundLibFilter
//*****************************************************************************

class SoundLibFilter
{
public:
	DspEffect			*m_userFilter;
	bool				m_openalFilter;			// True if an openal filter has been created

	SoundLibFilter(): m_userFilter(NULL), m_openalFilter(false) {}
};


//*****************************************************************************
// Class OpenALChannel
//*****************************************************************************

class OpenALChannel
{
public:
	SoundLibFilter		m_dspFX[SoundLibrary3d::NUM_FILTERS];

    ALuint			  	m_buffer[s_numChannelBuffers];
    ALuint				m_source;
	unsigned int		m_numBufferSamples;
    int                 m_lastSampleWritten;
    float               m_channelHealth;
    int                 m_silenceRemaining;     // Stream has finished, must write m_numBufferSamples worth of silence    
	long				m_simulatedPlayCursor;
	double				m_lastUpdate;
	int					m_buffersProcessed;

	unsigned int		m_freq;					// Value recorded on previous call of SetChannelFrequency
	unsigned int		m_prevFreq;				// Value used on previous call to alSourcef()
	float				m_volume;				// Value recorded on previous call of SetChannelVolume
	float				m_prevVolume;			// Value used on previous call to alSourcef()
	float				m_minDist;				// Value recorded on previous call of SetChannelMinDist
	float				m_prevMinDist;			// Value used on previous call to alSourcef()
	Vector3				m_pos;					// Value recorded on previous call of SetChannelPosition
	Vector3				m_prevPos;				// Value used on previous call to alSource3f()
	int					m_3DMode;				// Value recorded on previous call of SetChannel3DMode
	//NOTE: We won't cache 3DMode, as it changes very infrequently.
    
    
    bool				m_resetChannel;
public:
    OpenALChannel();

	void UpdateSimulatedPlayCursor(long _realPlayCursor);
};


OpenALChannel::OpenALChannel()
:	m_numBufferSamples(0),
    m_lastSampleWritten(0),
    m_channelHealth(0.0f),
    m_silenceRemaining(0),
    m_buffersProcessed(0),
	m_freq(-1),
	m_prevFreq(-1),
	m_volume(-1.0f),
	m_prevVolume(-1.0f),
	m_minDist(-1.0f),
	m_prevMinDist(-1.0f),
	m_pos(0,0,0),
	m_prevPos(0,0,0),
	m_3DMode(-1),
	m_resetChannel(false)
{
	for (int i = 0; i < s_numChannelBuffers; ++i)
	{
		m_buffer[i] = 0;
	}
}


void OpenALChannel::UpdateSimulatedPlayCursor(long _playCursor)
{
	double timeNow = GetHighResTime();

	if (_playCursor < 0)
	{
		double timeSinceLastUpdate = timeNow - m_lastUpdate;
		m_simulatedPlayCursor += timeSinceLastUpdate * (double)m_freq * 2.0;
		m_simulatedPlayCursor %= m_numBufferSamples * 2;
		m_simulatedPlayCursor &= 0xfffffffe;
	}
	else
	{
		m_simulatedPlayCursor = _playCursor;
	}

	m_lastUpdate = timeNow;
}


#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
static NetCallBackRetType SL3D_OpenALCallThread(void *ignored)
{
	while (g_soundLibrary3d && !s_shouldExit)
	{
		((SoundLibrary3dOpenAL*)g_soundLibrary3d)->ActuallyAdvance();
	}
	s_shouldExit = false;
}
#endif


//*****************************************************************************
// Class SoundLibrary3dOpenAL
//*****************************************************************************

SoundLibrary3dOpenAL::SoundLibrary3dOpenAL()
:   SoundLibrary3d(),
	m_musicChannel(NULL),
	m_channels(NULL)
{
}


SoundLibrary3dOpenAL::~SoundLibrary3dOpenAL()
{
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	//
	// Stop the thread
	g_app->m_soundSystem->m_mutex.Unlock();
	s_shouldExit = true;
	while (s_shouldExit);
	g_app->m_soundSystem->m_mutex.Lock();
#endif
	//
	// Release secondary buffers

	if( m_channels )
	{
		for( int i = 0; i < m_numChannels; ++i )
		{
			alDeleteSources(1, &m_channels[i].m_source);
			alDeleteBuffers(s_numChannelBuffers, m_channels[i].m_buffer);
		}
		delete[] m_channels;
	}


	//
	// Release music channel 

	if( m_musicChannel )
	{
		alDeleteSources(1, &m_musicChannel->m_source);
		alDeleteBuffers(s_numChannelBuffers, m_musicChannel->m_buffer);
	}

	//
	// Destroy the OpenAL device and context.
	alcMakeContextCurrent(0);
	alcDestroyContext(m_context);
	alcCloseDevice(m_device);

	//
	// Delete temp buffer.

	free(m_tempBuffer);
}

bool SoundLibrary3dOpenAL::Initialise(int _mixFreq, int _numChannels, bool _hw3d, 
										   int _mainBufNumSamples, int _musicBufNumSamples )
{    
    int errCode;
	AppReleaseAssert( _numChannels > 0, "SoundLibrary3d asked to create too few channels" );

	m_numChannels = _numChannels;
	m_sampleRate = _mixFreq;

	m_prevMasterVolume = m_masterVolume;
	alListenerf(AL_GAIN,log2f(m_masterVolume/255.f));	//TODO: Replace with log2f(m_masterVolume) - log2f(255.f)

	// Just use the default device for now.
	m_device = alcOpenDevice(0);
    m_context = alcCreateContext(m_device,0);
    
    alcMakeContextCurrent(m_context);
    
    AppDebugOut( "Initialising OpenAL Sound Library:\n"
				"Mix Frequency: %d\n"
				"Num Channels: %d\n"
				"Buffer size: %d (music: %d)\n"
				"AL Vendor: \"%s\"\n"
				"AL Version: \"%s\"\n"
				"AL Renderer: \"%s\"\n"
				"AL Extensions: \"%s\"\n",
				_mixFreq, _numChannels, _mainBufNumSamples, _musicBufNumSamples,
				alcGetString(m_device, AL_VENDOR),
				alcGetString(m_device, AL_VERSION),
				alcGetString(m_device, AL_RENDERER),
				alcGetString(m_device, AL_EXTENSIONS)
				);
    
    
    // We need to reserve some memory for the callbacks to write to.
    m_tempBuffer = (char*)malloc(max(_mainBufNumSamples,_musicBufNumSamples)*2);
    
	//
	// Create a streaming secondary buffer for each channel

    m_channels = new OpenALChannel[ m_numChannels ];

    for( int i = 0; i < m_numChannels; ++i )
    {
		
		alGenBuffers(s_numChannelBuffers, m_channels[i].m_buffer);
		
		alGenSources(1, &(m_channels[i].m_source));

		m_channels[i].m_numBufferSamples = _mainBufNumSamples;
		m_channels[i].m_lastSampleWritten = 0;
		m_channels[i].m_simulatedPlayCursor = -1;
		m_channels[i].m_freq = _mixFreq;
		m_channels[i].m_prevFreq = _mixFreq;

	}    


	// 
	// Create a streaming secondary buffer for music

	m_musicChannel = new OpenALChannel;

	alGenBuffers(s_numChannelBuffers, m_musicChannel->m_buffer);
	alGenSources(1, &(m_musicChannel->m_source));

	m_musicChannel->m_numBufferSamples = _musicBufNumSamples;
	m_musicChannel->m_lastSampleWritten = m_musicChannel->m_numBufferSamples - 1;
	m_musicChannel->m_simulatedPlayCursor = -1;
	m_musicChannel->m_freq = _mixFreq;

	SetChannel3DMode(m_musicChannelId, 2);
	SetChannelVolume(m_musicChannelId, 10.0f);
	SetChannelFrequency(m_musicChannelId, 44100);


    //
    // Set our listener properties

	alDopplerFactor(0.f);

#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetStartThread(&SL3D_OpenALCallThread,0);
#endif
	return true;
}

bool SoundLibrary3dOpenAL::Hardware3DSupport()
{
	// STUB
    return( false );
}


int SoundLibrary3dOpenAL::GetMaxChannels()
{           
    if( Hardware3DSupport() && m_hw3dDesired )
    {
        return 64;		//STUB
    }
    else
    {
        return 64;
    }
}


int SoundLibrary3dOpenAL::GetCPUOverhead()
{
    // STUB: We don't do this with OpenAL
    return 0;
}


float SoundLibrary3dOpenAL::GetChannelHealth(int _channel)
{
	OpenALChannel *channel;
	if (_channel == m_musicChannelId) channel = m_musicChannel;
	else				channel = &m_channels[_channel];

    if( channel->m_channelHealth >= 0.5f )
    {
        return 1.0f;
    }
    else
    {
        return channel->m_channelHealth * 2.0f;
    }
}


int SoundLibrary3dOpenAL::GetChannelBufSize(int _channel) const
{
	if (_channel == m_musicChannelId)
	{
		return m_musicChannel->m_numBufferSamples;
	}
	else
	{
		return m_channels[_channel].m_numBufferSamples;
	}
}


void SoundLibrary3dOpenAL::SetChannel3DMode( int _channel, int _mode )
{
	OpenALChannel *channel;
	if (_channel == m_musicChannelId) channel = m_musicChannel;
	else				channel = &m_channels[_channel];

	if (_mode != channel->m_3DMode)
	{
		channel->m_3DMode = _mode;

		switch( _mode )
		{
			case 0 :	alSourcei( channel->m_source, AL_SOURCE_RELATIVE, AL_FALSE ); break;
			case 1 :
			case 2 :	alSourcei( channel->m_source, AL_SOURCE_RELATIVE, AL_TRUE ); break;
		};

	}
}


void SoundLibrary3dOpenAL::SetChannelPosition( int _channel, Vector3 const &_pos, Vector3 const &_vel )
{
	OpenALChannel *channel;
	if (_channel == m_musicChannelId) channel = m_musicChannel;
	else				channel = &m_channels[_channel];

	channel->m_pos = _pos;
}


void SoundLibrary3dOpenAL::SetChannelFrequency( int _channel, int _frequency )
{
	OpenALChannel *channel;
	if (_channel == m_musicChannelId) channel = m_musicChannel;
	else				channel = &m_channels[_channel];

	channel->m_freq = _frequency;
}


void SoundLibrary3dOpenAL::SetChannelMinDistance( int _channel, float _minDistance )
{
	OpenALChannel *channel;
	if (_channel == m_musicChannelId) channel = m_musicChannel;
	else				channel = &m_channels[_channel];

	channel->m_minDist = _minDistance;
}


void SoundLibrary3dOpenAL::SetChannelVolume( int _channel, float _volume )
{
	OpenALChannel *channel;
	if (_channel == m_musicChannelId) 
		channel = m_musicChannel;
	else				channel = &m_channels[_channel];

	AppDebugAssert(_volume >= 0.0f && _volume <= 10.0f);

	channel->m_volume = _volume;

}


void SoundLibrary3dOpenAL::SetListenerPosition( Vector3 const &_pos, 
                                        Vector3 const &_front,
                                        Vector3 const &_up,
                                        Vector3 const &_vel )
{   
	m_listenerPos = _pos;

	//alListener3f(AL_POSITION, _pos.x, _pos.y, _pos.z);
	
	m_listenerFront = _front;
	
	m_listenerUp = _up;

	s_listenerDirty = true;


	//alListener3f(AL_VELOCITY, _vel.x, _vel.y, _vel.z);
}

void SoundLibrary3dOpenAL::CommitChangesChannel(OpenALChannel *channel)
{
	// al calls are _really_ slow, so we should avoid wherever possible.
	if (channel->m_silenceRemaining)
		return;
	
	// Don't bother updating when it's likely not needed.
	if (channel->m_lastUpdate + (channel->m_numBufferSamples/s_numChannelBuffers)/channel->m_freq < GetHighResTime())	
		alGetSourcei(channel->m_source, AL_BUFFERS_PROCESSED, &(channel->m_buffersProcessed));

	
	// Commit Volume changes
	if (!NearlyEquals(channel->m_prevVolume, channel->m_volume) || m_forceVolumeUpdate )
	{
		float calculatedVolume = -(5.f-channel->m_volume*0.5f);//log2f((channel->m_volume)*0.1f+1.f);

		calculatedVolume = expf(calculatedVolume);
        //float calculatedVolume = (_volume * 0.1f);
        //calculatedVolume += m_masterVolume;

		if( calculatedVolume < 0.0f ) calculatedVolume = 0.0f;
		if( calculatedVolume > 1.0f ) calculatedVolume = 1.0f;

		alSourcef(channel->m_source, AL_GAIN, calculatedVolume);
		channel->m_prevVolume = channel->m_volume;
	}
	
	// Commit channel frequency
	if (channel->m_freq != channel->m_prevFreq)
	{
		channel->m_prevFreq = channel->m_freq;
		alSourcef(channel->m_source, AL_PITCH, channel->m_freq/((float)m_sampleRate));
	}
	
	// Commit channel Min. Distance
	if (!NearlyEquals(channel->m_minDist, channel->m_prevMinDist))
	{
		channel->m_prevMinDist = channel->m_minDist;
		alSourcei(channel->m_source, AL_REFERENCE_DISTANCE, channel->m_minDist);	
	}
	
	// Commit channel position.
	Vector3 listenerToOldPos(channel->m_pos - m_listenerPos);
	Vector3 oldPosToNewPos(channel->m_pos - channel->m_prevPos);
	if (oldPosToNewPos.MagSquared() > listenerToOldPos.MagSquared() * 0.0025f)
	{
		channel->m_prevPos = channel->m_pos;
		
		alSource3f(channel->m_source, AL_POSITION, channel->m_pos.x, channel->m_pos.y, channel->m_pos.z);
		//Don't bother, we don't use it.
		//alSource3f(channel->m_source, AL_VELOCITY, channel->m_pos.x, channel->m_pos.y, channel->m_pos.z);
	}
	
	
}

void SoundLibrary3dOpenAL::CommitChangesGlobal()
{
	
	if (s_listenerDirty)
	{
		// TO DO: Find the reason why sometime _up equals (0,0,0)
		alListener3f(AL_POSITION, m_listenerPos.x, m_listenerPos.y, m_listenerPos.z);
		if( m_listenerUp != g_zeroVector && m_listenerFront != g_zeroVector )
		{
			ALfloat orientation[] = { m_listenerFront.x, m_listenerFront.y, m_listenerFront.z,
									m_listenerUp.x, m_listenerUp.y, m_listenerUp.z };
			alListenerfv(AL_ORIENTATION, orientation);
		}
		s_listenerDirty = false;
}
	
	//Update master volume
	if (m_masterVolume != m_prevMasterVolume)
	{
		float calculatedVolume = expf(m_masterVolume*0.001);
		alListenerf(AL_GAIN,calculatedVolume);
	}
}

void SoundLibrary3dOpenAL::PopulateBuffer(int _channel, int _fromSample, int _numSamples, bool _isMusic, char *buffer)
{
    int errCode;
    OpenALChannel *channel;
	if (_channel == m_musicChannelId) channel = m_musicChannel;
	else channel = &m_channels[_channel];
    int byteOffset = _fromSample * 2;
    int byteSize = _numSamples * 2;

    //
    // Fill some of the buffer 

	START_PROFILE( "FillBuf");
    if (_isMusic && m_musicCallback)
	{
		m_musicCallback( (signed short *)buffer, _numSamples, _numSamples, &channel->m_silenceRemaining );
	}
	else if(!_isMusic && m_mainCallback)
    {
		m_mainCallback(_channel, (signed short *)buffer, _numSamples, _numSamples, &channel->m_silenceRemaining);
    }
	END_PROFILE( "FillBuf");


    //
    // Apply any non OpenAL filters

    for( int i = 0; i < NUM_FILTERS; ++i )
    {
		if (channel->m_dspFX[i].m_userFilter == NULL) continue;
		channel->m_dspFX[i].m_userFilter->Process( (signed short *)buffer, _numSamples );
    }
    channel->m_channelHealth = 1.0f - ( (float) _numSamples / (float) channel->m_numBufferSamples );
}


void SoundLibrary3dOpenAL::AdvanceChannel(int _channel, int _frameNum)
{
    int errCode;
	OpenALChannel *channel;
	
	if ( (_channel & 3) != (_frameNum & 3) )
		return;
	
	bool isMusicChannel = false;
	if (_channel == m_musicChannelId) 
	{
		isMusicChannel = true;
		channel = m_musicChannel;
	}
	else 
	{
		channel = &m_channels[_channel];
	}



	if (!channel->m_buffersProcessed)
		return;
	
	char *data = m_tempBuffer;
	
	if (channel->m_buffersProcessed > 1)
		AppDebugOut ("OAL BuffersProcessed (Channel %d) = %d\n", _channel, channel->m_buffersProcessed);
		
	while (channel->m_buffersProcessed--)
	{	
		ALuint currentBuffer = 0;
		alSourceUnqueueBuffers(channel->m_source, 1, &currentBuffer);
		
		PopulateBuffer( _channel, 0, channel->m_numBufferSamples/s_numChannelBuffers, isMusicChannel, data);
		
		alBufferData(currentBuffer, AL_FORMAT_MONO16, data, channel->m_numBufferSamples/s_numChannelBuffers * 2, m_sampleRate);
		
		alSourceQueueBuffers(channel->m_source, 1, &currentBuffer);
		
		if (channel->m_silenceRemaining)
		{
			channel->m_silenceRemaining = 0;
			break;
		}
		

		
	}
	
	int sourceState = 0;
	alGetSourcei( channel->m_source, AL_SOURCE_STATE, &sourceState );
	if (sourceState == AL_STOPPED)
	{
		AppDebugOut( "Channel underrun in OpenAL (Channel %d)\n", _channel);
		alSourcePlay( channel->m_source );
	}
	
	channel->m_lastUpdate = GetHighResTime();

}


int SoundLibrary3dOpenAL::GetNumFilters(int _channel)
{
	OpenALChannel *chan = &m_channels[_channel];
	
	int numFilters = 0;

	for (int j = 0; j < NUM_FILTERS; ++j)
	{
		if (chan->m_dspFX[j].m_userFilter || chan->m_dspFX[j].m_openalFilter)
		{
			numFilters++;
		}
	}

	return numFilters;
}


void SoundLibrary3dOpenAL::Verify()
{
	for (int i = 0; i < m_numChannels; ++i)
	{
		int numFilters = GetNumFilters(i);
		AppDebugAssert(numFilters <= 1);
	}
}


void SoundLibrary3dOpenAL::ActuallyAdvance()
{
//	Verify();

    START_PROFILE( "Commit Global");
    CommitChangesGlobal();
	END_PROFILE( "Commit Global");

	static int frameNum = 0;
	++frameNum;
	if (frameNum >= 4)
	{
		frameNum = 0;
	}

	// Commit channel-level changes _before_ locking the mutex.
	START_PROFILE("Commit Channel");
	for (int i = 0; i < m_numChannels; ++i)
	{
		if ( (i & 3) != (frameNum & 3) )
			continue;
		CommitChangesChannel(&m_channels[i]);
	}
	
	CommitChangesChannel(m_musicChannel);
	END_PROFILE("Commit Channel");

	#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( g_app->m_soundSystem->m_mutex );
	#endif
    for (int i = 0; i < m_numChannels; ++i)
    {
		AdvanceChannel(i, frameNum);
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
		ActuallyResetChannel( i, &(m_channels[i]) );
#endif
    }

#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	ActuallyResetChannel( m_musicChannelId, m_musicChannel );
#endif
	AdvanceChannel(m_musicChannelId, frameNum);
}

void SoundLibrary3dOpenAL::Advance()
{
	// When we're running our OpenAL calls in another thread, Advance()
	// becomes a no-op.
#ifndef INVOKE_CALLBACK_FROM_SOUND_THREAD
	ActuallyAdvance();
#endif
}

void SoundLibrary3dOpenAL::ResetChannel( int _channel )
{    
	OpenALChannel *channel;
	if (_channel == m_musicChannelId) 
	{
		channel = m_musicChannel;
	}
	else
	{
		channel = &m_channels[_channel];
	}

    AppDebugAssert(_channel >= -1);
	channel->m_resetChannel = true;

#ifndef INVOKE_CALLBACK_FROM_SOUND_THREAD
	ActuallyResetChannel( _channel, channel );
#endif
}

void SoundLibrary3dOpenAL::ActuallyResetChannel( int _channel, OpenALChannel *channel)
{
	int errCode;
	const bool isMusicChannel = (_channel == m_musicChannelId);


	if (!channel->m_resetChannel)
		return;

	//
	// Stop the channel
	alSourceStop(channel->m_source);

	//
	// Update the channel parameters
	START_PROFILE("Commit Channel");
	CommitChangesChannel(channel);
	END_PROFILE("Commit Channel");

	//
	// Unqueue all buffers
	int queuedBuffers = 0;
	alGetSourcei( channel->m_source, AL_BUFFERS_QUEUED, &queuedBuffers);
	while (queuedBuffers--)
	{
		ALuint currentBuffer = 0;
		alSourceUnqueueBuffers( channel->m_source, 1, &currentBuffer);
	}

	//
    // Populate our buffer
	int buffersProcessed = s_numChannelBuffers;
	
	char *data = m_tempBuffer;
	
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	// Lock the mutex (per buffer for greater scheduling granularity)
	NetLockMutex lock( g_app->m_soundSystem->m_mutex );
#endif
	
	while (buffersProcessed--)
	{
		
		ALuint currentBuffer = channel->m_buffer[buffersProcessed];
		PopulateBuffer( _channel, 0, channel->m_numBufferSamples/s_numChannelBuffers, isMusicChannel, data);
		
		alBufferData(currentBuffer, AL_FORMAT_MONO16, data, channel->m_numBufferSamples/s_numChannelBuffers * 2, m_sampleRate);
		
		alSourceQueueBuffers(channel->m_source, 1, &currentBuffer);
		
		if (channel->m_silenceRemaining)
		{
			channel->m_silenceRemaining = 0;
			break;
		}
	}
	
	//
	// Play the source
	alSourcePlay(channel->m_source);
	
	channel->m_resetChannel = false;
	channel->m_buffersProcessed = 0;
}


void SoundLibrary3dOpenAL::EnableDspFX(int _channel, int _numFilters, int const *_filterTypes)
{
    AppReleaseAssert( _numFilters > 0, "Bad argument passed to EnableFilters" );

	AppDebugAssert(_channel >= 0 && _channel < m_numChannels);
	OpenALChannel *channel;
	if (_channel == m_musicChannelId) channel = m_musicChannel;
	else				channel = &m_channels[_channel];

    for( int i = 0; i < _numFilters; ++i )
    {
		AppDebugAssert(_filterTypes[i] >= 0);

		switch (_filterTypes[i])
		{
			case DSP_RESONANTLOWPASS:
				channel->m_dspFX[_filterTypes[i]].m_userFilter = new DspResLowPass(m_sampleRate);
				break;
			case DSP_BITCRUSHER:
				channel->m_dspFX[_filterTypes[i]].m_userFilter = new DspBitCrusher(m_sampleRate);
				break;
			case DSP_GARGLE:
				channel->m_dspFX[_filterTypes[i]].m_userFilter = new DspGargle(m_sampleRate);
				break;
			case DSP_ECHO:
				channel->m_dspFX[_filterTypes[i]].m_userFilter = new DspEcho(m_sampleRate);
				break;
			case DSP_SIMPLE_REVERB:
				channel->m_dspFX[_filterTypes[i]].m_userFilter = new DspReverb(m_sampleRate);
				break;
		}
    }
}

void SoundLibrary3dOpenAL::UpdateDspFX( int _channel, int _filterType, int _numParams, float const *_params )
{
	OpenALChannel *channel;
	if (_channel == m_musicChannelId) channel = m_musicChannel;
	else				channel = &m_channels[_channel];
	
	if (!channel->m_dspFX[_filterType].m_userFilter) return;

    channel->m_dspFX[_filterType].m_userFilter->SetParameters( _params );
}

void SoundLibrary3dOpenAL::DisableDspFX( int _channel )
{
	AppDebugAssert(GetNumFilters(_channel) > 0);

   	OpenALChannel *channel;
	if (_channel == m_musicChannelId) channel = m_musicChannel;
	else				channel = &m_channels[_channel];
  
    //
    // Shut down the filters

	int numALFilters = 0;
	for (int i = 0; i < NUM_FILTERS; ++i)
	{
		SoundLibFilter *filter = &channel->m_dspFX[i];
		if (filter->m_userFilter != NULL)
		{
			delete filter->m_userFilter; 
            filter->m_userFilter = NULL;
		}
		else if (filter->m_openalFilter)
		{
			numALFilters++;
		}
		filter->m_openalFilter = false;
	}
	if (numALFilters > 0)
	{
		//TODO: Native filters
	}

    
    
}

#endif
