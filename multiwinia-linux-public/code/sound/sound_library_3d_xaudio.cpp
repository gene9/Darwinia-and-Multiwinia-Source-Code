#include "lib/universal_include.h"

#ifdef HAVE_XAUDIO
#include "sound/sound_library_3d_xaudio.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"

#include <xaudio.h>

//#define AUDIO_DEBUGGING

#ifdef AUDIO_DEBUGGING
#include <sstream>
#include <iomanip>
#endif

#define CHANNELCOUNT 6		// Dolby 5.1

inline
X3DAUDIO_VECTOR & Assign(X3DAUDIO_VECTOR &_a, const Vector3 &_b)
{
	_a.x = _b.x;
	_a.y = _b.y;
	_a.z = _b.z;
	return _a;
}

//
// XAudioChannel
//

class XAudioChannel
{
public:
	XAudioChannel()
		: m_id( -1 ),
		  m_pSourceVoice( NULL ),
		  m_baseFreq( 0.0f ),
		  m_numPackets( 0 ),
		  m_currentPacket( 0 ),
		  m_starved( 0 ),
		  m_packet( NULL ),
		  m_setFreq( -1 ),
		  m_requestedFreq( -1 ),
		  m_lastPacketSubmitted( 0 ),
		  m_packetDuration( 0 ),
		  m_3dMode( 2 ),
		  m_old3dMode( 2 ),
		  m_minDistance( 1.0f )
	{
	}

	~XAudioChannel()
	{
		delete[] m_packet;
		m_packet = NULL;
	}

	bool Initialise( int _id, unsigned _mixFreq, float _packetDuration, unsigned _numPackets )
	{
		m_id = _id;
		m_requestedFreq = m_setFreq = m_baseFreq = _mixFreq;
		m_packetDuration = _packetDuration;
		m_currentPacket = 0;
		m_numPackets = _numPackets;
		m_3dMode = m_old3dMode = 2;
		m_minDistance = 1.0f;

	    XAUDIOSOURCEVOICEINIT sourceVoiceInit = { 0 };
		sourceVoiceInit.Format.SampleType = XAUDIOSAMPLETYPE_16BITPCM;
		sourceVoiceInit.Format.ChannelCount = 1;
		sourceVoiceInit.Format.SampleRate = _mixFreq;
		sourceVoiceInit.MaxPacketCount = _numPackets;

		// 6 Channel output
		XAUDIOCHANNELMAPENTRY ChannelMapEntry[] = {
						{ 0, 0, 1.0f },		// Left
						{ 0, 1, 1.0f },		// Right
						{ 0, 2, 1.0f },		// Centre
						{ 0, 3, 0.0f },
						{ 0, 4, 0.0f },
						{ 0, 5, 0.0f },
		};
		XAUDIOCHANNELMAP ChannelMap;
		ChannelMap.EntryCount = CHANNELCOUNT;
		ChannelMap.paEntries = ChannelMapEntry;

		XAUDIOVOICEOUTPUT Output;
		XAUDIOVOICEOUTPUTENTRY OutputEntry[ 1 ]; // make this an array so we can more easily extend to additional outputs later
		Output.EntryCount = 1;
		Output.paEntries = OutputEntry;
		OutputEntry[ 0 ].pDestVoice = NULL;
		OutputEntry[ 0 ].pChannelMap = &ChannelMap;		
		sourceVoiceInit.pVoiceOutput = &Output;

		// 3D emitter init
		Init3D();

		// Create the voice
		HRESULT hr = XAudioCreateSourceVoice( &sourceVoiceInit, &m_pSourceVoice );
		if( FAILED( hr ) )
			AppDebugOut("XAudioChannel::Initialise - Failed to create xaudio voice. Error code = %#X\n", hr );

		m_packet = new XAUDIOPACKET[_numPackets];
		ZeroMemory( m_packet, sizeof(XAUDIOPACKET) * _numPackets );

		return true;
	}

	void SetVolume( LONG _millibels )
	{		
		//AppDebugOut("XAudioChannel(%d)::SetVolume %d millibels\n", m_id, _millibels );
		XAUDIOVOLUME volume = XAudioMillibelsToVolume( _millibels );
		m_pSourceVoice->SetVolume( volume );
	}

