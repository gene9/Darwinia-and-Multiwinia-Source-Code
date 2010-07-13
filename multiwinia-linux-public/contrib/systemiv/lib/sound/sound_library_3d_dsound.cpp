#include "lib/universal_include.h"

#include <BASETSD.H>
#include <MMSYSTEM.H>
#include <DSOUND.H>
#include <DXERR9.H>

#include <math.h>

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
//#include "lib/profiler.h"
#include "lib/gucci/window_manager.h"
#include "lib/gucci/window_manager_win32.h"
#include "lib/gucci/input.h"

#include "soundsystem.h"
#include "sound_filter.h"
#include "sound_library_3d_dsound.h"

#define START_PROFILE(x)
#define END_PROFILE(x)

//*****************************************************************************
// Class Misc Static Data
//*****************************************************************************

static char s_dxErrorMsg[512];

#define SOUNDASSERT(x, y)     { if(x != DS_OK && x != S_FALSE) {  \
                                sprintf( s_dxErrorMsg, "%s %s", \
                                DXGetErrorString9(x),   \
                                DXGetErrorDescription9(x) );  \
                                char msg[512];             \
                                sprintf( msg, "DirectSound ERROR\n%s line %d\n\n%s\nError Code : %s\n%s",  \
                                __FILE__, __LINE__,   \
                                y,                    \
                                DXGetErrorString9(x),   \
                                DXGetErrorDescription9(x) );  \
                                AppReleaseAssert( false, msg );  } }                                 


#define NearlyEquals(a, b)	(fabsf((a) - (b)) < 1e-6 ? 1 : 0)


struct DirectFXDescriptor
{
    const GUID *m_sfxClass;
    const GUID *m_interfaceClass;
};


static const DirectFXDescriptor s_dxDescriptors[] = {   &GUID_DSFX_STANDARD_CHORUS,         &IID_IDirectSoundFXChorus8,
                                                        &GUID_DSFX_STANDARD_COMPRESSOR,     &IID_IDirectSoundFXCompressor8,
                                                        &GUID_DSFX_STANDARD_DISTORTION,     &IID_IDirectSoundFXDistortion8,
                                                        &GUID_DSFX_STANDARD_ECHO,           &IID_IDirectSoundFXEcho8,
                                                        &GUID_DSFX_STANDARD_FLANGER,        &IID_IDirectSoundFXFlanger8,
                                                        &GUID_DSFX_STANDARD_GARGLE,         &IID_IDirectSoundFXGargle8,
                                                        &GUID_DSFX_STANDARD_I3DL2REVERB,    &IID_IDirectSoundFXI3DL2Reverb8,
                                                        &GUID_DSFX_STANDARD_PARAMEQ,        &IID_IDirectSoundFXParamEq8,
                                                        &GUID_DSFX_WAVES_REVERB,            &IID_IDirectSoundFXWavesReverb  }     ;



//*****************************************************************************
// Class DirectSoundData
//*****************************************************************************

class DirectSoundData
{
public:
    IDirectSound8       *m_device;
    IDirectSoundBuffer  *m_primaryBuffer;
	DSCAPS				m_caps;             // Cached because some information requests don't require
										    // the value to be read, and some do, and its probably slow
	static char			*m_hwDescription;
	static int			m_hwNumDevices;

	DirectSoundData():
		m_device(NULL),
		m_primaryBuffer(NULL)
	{
	}
};

char *DirectSoundData::m_hwDescription = NULL;
int DirectSoundData::m_hwNumDevices = 0;


//*****************************************************************************
// Class SoundLibFilter
//*****************************************************************************

class SoundLibFilter
{
public:
	DspEffect			*m_userFilter;
	bool				m_dxFilter;			// True if a direct sound filter has been created

	SoundLibFilter(): m_userFilter(NULL), m_dxFilter(false) {}
};


//*****************************************************************************
// Class DirectSoundChannel
//*****************************************************************************

class DirectSoundChannel
{
public:
	SoundLibFilter		    m_dspFX[SoundLibrary3d::NUM_FILTERS];

    IDirectSoundBuffer      *m_bufferInterface;
	IDirectSound3DBuffer    *m_buffer3DInterface;
    unsigned int            m_numChannels;
	unsigned int		    m_numBufferSamples;
    int                     m_lastSampleWritten;
    float                   m_channelHealth;
    int                     m_silenceRemaining;     // Stream has finished, must write m_numBufferSamples worth of silence    
	long				    m_simulatedPlayCursor;
	double				    m_lastUpdate;

	unsigned int		    m_freq;					// Value recorded on previous call of SetChannelFrequency
	float				    m_volume;				// Value recorded on previous call of SetChannelVolume
	float				    m_minDist;				// Value recorded on previous call of SetChannelMinDist
	Vector3<float>		    m_pos;					// Value recorded on previous call of SetChannelPosition
	int					    m_3DMode;				// Value recorded on previous call of SetChannel3DMode
    
public:
    DirectSoundChannel();

	void UpdateSimulatedPlayCursor(long _realPlayCursor);
};