	XAUDIOVOLUME GetVolume() const
	{
		XAUDIOVOLUME result;
		m_pSourceVoice->GetVolume( &result );
		return result;
	}

	void SetFrequency( float _freq )
	{
		if( m_setFreq != _freq )
		{
			if( _freq > 0 && _freq < 48000 * 3 )
				m_requestedFreq = _freq;
			else
				AppDebugOut("XAudioChannel::SetFrequency - attempted to set bogus frequency: %f\n");
		}
	}

	float GetChannelHealth()
	{
		if( !m_pSourceVoice )
			return 0.0f;
		else
		{
			XAUDIOSOURCESTATE sourceState;
			if( m_pSourceVoice->GetVoiceState( &sourceState ) != S_OK)
				return 0.0f;
			
			float health = 0.5f;

			if( sourceState & XAUDIOSOURCESTATE_STARTED )
				health += 0.5;

			if( sourceState & XAUDIOSOURCESTATE_STARVED )
				health -= 0.25;

			if( sourceState & XAUDIOSOURCESTATE_READYPACKET )
				health -= 0.25;

			return health;
		}
	}
	
	void Start()
	{
		m_pSourceVoice->Start( 0 );
	}

	bool Hungry( short *&_samples, int &_numSamples )
	{
		// NB: Another thing to try is dynamic packet size depending on what the
		// channel frequency is.

		XAUDIOSOURCESTATE sourceState;
		HRESULT hr = m_pSourceVoice->GetVoiceState( &sourceState );
		if( hr != S_OK )
		{
			AppDebugOut("Error getting voice state: %#X\n", hr);
			return false;
		}
		
		if( !(sourceState & XAUDIOSOURCESTATE_STARTED) )
			return false;

		if( sourceState & XAUDIOSOURCESTATE_STARVED )			
			m_starved++;

		if( sourceState & XAUDIOSOURCESTATE_READYPACKET )
		{
			if( m_requestedFreq != m_setFreq )			
				SetFrequencyActual( m_requestedFreq );

			XAUDIOPACKET &packet = PreparePacket();
			
			_numSamples = packet.BufferSize / sizeof(short);
			_samples = (short *) packet.pBuffer;
			return true;
		}
		else 
		{
			_samples = NULL;
			_numSamples = 0;
			return false;
		}
	}

	void DebugPrint()
	{
		/* Let's print the state of the buffer */
		
		if( m_pSourceVoice )
		{
			AppDebugOut("XAudioChannel, null voice\n" );
			return;
		}

		XAUDIOSOURCESTATE sourceState;
		HRESULT hr = m_pSourceVoice->GetVoiceState( &sourceState );
		if( hr != S_OK )
			AppDebugOut("Error getting voice state: %#X\n", hr);		
		
		AppDebugOut( "XAudioChannel State -- |%s%s%s%s%s%s%s\n",
			sourceState & XAUDIOSOURCESTATE_STARTED ? "STARTED|" : "",
			sourceState & XAUDIOSOURCESTATE_STARTING ? "STARTING|" : "",
			sourceState & XAUDIOSOURCESTATE_STOPPING ? "STOPPING|" : "",
			sourceState & XAUDIOSOURCESTATE_READYPACKET ? "READYPACKET|" : "",
			sourceState & XAUDIOSOURCESTATE_DISCONTINUITY ? "DISCONTINUITY|" : "",
			sourceState & XAUDIOSOURCESTATE_STARVED ? "STARVED|" : "",
			sourceState & XAUDIOSOURCESTATE_SYNCHRONIZED ? "SYNC|" : "" );
	}

	unsigned NumPacketsQueued()
	{
		unsigned currentPacket;
		HRESULT hr = m_pSourceVoice->GetPacketContext( (LPVOID *) &currentPacket );
		if( SUCCEEDED( hr ) )
		{
			return m_lastPacketSubmitted - currentPacket;
		}
		else
			return 0;
	}