DirectSoundChannel::DirectSoundChannel()
:	m_bufferInterface(NULL),
	m_buffer3DInterface(NULL),
	m_numBufferSamples(0),
    m_numChannels(0),
    m_lastSampleWritten(0),
    m_channelHealth(0.0f),
    m_silenceRemaining(0),
	m_freq(-1),
	m_volume(-1.0f),
	m_minDist(-1.0f),
	m_pos(0,0,0),
	m_3DMode(-1)
{
}


void DirectSoundChannel::UpdateSimulatedPlayCursor(long _playCursor)
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


//*****************************************************************************
// Class SoundLibrary3dDirectSound
//*****************************************************************************

SoundLibrary3dDirectSound::SoundLibrary3dDirectSound()
:   SoundLibrary3d(),
	m_channels(NULL)
{
	m_directSound = new DirectSoundData;
}


SoundLibrary3dDirectSound::~SoundLibrary3dDirectSound()
{
    int errCode;

    //
    // Release secondary buffers

    for( int i = 0; i < m_numChannels; ++i )
    {
        IDirectSoundBuffer8 *buffer = (IDirectSoundBuffer8 *) m_channels[i].m_bufferInterface;
		errCode = buffer->Stop();
        SOUNDASSERT( errCode, "DirectSound failed to stop secondary buffer" );
//        errCode = buffer->SetFX(0, NULL, NULL);
//        SOUNDASSERT( errCode, "DirectSound failed to remove FX from secondary buffer" );
        errCode = buffer->Release();
        SOUNDASSERT( errCode, "DirectSound failed to release secondary buffer" );        
    }
	
	delete m_channels;
	

    //
    // Release primary buffer

    errCode = m_directSound->m_primaryBuffer->Stop();
    SOUNDASSERT( errCode, "DirectSound failed to shutdown primary buffer" );

    errCode = m_directSound->m_primaryBuffer->Release();
    SOUNDASSERT( errCode, "DirectSound failed to shutdown primary buffer" );
    

    //
    // Release Direct Sound Device

    errCode = m_directSound->m_device->Release();
    SOUNDASSERT( errCode, "DirectSound failed to shutdown device" );

    delete m_directSound;
    
}


IDirectSoundBuffer *SoundLibrary3dDirectSound::CreateSecondaryBuffer(int _numSamples, int _numChannels)
{
	int errCode;
	IDirectSoundBuffer *buffer = NULL;

	WAVEFORMATEX wfx;
	ZeroMemory( &wfx, sizeof(WAVEFORMATEX) ); 
	wfx.wFormatTag      = (WORD) WAVE_FORMAT_PCM; 
	wfx.nChannels       = _numChannels; 
	wfx.nSamplesPerSec  = m_sampleRate; // was _mixFreq;
	wfx.wBitsPerSample  = 16; 
	wfx.nBlockAlign     = (WORD) (wfx.wBitsPerSample / 8 * wfx.nChannels);
	wfx.nAvgBytesPerSec = (DWORD) (wfx.nSamplesPerSec * wfx.nBlockAlign);
    
    int flags = DSBCAPS_CTRLFX |
                DSBCAPS_CTRLFREQUENCY | 
                DSBCAPS_CTRLVOLUME |
                DSBCAPS_GETCURRENTPOSITION2;

    if( _numChannels == 1 )
    {
        flags |= DSBCAPS_CTRL3D;
    }

    if( Hardware3DSupport() && m_hw3dDesired )   flags |= DSBCAPS_LOCHARDWARE;
    else                                         flags |= DSBCAPS_LOCSOFTWARE;
                
	DSBUFFERDESC dsbd;
	ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
	dsbd.dwSize        = sizeof(DSBUFFERDESC);
	dsbd.dwFlags       = flags;
	dsbd.dwBufferBytes = _numSamples * 2;
	dsbd.lpwfxFormat   = &wfx;
    dsbd.guid3DAlgorithm = DS3DALG_DEFAULT;
                   
	errCode = m_directSound->m_device->CreateSoundBuffer(&dsbd, &buffer, NULL);
    SOUNDASSERT( errCode, "Direct sound couldn't create a secondary buffer");
    
    // Fill the buffer with zeros to start with
	void *buf1;
	unsigned long buf1Size;
	errCode = buffer->Lock(0,	        // Offset
						   0,	        // Size (ignored because using DSBLOCK_ENTIREBUFFER)
							&buf1,
							&buf1Size,
							NULL,
							NULL,
							DSBLOCK_ENTIREBUFFER);
    SOUNDASSERT( errCode, "Direct Sound couldn't lock buffer" );
    memset( buf1, 0, buf1Size );        
	buffer->Unlock(buf1, buf1Size, NULL, 0);

	// Start the stream playing
	errCode = buffer->Play( 0, 0, DSBPLAY_LOOPING );
	SOUNDASSERT(errCode, "Direct sound couldn't start playing a stream" );

	return buffer;
}


void SoundLibrary3dDirectSound::Initialise(int _mixFreq, int _numChannels, int _numMusicChannels, bool _hw3d)
{    
    int errCode;

    //AppReleaseAssert( g_systemInfo->m_directXVersion >= 9.0f, "Darwinia requires DirectX9 or Greater" );
	AppReleaseAssert( _numChannels > 0, "SoundLibrary3d asked to create too few channels" );


    //
    // Initialise COM
    // We need to do this in order to use the directX FX stuff

    errCode = CoInitialize( NULL );

    
    //
	// Create Direct Sound Device

	errCode = DirectSoundCreate8(NULL,              // Specifies default device 
								 &m_directSound->m_device,
								 NULL);             // Has to be NULL - stupid, stupid Microsoft
	SOUNDASSERT(errCode, "Direct Sound couldn't create a sound device");
    

	errCode = m_directSound->m_device->SetCooperativeLevel(GetWindowManagerWin32()->m_hWnd, DSSCL_PRIORITY);
	SOUNDASSERT(errCode, "Direct Sound couldn't set the cooperative level");

	
    RefreshCapabilities();


	m_sampleRate = _mixFreq;
	m_hw3dDesired = _hw3d;
	m_numChannels = min( _numChannels, GetMaxChannels() );	
    m_numMusicChannels = min( _numMusicChannels, m_numChannels );
    int mainBufNumSamples = 20000;
    
    
	// 
	// Create the Primary Sound Buffer
	
	{		
        int flags = DSBCAPS_PRIMARYBUFFER |
                    DSBCAPS_CTRL3D;
        if( Hardware3DSupport() && m_hw3dDesired )   flags |= DSBCAPS_LOCHARDWARE;
        else                                         flags |= DSBCAPS_LOCSOFTWARE;
        
		DSBUFFERDESC dsbd;
		ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
		dsbd.dwSize        = sizeof(DSBUFFERDESC);
		dsbd.dwFlags       = flags;
		dsbd.dwBufferBytes = 0;
		dsbd.lpwfxFormat   = NULL;
       
		errCode = m_directSound->m_device->CreateSoundBuffer(&dsbd, &m_directSound->m_primaryBuffer, NULL);
		SOUNDASSERT(errCode, "Direct sound couldn't create the primary sound buffer");

		WAVEFORMATEX wfx;
		ZeroMemory( &wfx, sizeof(WAVEFORMATEX) ); 
		wfx.wFormatTag      = (WORD) WAVE_FORMAT_PCM; 
		wfx.nChannels       = 2; 
		wfx.nSamplesPerSec  = _mixFreq; 
		wfx.wBitsPerSample  = 16;
		wfx.nBlockAlign     = (WORD) (wfx.wBitsPerSample / 8 * wfx.nChannels);
		wfx.nAvgBytesPerSec = (DWORD) (wfx.nSamplesPerSec * wfx.nBlockAlign);

		errCode = m_directSound->m_primaryBuffer->SetFormat(&wfx);
		SOUNDASSERT(errCode, "Direct sound couldn't set the primary sound buffer format");
	}

    
	//
	// Create a streaming secondary buffer for each channel

    m_channels = new DirectSoundChannel[ m_numChannels ];

    for( int i = 0; i < m_numChannels-m_numMusicChannels; ++i )
    {
        m_channels[i].m_bufferInterface = CreateSecondaryBuffer(mainBufNumSamples, 1);
		m_channels[i].m_numBufferSamples = mainBufNumSamples;
		m_channels[i].m_lastSampleWritten = m_channels[i].m_numBufferSamples - 1;     // We just filled all of it with zeros remember
		m_channels[i].m_simulatedPlayCursor = -1;
		m_channels[i].m_freq = _mixFreq;
        m_channels[i].m_numChannels = 1;

		// Get the DirectSound3DBuffer interface
		errCode = m_channels[i].m_bufferInterface->QueryInterface( IID_IDirectSound3DBuffer, 
														  (void **) &m_channels[i].m_buffer3DInterface );
		SOUNDASSERT(errCode, "Direct sound couldn't get Sound3DBuffer interface");
	}    

    for( int i = m_numChannels-m_numMusicChannels; i < m_numChannels; ++i )
    {
        m_channels[i].m_bufferInterface = CreateSecondaryBuffer(mainBufNumSamples*10, 2);
		m_channels[i].m_numBufferSamples = mainBufNumSamples * 10;
		m_channels[i].m_lastSampleWritten = m_channels[i].m_numBufferSamples - 1;     // We just filled all of it with zeros remember
		m_channels[i].m_simulatedPlayCursor = -1;
		m_channels[i].m_freq = _mixFreq;
        m_channels[i].m_numChannels = 2;

        m_channels[i].m_buffer3DInterface = NULL;
    }
}


void SoundLibrary3dDirectSound::RefreshCapabilities()
{
    ZeroMemory( &m_directSound->m_caps, sizeof(DSCAPS) );
    m_directSound->m_caps.dwSize = sizeof(DSCAPS);
    int errCode = m_directSound->m_device->GetCaps( &m_directSound->m_caps );    
    SOUNDASSERT(errCode, "Direct sound couldn't get driver caps");
}


BOOL CALLBACK DSEnumProc(LPGUID lpGUID, 
                         LPCTSTR lpszDesc,
                         LPCTSTR lpszDrvName, 
                         LPVOID lpContext )
{
    if( lpGUID )
    {
        char thisDriver[512];
        sprintf( thisDriver, "%d : %s\n", DirectSoundData::m_hwNumDevices, lpszDesc );
        strcat( DirectSoundData::m_hwDescription, thisDriver );
        DirectSoundData::m_hwNumDevices++;
    }
    return true;
}