	void Swallow()
	{
		//AppDebugOut( "XAudioChannel::Swallow() called\n" );
		XAUDIOPACKET &packet = m_packet[m_currentPacket];
		packet.pContext = (LPVOID) m_lastPacketSubmitted++;

		HRESULT hr = m_pSourceVoice->SubmitPacket( &packet, 0 );
		if( FAILED( hr ) )
		{
			AppDebugOut( "XAudioChannel::Swallow() - failed to submit audio packet. ErrorCode = %#X\n", hr );
		}
		m_currentPacket = (m_currentPacket + 1) % m_numPackets;
	}

	int GetNumStarvations() const
	{
		return m_starved;
	}

	void ResetStats() 
	{
		m_starved = 0;
	}

	unsigned GetNumSamples() const 
	{
		return m_setFreq * m_packetDuration;
	}

	// 3D Stuff

	void SetMinDistance( float _minDistance )
	{
		m_minDistance = _minDistance;
	}

	void Set3dMode( int _3dMode )
	{
		m_3dMode = _3dMode;
	}

	int Get3dMode() const
	{
		return m_3dMode;
	}

	void SetPosition( Vector3 const &_pos, Vector3 const &_vel )
	{
		Assign(m_emitter.Velocity, _vel);
		Assign(m_emitter.Position, _pos);
	}

	void Calculate3D( const X3DAUDIO_HANDLE _instance, const X3DAUDIO_LISTENER &_listener )
	{		
		if( m_3dMode != 2 )
		{
			X3DAUDIO_LISTENER listener = _listener;

			if( m_3dMode == 1 )
			{
				// Head Relative
				listener.Position.x = listener.Position.y = listener.Position.z = 0;
			}

			if( m_minDistance == 0.0f )
			{
				AppDebugOut("Warning, m_minDist = 0\n, setting to 1" );
				m_minDistance = 1.0f;
			}
			
			listener.Velocity.x /= m_minDistance;
			listener.Velocity.y /= m_minDistance;
			listener.Velocity.z /= m_minDistance;

			listener.Position.x /= m_minDistance;
			listener.Position.y /= m_minDistance;
			listener.Position.z /= m_minDistance;

			X3DAUDIO_EMITTER emitter = m_emitter;
						
			emitter.Velocity.x /= m_minDistance;
			emitter.Velocity.y /= m_minDistance;
			emitter.Velocity.z /= m_minDistance;

			emitter.Position.x /= m_minDistance;
			emitter.Position.y /= m_minDistance;
			emitter.Position.z /= m_minDistance;

			X3DAudioCalculate( _instance, &listener, &emitter, X3DAUDIO_CALCULATE_MATRIX, &m_dspSettings );

			XAUDIOCHANNELVOLUMEENTRY VolumeEntries[] = {
				{ 0, m_dspSettings.pMatrixCoefficients[ 0 ] },
				{ 1, m_dspSettings.pMatrixCoefficients[ 1 ] },
				{ 2, m_dspSettings.pMatrixCoefficients[ 2 ] },
				{ 3, m_dspSettings.pMatrixCoefficients[ 3 ] },
				{ 4, m_dspSettings.pMatrixCoefficients[ 4 ] },
				{ 5, m_dspSettings.pMatrixCoefficients[ 5 ] }    
			};

#ifdef AUDIO_DEBUGGING1
			std::ostringstream s;
			s << m_3dMode << " Listener = (" << std::setprecision( 2 ) << _listener.Position.x << ", " << _listener.Position.y << ", " << _listener.Position.z << ")"
			  << " Emitter = (" << m_emitter.Position.x << ", " << m_emitter.Position.y << ", " << m_emitter.Position.z << ")"
			  << " Matrix: [";
			for(int i = 0; i < 6; i++)
				s << m_dspSettings.pMatrixCoefficients[i] << " ";
			s << "]";
			AppDebugOut( "%s", s.str().c_str() );
#endif

			XAUDIOCHANNELVOLUME ChannelVolume = 
				{ sizeof( VolumeEntries ) / sizeof( VolumeEntries[ 0 ] ), VolumeEntries };

			XAUDIOVOICEOUTPUTVOLUMEENTRY VoiceEntries[] = {
				{ 0, &ChannelVolume }
			};

			XAUDIOVOICEOUTPUTVOLUME Volume = 
				{ 1, VoiceEntries };

			m_pSourceVoice->SetVoiceOutputVolume( &Volume );
		}
		else {
			if( m_old3dMode != 2 )
			{
				// Set up the default channel map entry

				XAUDIOCHANNELVOLUMEENTRY VolumeEntries[] = {
					{ 0, 1 },
					{ 1, 1 },
					{ 2, 1 },
					{ 3, 0 },
					{ 4, 0 },
					{ 5, 0 }    
				};

				XAUDIOCHANNELVOLUME ChannelVolume = 
					{ sizeof( VolumeEntries ) / sizeof( VolumeEntries[ 0 ] ), VolumeEntries };

				XAUDIOVOICEOUTPUTVOLUMEENTRY VoiceEntries[] = {
					{ 0, &ChannelVolume }
				};

				XAUDIOVOICEOUTPUTVOLUME Volume = 
					{ 1, VoiceEntries };

				m_pSourceVoice->SetVoiceOutputVolume( &Volume );
			}
		}

		m_old3dMode = m_3dMode;
	}

private:

	void Init3D()
	{
		ZeroMemory( &m_emitter, sizeof( m_emitter ) );

		m_emitter.OrientFront.x         = 0.0f;
		m_emitter.OrientFront.y         = 0.0f;
		m_emitter.OrientFront.z         = 1.0f;
		m_emitter.OrientTop.x           = 0.0f;
		m_emitter.OrientTop.y           = 1.0f;
		m_emitter.OrientTop.z           = 0.0f;

		m_emitter.ChannelCount          = 1;
		m_emitter.CurveDistanceScaler   = 10000.0f;
		m_emitter.DopplerScaler         = 1.0f;

		m_dspSettings.SrcChannelCount     = 1;
		m_dspSettings.DstChannelCount     = CHANNELCOUNT;
		m_dspSettings.pMatrixCoefficients = m_matrixCoeff;
	}

	void Init3dChannelMap()
	{
		// For 3d mode, we want the channel to be routed to all six channels
		// Rather than just left and right speakers

		XAUDIOCHANNELMAPENTRY ChannelMapEntry[] = {
						{ 0, 0, 0.0f },
						{ 0, 1, 0.0f },
						{ 0, 2, 0.0f },
						{ 0, 3, 0.0f },
						{ 0, 4, 0.0f },
						{ 0, 5, 0.0f },
		};
		XAUDIOCHANNELMAP ChannelMap;
		ChannelMap.EntryCount = 6;
		ChannelMap.paEntries = ChannelMapEntry;

		XAUDIOVOICEOUTPUT Output;
		XAUDIOVOICEOUTPUTENTRY OutputEntry[ 1 ]; // make this an array so we can more easily extend to additional outputs later
		Output.EntryCount = 1;
		Output.paEntries = OutputEntry;
		OutputEntry[ 0 ].pDestVoice = NULL;
		OutputEntry[ 0 ].pChannelMap = &ChannelMap;

		m_pSourceVoice->SetVoiceOutput( &Output );
	}


	void SetFrequencyActual( float _freq )
	{
		XAUDIOPITCH pitch = XAudioSampleRateToPitch( _freq, m_baseFreq );
		HRESULT hr = m_pSourceVoice->SetPitch( pitch );
		if( FAILED( hr ) )
			AppDebugOut("XAudioChannel::SetFrequency - Failed to set pitch on source voice. Error code = %#X\n", hr );
		m_setFreq = _freq;
	}

	XAUDIOPACKET &PreparePacket()
	{
		XAUDIOPACKET &packet = m_packet[m_currentPacket];
	
		unsigned numSamples = GetNumSamples();		
		unsigned requiredBufferSize = numSamples * sizeof(short);

		if( packet.BufferSize < requiredBufferSize )
		{
			delete[] (short *) packet.pBuffer;
			packet.BufferSize = requiredBufferSize;
			packet.pBuffer = (PVOID) new short [numSamples];
		}

		return packet;
	}

private:

	int m_id;
	float m_baseFreq;
	float m_setFreq;
	float m_requestedFreq;
	float m_packetDuration;
	unsigned m_numPackets;
	unsigned m_currentPacket;
	unsigned m_starved;
	unsigned m_lastPacketSubmitted;
	IXAudioSourceVoice *m_pSourceVoice;
    XAUDIOPACKET *m_packet;

	// 3D Audio 
    X3DAUDIO_EMITTER m_emitter;
    X3DAUDIO_DSP_SETTINGS   m_dspSettings;
	float m_matrixCoeff[ CHANNELCOUNT ];

	int m_3dMode;	// 0 = Normal, 1 = Head Relative, 2 = Disabled
	int m_old3dMode;
	float m_minDistance;

};

SoundLibrary3dXAudio::SoundLibrary3dXAudio()
:   m_channels(NULL),
	m_3dSettingsChanged( false )
{
	XAudioSetDebugPrintLevel( XAUDIODEBUGLEVEL_NONE );

	X3DAudioInitialize( 
		SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER |
		SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
		X3DAUDIO_SPEED_OF_SOUND, m_x3dInstance );

	ZeroMemory( &m_listener, sizeof( m_listener ) );
}

SoundLibrary3dXAudio::~SoundLibrary3dXAudio()
{
	delete[] m_channels;
}

bool SoundLibrary3dXAudio::Initialise(int _mixFreq, int _numChannels, bool _hw3d, 
									  int _mainBufNumSamples, int _musicBufNumSamples )
{    
	// The sound system is updated with a frequency of approx 20Hz
	// We need to provide _mixFreq samples/sec
	//                  = _mixFreq / 20 samples per update
	// To be safe, we calculate the packet size with the for an update rate of 15Hz

	const int assumedUpdateFreq = 15;
	const int maxPitchShift = 4;

	m_numChannels = min( _numChannels, GetMaxChannels() ) - 1;	
	m_channels = new XAudioChannel[m_numChannels + 1];

	AppDebugOut( "Initialising XAudio 3D SoundLibrary with %d channels at mix frequency of %d.\n", 
		m_numChannels + 1, _mixFreq );

	for( int i = 0; i < m_numChannels; i++ )
	{
		int numPackets = 3;
		m_channels[i].Initialise( i, _mixFreq, 1.0 / assumedUpdateFreq / numPackets, numPackets );
	}

	m_musicChannelId = m_numChannels;
	m_channels[m_musicChannelId].Initialise( m_musicChannelId, 44100, 10.0 / assumedUpdateFreq, 3 );

	for( int i = 0; i <= m_numChannels; i++ )
	{
		m_channels[i].Start();
	}

	return true;
}

bool SoundLibrary3dXAudio::Hardware3DSupport()
{
	return false;
}

int SoundLibrary3dXAudio::GetMaxChannels()
{           
	return 32;
}

int SoundLibrary3dXAudio::GetCPUOverhead()
{
	return 0;
}

float SoundLibrary3dXAudio::GetChannelHealth(int _channel)
{
	// Used by SoundStats Window
	return m_channels[_channel].GetChannelHealth();
}

int SoundLibrary3dXAudio::GetChannelBufSize(int _channel) const
{
	// Used by SoundSystem::SoundLibraryMainCallback to determine how much silence to fill
	// for a given channel.
	return m_channels[_channel].GetNumSamples();
}

void SoundLibrary3dXAudio::SetChannel3DMode( int _channel, int _mode )
{
	// Called from SoundInstance::StartPlaying
	// Modes from Direct Sound
	//		0 - normal
	//		1 - head relative
	//		2 - disabled

	m_channels[_channel].Set3dMode( _mode );
	m_3dSettingsChanged = true;
}

void SoundLibrary3dXAudio::SetChannelFrequency( int _channel, int _frequency )
{
	m_channels[_channel].SetFrequency( _frequency );
}


void SoundLibrary3dXAudio::SetChannelPosition( int _channel, Vector3 const &_pos, Vector3 const &_vel )
{
	m_channels[_channel].SetPosition( _pos, _vel );
	m_3dSettingsChanged = true;
}

void SoundLibrary3dXAudio::SetChannelMinDistance( int _channel, float _minDistance )
{
	m_channels[_channel].SetMinDistance( _minDistance );
	m_3dSettingsChanged = true;
}