bool SoundLibrary3dDirectSound::Hardware3DSupport()
{
    return( m_directSound->m_caps.dwMaxHw3DStreamingBuffers > 0 );
}


int SoundLibrary3dDirectSound::GetMaxChannels()
{           
    if( Hardware3DSupport() && m_hw3dDesired )
    {
        return m_directSound->m_caps.dwMaxHw3DStreamingBuffers - 2;
    }
    else
    {
        return 64;
    }
}


int SoundLibrary3dDirectSound::GetCPUOverhead()
{
    RefreshCapabilities();
    int total = m_directSound->m_caps.dwPlayCpuOverheadSwBuffers;

    for( int i = 0; i < m_numChannels; ++i )
    {
        DSBCAPS caps;
        ZeroMemory( &caps, sizeof(caps) );
        caps.dwSize = sizeof(DSBCAPS);
        int errCode = m_channels[i].m_bufferInterface->GetCaps( &caps );
        SOUNDASSERT( errCode, "Direct sound couldn't get CPU overhead" );

        total += caps.dwPlayCpuOverhead;
    }

    return total;
}


float SoundLibrary3dDirectSound::GetChannelHealth(int _channel)
{
	DirectSoundChannel *channel = &m_channels[_channel];

    if( channel->m_channelHealth >= 0.5f )
    {
        return 1.0f;
    }
    else
    {
        return channel->m_channelHealth * 2.0f;
    }
}


int SoundLibrary3dDirectSound::GetChannelBufSize(int _channel) const
{
	return m_channels[_channel].m_numBufferSamples;
}


float SoundLibrary3dDirectSound::GetChannelVolume(int _channel) const
{
    //
    // Easy way

    //return m_channels[_channel].m_volume;


    //
    // Complex way - query directX about channel volume
    // convert from dX format (0 to -10000) to our format (0 to 10)
    
	DirectSoundChannel *channel = &m_channels[_channel];
	IDirectSoundBuffer *buffer = channel->m_bufferInterface;
    long actualVolume;
    int errCode = buffer->GetVolume( &actualVolume );

    float setVolume = actualVolume - m_masterVolume;
    setVolume /= 1000.0f;
    setVolume = ( setVolume + 5.0f ) / 0.5f;

    return setVolume;
}


void SoundLibrary3dDirectSound::SetChannel3DMode( int _channel, int _mode )
{
	DirectSoundChannel *channel = &m_channels[_channel];

	if (_mode != channel->m_3DMode)
	{
		channel->m_3DMode = _mode;

		IDirectSound3DBuffer *buffer3d = channel->m_buffer3DInterface;
		int errCode;

		switch( _mode )
		{
			case 0 :            errCode = buffer3d->SetMode( DS3DMODE_NORMAL,         DS3D_IMMEDIATE );           break;
			case 1 :            errCode = buffer3d->SetMode( DS3DMODE_HEADRELATIVE,   DS3D_IMMEDIATE );           break;
			case 2 :            errCode = buffer3d->SetMode( DS3DMODE_DISABLE,        DS3D_IMMEDIATE );           break;
		};

		SOUNDASSERT(errCode, "Direct sound couldn't set channel 3D mode" );
	}
}


void SoundLibrary3dDirectSound::SetChannelPosition( int _channel, Vector3<float> const &_pos, Vector3<float> const &_vel )
{
	DirectSoundChannel *channel = &m_channels[_channel];

	Vector3<float> listenerToOldPos(_pos - m_listenerPos);
	Vector3<float> oldPosToNewPos(_pos - channel->m_pos);
	if (oldPosToNewPos.MagSquared() > listenerToOldPos.MagSquared() * 0.0025f)
	{
		channel->m_pos = _pos;

		IDirectSound3DBuffer *buffer3d = channel->m_buffer3DInterface;
		int errCode = buffer3d->SetPosition( _pos.x, _pos.y, _pos.z, DS3D_DEFERRED );
		SOUNDASSERT(errCode, "Direct sound couldn't set buffer3d position");

		errCode = buffer3d->SetVelocity( _vel.x, _vel.y, _vel.z, DS3D_DEFERRED );
		SOUNDASSERT(errCode, "Direct sound couldn't set buffer3d velocity");
	}
}

float SoundLibrary3dDirectSound::GetChannelRelFreq (int _channel) const
{
#ifdef SOUND_USE_DSOUND_FREQUENCY_STUFF
    return 1.0f;
#endif

    if( _channel >= m_numChannels - m_numMusicChannels )
    {
        // Note : This is not correct, as it assumes playback speed is 44Khz
        return m_channels[_channel].m_freq / 44100.0f;  
    }
    
    return 1.0f;
}


void SoundLibrary3dDirectSound::SetChannelFrequency( int _channel, int _frequency )
{
	DirectSoundChannel *channel = &m_channels[_channel];
   
	if(channel->m_freq != _frequency)
	{
		channel->m_freq = _frequency;
        
        bool setFreqDirectly = _channel < m_numChannels - m_numMusicChannels;

#ifdef SOUND_USE_DSOUND_FREQUENCY_STUFF
        setFreqDirectly = true;
#endif

        if( setFreqDirectly )
        {
            IDirectSoundBuffer *buffer = channel->m_bufferInterface;
            int errCode = buffer->SetFrequency( _frequency );
            SOUNDASSERT( errCode, "Direct sound couldn't set channel frequency" );
        }
	}
}