void SoundLibrary3dXAudio::SetChannelVolume( int _channel, float _volume )
{
    float calculatedVolume = -(5.0f - _volume * 0.5f);
    calculatedVolume *= 1000.0f;
    calculatedVolume += m_masterVolume;

	if( calculatedVolume < -10000.0f ) calculatedVolume = -10000.0f;
	if( calculatedVolume > 0.0f ) calculatedVolume = 0.0f;

	m_channels[_channel].SetVolume( calculatedVolume );
}

void SoundLibrary3dXAudio::SetListenerPosition( Vector3 const &_pos, 
												Vector3 const &_front,
												Vector3 const &_up,
												Vector3 const &_vel )
{   
	Assign( m_listener.Position, _pos );
	Assign( m_listener.Velocity, _vel );
	Assign( m_listener.OrientFront, _front );
	Assign( m_listener.OrientTop, _up );
	m_3dSettingsChanged = true;
}

bool SoundLibrary3dXAudio::CallSoundCallback( int _channel, short *_samples, int _numSamples, int *_silenceRemaining )
{
	if( _channel == m_musicChannelId )
		return m_musicCallback( _samples, _numSamples, _numSamples, _silenceRemaining );
	else
		return m_mainCallback( _channel, _samples, _numSamples, _numSamples, _silenceRemaining );
}

void SoundLibrary3dXAudio::AdvanceChannel( int _channel )
{
	XAudioChannel &channel = m_channels[_channel];

	short *samples;
	int numSamples;

	while( channel.Hungry( samples, numSamples ) )
	{
		int silenceRemaining = 0;		

		CallSoundCallback( _channel, samples, numSamples, &silenceRemaining );
		channel.Swallow();
	}
}

void SoundLibrary3dXAudio::Advance()
{
	if( m_3dSettingsChanged )
	{
	    for (int i = 0; i <= m_numChannels; ++i)
			m_channels[i].Calculate3D( m_x3dInstance, m_listener );
		
		m_3dSettingsChanged = false;
	}

    for (int i = 0; i <= m_numChannels; ++i)
    {
		AdvanceChannel( i );
    }

	// Top the music channel up twice 
	// So that music continues if system is doing something else
	AdvanceChannel( m_musicChannelId );

#ifdef AUDIO_DEBUGGING
	double now = GetHighResTime();
	static double lastCheckTime = now;
	static int numUpdates = 0;

	numUpdates++;
	
	if( now > lastCheckTime + 10 )
	{	
		double updatesPerSec = numUpdates / 10.0;

		std::ostringstream s, t, u;
		s << "Audio Updates/Sec = " << updatesPerSec << ".\n"
          << "Starvations:     [";
		t << "Channel Volumes: [" << std::setiosflags( std::ios_base::fixed ) << std::setprecision(2);
		u << "Channel Modes:   [";
	    for( int i = 0; i <= m_numChannels; ++i )
		{
			s << std::setw(5) << m_channels[i].GetNumStarvations();
			t << std::setw(5) << m_channels[i].GetVolume();
			u << std::setw(5) << m_channels[i].Get3dMode();

			if( i != m_numChannels )
			{
				s << " ";
				t << " ";
				u << " ";
			}

			m_channels[i].ResetStats();
		}		
		s << "]\n";
		t << "]\n";
		u << "]\n";
		AppDebugOut( "%s", s.str().c_str() );
		AppDebugOut( "%s", t.str().c_str() );
		AppDebugOut( "%s", u.str().c_str() );

		lastCheckTime = now;
		numUpdates = 0;
	}


#endif

	// Remember to create the music voice with category - background music
	// This will allow the guide to mute it at the appropriate times.
}

void SoundLibrary3dXAudio::ResetChannel( int _channel )
{
	//AppDebugOut("ResetChannel %d\n", _channel );
	AdvanceChannel( _channel );
}

void SoundLibrary3dXAudio::EnableDspFX(int _channel, int _numFilters, int const *_filterTypes)
{
}

void SoundLibrary3dXAudio::UpdateDspFX( int _channel, int _filterType, int _numParams, float const *_params )
{
}

void SoundLibrary3dXAudio::DisableDspFX( int _channel )
{
}
#endif