void SoundLibrary3dDirectSound::SetChannelMinDistance( int _channel, float _minDistance )
{
	DirectSoundChannel *channel = &m_channels[_channel];
    
	if (!NearlyEquals(_minDistance, channel->m_minDist))
	{
        AppReleaseAssert( _minDistance >= DS3D_DEFAULTMINDISTANCE && 
                       _minDistance <= DS3D_DEFAULTMAXDISTANCE,
                       "Channel MinDistance must be between %2.2f and %2.2f."
                       "You requested %2.2f", 
                       DS3D_DEFAULTMINDISTANCE, DS3D_DEFAULTMAXDISTANCE,
                       _minDistance );
        

		channel->m_minDist = _minDistance;
		
		IDirectSound3DBuffer *buffer3d = channel->m_buffer3DInterface;
		int errCode = buffer3d->SetMinDistance( _minDistance, DS3D_DEFERRED );
		SOUNDASSERT(errCode, "Direct sound couldn't set buffer3d min distance");
	}
}


void SoundLibrary3dDirectSound::SetChannelVolume( int _channel, float _volume )
{
	DirectSoundChannel *channel = &m_channels[_channel];

	if (!NearlyEquals(_volume, channel->m_volume))
	{
		AppDebugAssert(_volume >= 0.0f && _volume <= 10.0f);

		channel->m_volume = _volume;

        float calculatedVolume = -(5.0f - _volume * 0.5f);
        calculatedVolume *= 1000.0f;
        calculatedVolume += m_masterVolume;

		if( calculatedVolume < -10000.0f ) calculatedVolume = -10000.0f;
		if( calculatedVolume > 0.0f ) calculatedVolume = 0.0f;

		IDirectSoundBuffer *buffer = channel->m_bufferInterface;
		int errCode = buffer->SetVolume( calculatedVolume );
		SOUNDASSERT( errCode, "Direct sound couldn't set buffer volume" );        
	}
}


void SoundLibrary3dDirectSound::SetListenerPosition( Vector3<float> const &_pos, 
                                                     Vector3<float> const &_front,
                                                     Vector3<float> const &_up,
                                                     Vector3<float> const &_vel )
{   
	m_listenerPos = _pos;

    IDirectSound3DListener *listener = NULL;
    int errCode = m_directSound->m_primaryBuffer->QueryInterface( IID_IDirectSound3DListener, (void **) &listener );
    SOUNDASSERT(errCode, "Direct sound couldn't get Sound3DListener interface");

    errCode = listener->SetPosition( _pos.x, _pos.y, _pos.z, DS3D_DEFERRED );
    SOUNDASSERT( errCode, "Direct sound couldn't set listener position" );

    errCode = listener->SetOrientation( _front.x, _front.y, _front.z, 
                                        _up.x, _up.y, _up.z, DS3D_DEFERRED );
    SOUNDASSERT( errCode, "Direct sound couldn't set listener orientation" );

    errCode = listener->SetVelocity( _vel.x, _vel.y, _vel.z, DS3D_DEFERRED );
    SOUNDASSERT( errCode, "Direct sound couldn't set listener velocity" );
}


void SoundLibrary3dDirectSound::SetDopplerFactor( float _doppler )
{
    IDirectSound3DListener *listener = NULL;
    int errCode = m_directSound->m_primaryBuffer->QueryInterface( IID_IDirectSound3DListener, (void **) &listener );
    SOUNDASSERT(errCode, "Direct sound couldn't get Sound3DListener interface");

    errCode = listener->SetDopplerFactor( _doppler, DS3D_IMMEDIATE );
    SOUNDASSERT(errCode, "Direct sound couldn't set Doppler factor" );
}



void SoundLibrary3dDirectSound::CommitChanges()
{
    IDirectSound3DListener *listener = NULL;
    int errCode = m_directSound->m_primaryBuffer->QueryInterface( IID_IDirectSound3DListener, (void **) &listener );
    SOUNDASSERT(errCode, "Direct sound couldn't get Sound3DListener interface");

    errCode = listener->CommitDeferredSettings();
    SOUNDASSERT( errCode, "Direct sound couldn't commit deferred listener changes");
}


void SoundLibrary3dDirectSound::PopulateBuffer(int _channel, int _fromSample, int _numSamples)
{
    int errCode;
    DirectSoundChannel *channel = &m_channels[_channel];

    int byteOffset = _fromSample * 2;
    int byteSize = _numSamples * 2;


    //
    // Lock the buffer

    void *buf1, *buf2;
    unsigned long size1, size2;
	START_PROFILE( "LockBuf");
    errCode = channel->m_bufferInterface->Lock(byteOffset, byteSize, 
                                                 &buf1, &size1, 
                                                 &buf2, &size2,
                                                 0);
	END_PROFILE( "LockBuf");
    SOUNDASSERT(errCode, "Direct sound couldn't get a lock on a secondary buffer");


    //
    // Fill some of the buffer 

	START_PROFILE( "FillBuf");
	if(m_mainCallback)
    {
        if( buf1 )
        {
            m_mainCallback(_channel, (signed short *)buf1, size1/2, &channel->m_silenceRemaining);
        }
        if( buf2 )
        {
            m_mainCallback(_channel, (signed short *)buf2, size2/2, &channel->m_silenceRemaining);
        }
    }
	END_PROFILE( "FillBuf");


    //
    // Apply any non DirectSound filters

    for( int i = DSP_RESONANTLOWPASS; i < NUM_FILTERS; ++i )
    {
		if (channel->m_dspFX[i].m_userFilter == NULL) continue;

		if (buf1)
		{
	        channel->m_dspFX[i].m_userFilter->Process( (signed short *)buf1, size1/2 );
		}
		if (buf2)
		{
	        channel->m_dspFX[i].m_userFilter->Process( (signed short *)buf2, size2/2 );
		}
    }

    //
    // Unlock the buffer

	START_PROFILE( "UnlockBuf");
    errCode = channel->m_bufferInterface->Unlock(buf1, size1, buf2, size2);
	END_PROFILE( "UnlockBuf");
    SOUNDASSERT(errCode, "Direct sound couldn't unlock a secondary buffer");
        
    channel->m_lastSampleWritten = _fromSample + _numSamples - 1;
    channel->m_lastSampleWritten %= channel->m_numBufferSamples;
    channel->m_channelHealth = 1.0f - ( (float) _numSamples / (float) channel->m_numBufferSamples );
}


long SoundLibrary3dDirectSound::CalcWrappedDelta(long a, long b, unsigned long bufferSize)
{
	long delta = b - a;
	long halfSize = bufferSize / 2;
	
	if (delta > halfSize)
	{
		delta -= bufferSize;
	}
	else if (delta < -halfSize)
	{
		delta += bufferSize;
	}

	return delta;
}


void SoundLibrary3dDirectSound::AdvanceChannel(int _channel, int _frameNum)
{
    int errCode;
	DirectSoundChannel *channel = &m_channels[_channel];	

	// Get Play Cursor
    
	START_PROFILE( "GetPlayCursor");
	unsigned long playCursor;               
	if (channel->m_simulatedPlayCursor == -1 ||
		(_channel & 1) == (_frameNum&1))
	{
		errCode = channel->m_bufferInterface->GetCurrentPosition(&playCursor, NULL);
		SOUNDASSERT(errCode, "Direct sound couldn't get current position");
		channel->UpdateSimulatedPlayCursor(playCursor);
	}
	else
	{
		channel->UpdateSimulatedPlayCursor(-1);
		int sizeInBytes = channel->m_numBufferSamples * 2;
		playCursor = channel->m_simulatedPlayCursor + sizeInBytes - 1000;
		playCursor %= sizeInBytes;
		long delta = CalcWrappedDelta(channel->m_lastSampleWritten * 2, playCursor, sizeInBytes);
		if (delta < 0)
		{
			playCursor = channel->m_lastSampleWritten * 2 + 2;
		}
	}
	END_PROFILE( "GetPlayCursor");
	
	
    // Find out where we can write our samples

    int firstSample = channel->m_lastSampleWritten + 1;
    int playCursorAhead = playCursor/2;
    if( playCursor/2 < channel->m_lastSampleWritten )
    {
        playCursorAhead += channel->m_numBufferSamples;
    }
    int numSamples = playCursorAhead - firstSample - 1;
    
    // Always write an even number of samples (or else stereo is fucked)
    if( numSamples % 2 == 1 ) numSamples++;                                 

    if( firstSample >= channel->m_numBufferSamples )
    {
        firstSample -= channel->m_numBufferSamples;
    }

    
    // Populate our buffer

    if (numSamples > 0)
    {            
		START_PROFILE( "PopulateBuf");
		PopulateBuffer( _channel, firstSample, numSamples);
		END_PROFILE( "PopulateBuf");
    }
}


int SoundLibrary3dDirectSound::GetNumFilters(int _channel)
{
	DirectSoundChannel *chan = &m_channels[_channel];
	
	int numFilters = 0;

	for (int j = 0; j < NUM_FILTERS; ++j)
	{
		if (chan->m_dspFX[j].m_userFilter || chan->m_dspFX[j].m_dxFilter)
		{
			numFilters++;
		}
	}

	return numFilters;
}


void SoundLibrary3dDirectSound::Verify()
{
	for (int i = 0; i < m_numChannels; ++i)
	{
		int numFilters = GetNumFilters(i);
		AppDebugAssert(numFilters <= 1);
	}
}


void SoundLibrary3dDirectSound::Advance()
{
//	Verify();


    //
    // ========================================================================
    // This horrible bit of code fixes a strange bug that causes random volumes
    // on the music channels after focus has been lost.  this seems to occur if
    // a piece of music enters and completes its release phase while out of focus.
    // This fix basically resets the volumes a short while after regaining focus.
    // Hilariously, anything less than around 300ms doesn't work - implying we are
    // waiting for something inside directX to fix itself

    static float regainedFocus = 0;
    if( !g_inputManager->m_windowHasFocus )
    {
        regainedFocus = GetHighResTime() + 1.0f;
    }

    if( g_inputManager->m_windowHasFocus && 
        regainedFocus > 0.0f &&
        GetHighResTime() > regainedFocus )
    {
        for( int i = 0; i < m_numChannels; ++i )
        {
            SetChannelVolume( i, 0 );
        }
        regainedFocus = 0.0f;
        AppDebugOut( "Sound System regained window focus\n" );
    }

    // ========================================================================
    //



    START_PROFILE( "Commit");
    CommitChanges();
	END_PROFILE( "Commit");

	static int frameNum = 0;
	++frameNum;
	if (frameNum >= 4)
	{
		frameNum = 0;
	}

    for (int i = 0; i < m_numChannels; ++i)
    {
		AdvanceChannel(i, frameNum);
    }
}


void SoundLibrary3dDirectSound::ResetChannel( int _channel )
{    
	DirectSoundChannel *channel = &m_channels[_channel];	
    int errCode;


	AppDebugAssert(_channel >= -1);


	//
    // Get Play Cursor

    unsigned long playCursor;
    unsigned long writeCursor;
    errCode = channel->m_bufferInterface->GetCurrentPosition(&playCursor, &writeCursor);
    SOUNDASSERT(errCode, "Direct sound couldn't get current position");
	

	//
    // Find out where we can write our samples
   
    int firstSample = writeCursor/2;
    int playCursorAhead = playCursor/2;
    if( playCursor/2 < firstSample )
    {
        playCursorAhead += channel->m_numBufferSamples;
    }
    int numSamples = playCursorAhead - firstSample - 1;

    // Always write an even number of samples (or else stereo is fucked)
    if( numSamples % 2 == 1 ) numSamples--;
    
    if( firstSample >= channel->m_numBufferSamples )
    {
        firstSample -= channel->m_numBufferSamples;
    }


	//
    // Populate our buffer

    if (numSamples > 0)
    {           
		PopulateBuffer( _channel, firstSample, numSamples );
		channel->m_channelHealth = 1.0f;
    }
}


void SoundLibrary3dDirectSound::EnableDspFX(int _channel, int _numFilters, int const *_filterTypes)
{
    AppReleaseAssert( _numFilters > 0, "Bad argument passed to EnableFilters" );

	AppDebugAssert(_channel >= 0 && _channel < m_numChannels);
	DirectSoundChannel *channel = &m_channels[_channel];

    int errCode;
    IDirectSoundBuffer8 *buffer = (IDirectSoundBuffer8 *) channel->m_bufferInterface;


	if (GetNumFilters(_channel) > 0)
	{
		DisableDspFX(_channel);
	}


    //
    // Prepare the sound descriptions

    DSEFFECTDESC desc[DSP_DSOUND_WAVESREVERB];
    int numDSoundFilters = 0;
	for( int i = 0; i < _numFilters; ++i )
    {
		if (_filterTypes[i] <= DSP_DSOUND_WAVESREVERB)
		{
			ZeroMemory( &desc[numDSoundFilters], sizeof(DSEFFECTDESC) );
			desc[numDSoundFilters].dwSize = sizeof( DSEFFECTDESC );
			desc[numDSoundFilters].dwFlags = DSFX_LOCSOFTWARE;    
			desc[numDSoundFilters].guidDSFXClass = *s_dxDescriptors[ _filterTypes[i] ].m_sfxClass;

			AppDebugAssert(
				desc[numDSoundFilters].guidDSFXClass == GUID_DSFX_STANDARD_CHORUS ||
				desc[numDSoundFilters].guidDSFXClass == GUID_DSFX_STANDARD_COMPRESSOR ||
				desc[numDSoundFilters].guidDSFXClass == GUID_DSFX_STANDARD_DISTORTION ||
				desc[numDSoundFilters].guidDSFXClass == GUID_DSFX_STANDARD_ECHO ||
				desc[numDSoundFilters].guidDSFXClass == GUID_DSFX_STANDARD_FLANGER ||
				desc[numDSoundFilters].guidDSFXClass == GUID_DSFX_STANDARD_GARGLE ||
				desc[numDSoundFilters].guidDSFXClass == GUID_DSFX_STANDARD_I3DL2REVERB ||
				desc[numDSoundFilters].guidDSFXClass == GUID_DSFX_STANDARD_PARAMEQ ||
				desc[numDSoundFilters].guidDSFXClass == GUID_DSFX_WAVES_REVERB
				);
			++numDSoundFilters;
		}
    }
    
	unsigned long results[NUM_FILTERS];
	if (numDSoundFilters > 0)
	{
		//
		// Stop the channel

		errCode = buffer->Stop();
		SOUNDASSERT( errCode, "Direct sound couldn't stop a secondary buffer from playing" );


		//
		// Set up the filters

		errCode = buffer->SetFX( numDSoundFilters, desc, results );
		SOUNDASSERT( errCode, "Direct sound couldn't set fx" );


		//
		// Restart the channel

		errCode = buffer->Play( 0, 0,  DSBPLAY_LOOPING );
		SOUNDASSERT( errCode, "Direct sound couldn't play a secondary buffer" );
	}


    for( int i = 0; i < _numFilters; ++i )
    {
		AppDebugAssert(_filterTypes[i] >= 0);
		
		if (_filterTypes[i] <= DSP_DSOUND_WAVESREVERB)
		{
	        AppReleaseAssert( results[i] == DSFXR_LOCSOFTWARE, "DirectSound couldn't enable filter" );
			channel->m_dspFX[i].m_dxFilter = true;
		}

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
            case DSP_SMOOTHER:
                channel->m_dspFX[_filterTypes[i]].m_userFilter = new DspSmoother(m_sampleRate);
                break;
		}
    }
}


void SoundLibrary3dDirectSound::UpdateDspFX( int _channel, int _filterType, int _numParams, float const *_params )
{
	AppDebugAssert(_channel >= 0 && _channel < m_numChannels);
	AppDebugAssert(GetNumFilters(_channel) > 0);

	DirectSoundChannel *channel = &m_channels[_channel];

	if (_filterType <= DSP_DSOUND_WAVESREVERB)
	{
		int errCode;
		IDirectSoundBuffer8 *buffer = (IDirectSoundBuffer8 *) channel->m_bufferInterface;
        
		IUnknown *effect;
		GUID sfxClass = *s_dxDescriptors[_filterType].m_sfxClass;
		GUID interfaceClass = *s_dxDescriptors[_filterType].m_interfaceClass;
		errCode = buffer->GetObjectInPath(sfxClass, 
										  0, 
										  interfaceClass, 
										  (void **) &effect );
		SOUNDASSERT( errCode, "Failed to get hold of DirectFX filter" );
    
		switch( _filterType )
		{
			case DSP_DSOUND_CHORUS:         
				errCode = ((IDirectSoundFXChorus8 *)effect)->SetAllParameters( (DSFXChorus *)_params );          
				break;
			case DSP_DSOUND_COMPRESSOR:     
				errCode = ((IDirectSoundFXCompressor8 *)effect)->SetAllParameters( (DSFXCompressor *)_params );          
				break;
			case DSP_DSOUND_DISTORTION:     
				errCode = ((IDirectSoundFXDistortion8 *)effect)->SetAllParameters( (DSFXDistortion *)_params );          
				break;
			case DSP_DSOUND_ECHO:           
				errCode = ((IDirectSoundFXEcho8 *)effect)->SetAllParameters( (DSFXEcho *)_params );          
				break;
			case DSP_DSOUND_FLANGER:        
				errCode = ((IDirectSoundFXFlanger8 *)effect)->SetAllParameters( (DSFXFlanger *)_params );          
				break;
			case DSP_DSOUND_GARGLE:         
				errCode = ((IDirectSoundFXGargle8 *)effect)->SetAllParameters( (DSFXGargle *)_params );          
				break;
			case DSP_DSOUND_PARAMEQ:        
				errCode = ((IDirectSoundFXParamEq8 *)effect)->SetAllParameters( (DSFXParamEq *)_params );          
				break;
			case DSP_DSOUND_WAVESREVERB:    
				errCode = ((IDirectSoundFXWavesReverb8 *)effect)->SetAllParameters( (DSFXWavesReverb *)_params );          
				break;
			case DSP_DSOUND_I3DL2REVERB:    
				errCode = ((IDirectSoundFXI3DL2Reverb8 *)effect)->SetAllParameters( (DSFXI3DL2Reverb *)_params );
				errCode = ((IDirectSoundFXI3DL2Reverb8 *)effect)->SetQuality( _params[12] );                                 
				break;
			default:
				AppReleaseAssert( false, "Unknown filter type" );
		}
	
		SOUNDASSERT( errCode, "Failed to set filter parameters" );
	}
    else
	{
		AppDebugAssert(channel->m_dspFX[_filterType].m_userFilter);
        channel->m_dspFX[_filterType].m_userFilter->SetParameters( _params );
    }
}


void SoundLibrary3dDirectSound::DisableDspFX( int _channel )
{
    int errCode;

	AppDebugAssert(GetNumFilters(_channel) > 0);

   	DirectSoundChannel *channel = &m_channels[_channel];

	IDirectSoundBuffer8 *buffer = (IDirectSoundBuffer8 *) channel->m_bufferInterface;

	
    //
    // Stop the channel

    errCode = buffer->Stop();
    SOUNDASSERT( errCode, "Direct sound couldn't stop a secondary buffer" );

    
    //
    // Shut down the filters

	for (int i = 0; i < NUM_FILTERS; ++i)
	{
		SoundLibFilter *filter = &channel->m_dspFX[i];
		if (filter->m_userFilter != NULL)
		{
			filter->m_dxFilter = false;
			delete filter->m_userFilter; 
            filter->m_userFilter = NULL;
		}
	}
	errCode = buffer->SetFX( 0, NULL, NULL );
    SOUNDASSERT( errCode, "Direct sound couldn't set fx" );

    
    //
    // Restart the channel

    errCode = buffer->Play( 0, 0,  DSBPLAY_LOOPING );
    SOUNDASSERT( errCode, "Direct sound couldn't play a secondary buffer" );    
}
