#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/math/math_utils.h"
#include "lib/profiler.h"
#include "lib/gucci/input.h"

#include "sound_filter.h"
#include "sound_library_3d_software.h"
#include "sound_library_2d.h"



//*****************************************************************************
// Class SoundLibFilterSoftware
//*****************************************************************************

class SoundLibFilterSoftware
{
public:
	int					m_chainIndex;	// Each channel has an 'effects chain'. 
										// This val is the index for this filter in that chain
										// A value of -1 means the filter is NOT active
	DspEffect			*m_userFilter;

	SoundLibFilterSoftware()
	:	m_chainIndex(-1), 
		m_userFilter(NULL) 
	{
	}
};


//*****************************************************************************
// Class SoftwareChannel
//*****************************************************************************

class SoftwareChannel
{
public:
	SoundLibFilterSoftware		m_dspFX[SoundLibrary3d::NUM_FILTERS];

	signed short		*m_buffer;
	unsigned int		m_samplesInBuffer;
	bool				m_containsSilence;		// Updated in callback

	unsigned int		m_freq;					// Value recorded on previous call of SetChannelFrequency
	float				m_volume;				// Value recorded on previous call of SetChannelVolume
	float				m_minDist;				// Value recorded on previous call of SetChannelMinDist
	Vector3<float>		m_pos;					// Value recorded on previous call of SetChannelPosition
	int					m_3DMode;				// Value recorded on previous call of SetChannel3DMode

	float				m_oldVolLeft;			// Volumes used last time this channel was mixed into 
	float				m_oldVolRight;			// the output buffer
	bool				m_forceVolumeJump;		// True when a channel has just had a new sound effect assigned to it - this prevents the volume ramping code from being used when a new sound first starts

public:
    SoftwareChannel();
    void Initialise( bool _stereo );
};


SoftwareChannel::SoftwareChannel()
:	m_freq(1),
	m_volume(0.0f),
	m_oldVolLeft(0.0f),
	m_oldVolRight(0.0f),
	m_forceVolumeJump(true),
	m_containsSilence(true),
	m_minDist(0.1f),
	m_pos(0,0,0),
	m_3DMode(0)
{
}

void SoftwareChannel::Initialise( bool _stereo )
{
	// The 3 on the next line is there because we limit the frequency of a 
	// channel to 3 times that of the mix frequency
	m_samplesInBuffer = g_soundLibrary2d->GetSamplesPerBuffer() * 3;
	if( _stereo ) m_samplesInBuffer *= 5;
	m_buffer = new signed short[m_samplesInBuffer];
	memset(m_buffer, 0, sizeof(signed short) * m_samplesInBuffer);

	for (int i = 0; i < SoundLibrary3dSoftware::NUM_FILTERS; ++i)
	{
		m_dspFX[i].m_userFilter = NULL;
	}
}



//*****************************************************************************
// Class SoundLibrary3dSoftware
//*****************************************************************************

void SoundLib3dSoftwareCallbackWrapper(StereoSample *_buf, unsigned int _numSamples)
{
	AppDebugAssert(g_soundLibrary3d);
	SoundLibrary3dSoftware *lib = (SoundLibrary3dSoftware*)g_soundLibrary3d;
	if (lib)
		lib->Callback(_buf, _numSamples);
}


SoundLibrary3dSoftware::SoundLibrary3dSoftware()
:   SoundLibrary3d(),
	m_channels(NULL),
	m_listenerFront(1,0,0),
	m_listenerUp(0,1,0),
	m_lastVolumeSet(GetMasterVolume())
{
	AppReleaseAssert(g_soundLibrary2d, "SoundLibrary2d must be initialised before SoundLibrary3d");
	
	EnableCallback(true);

	m_left = new float[g_soundLibrary2d->GetSamplesPerBuffer()];
	m_right = new float[g_soundLibrary2d->GetSamplesPerBuffer()];
}


SoundLibrary3dSoftware::~SoundLibrary3dSoftware()
{
	EnableCallback(false);

	delete [] m_channels;		m_channels = NULL;
	m_numChannels = 0;
	delete [] m_left;			m_left = NULL;
	delete [] m_right;			m_right = NULL;
}


void SoundLibrary3dSoftware::Initialise(int _mixFreq, int _numChannels, int _numMusicChannels, bool /* hw3d */)
{
	m_sampleRate = _mixFreq;

	int log2NumChannels = ceilf(Log2(_numChannels));
	m_numChannels = 1 << log2NumChannels;
	AppReleaseAssert(m_numChannels == _numChannels, "Num channels must be a power of 2");
	
	m_channels = new SoftwareChannel[m_numChannels];

    m_numMusicChannels = _numMusicChannels;

    for( int i = 0; i < m_numChannels; ++i )
    {
        bool stereo = i >= m_numChannels - m_numMusicChannels;
        m_channels[i].Initialise( stereo );
    }
}


void SoundLibrary3dSoftware::SetMasterVolume( int _volume )
{
	m_lastVolumeSet = _volume;
	SoundLibrary3d::SetMasterVolume(_volume);
}

void SoundLibrary3dSoftware::Advance()
{
	//
	// Mute everything when in background (the Windows DirectSound version seems to do this
	// automatically)
	if (!g_inputManager->m_windowHasFocus)
	{
		if (GetMasterVolume() != 0)
		{
			m_lastVolumeSet = GetMasterVolume();
			SoundLibrary3d::SetMasterVolume(0);
		}
	}
	else if (m_lastVolumeSet != GetMasterVolume())
		SoundLibrary3d::SetMasterVolume(m_lastVolumeSet);
		
	g_soundLibrary2d->TopupBuffer();
}


void SoundLibrary3dSoftware::GetChannelData(float _duration)
{
	// START_PROFILE2(m_profiler, "GetChannelData");

		int numActiveChannels = 0;
		
		for (int i = 0; i < m_numChannels; ++i)
		{
			int silenceRemaining = -1;	
			unsigned int samplesNeeded;
			if (m_channels[i].m_freq < m_sampleRate)
			{
				samplesNeeded = ceil((double)m_channels[i].m_freq * _duration + 0.5f);
			}
			else
			{
				samplesNeeded = ceil((double)m_channels[i].m_freq * _duration - 0.5f); //floor this
			}
			if( i >= m_numChannels - m_numMusicChannels ) samplesNeeded *= 2;

			if (m_mainCallback)
			{
				m_channels[i].m_containsSilence = 
					!m_mainCallback(i, m_channels[i].m_buffer, samplesNeeded, &silenceRemaining);
			}
			else
			{
				m_channels[i].m_containsSilence = true;
			}
				
			if (!m_channels[i].m_containsSilence)
			{
				numActiveChannels++;
			}
		}
	// END_PROFILE2(m_profiler, "GetChannelData");
}


void SoundLibrary3dSoftware::ApplyDspFX(float _duration)
{
	//START_PROFILE2(m_profiler, "ApplyDspFX");
	{
		for (int i = 0; i < m_numChannels; ++i)
		{
			for( int j = 0; j < NUM_FILTERS; ++j )
		    {
				if (m_channels[i].m_dspFX[j].m_userFilter == NULL) continue;

				unsigned int samplesNeeded;
				if (m_channels[i].m_freq < m_sampleRate)
				{
					samplesNeeded = ceil((double)m_channels[i].m_freq * _duration + 0.5f);
				}
				else
				{
					samplesNeeded = ceil((double)m_channels[i].m_freq * _duration - 0.5f);
				}
				m_channels[i].m_dspFX[j].m_userFilter->Process(m_channels[i].m_buffer, 
																 samplesNeeded);
			}
		}
	}
	// END_PROFILE2(m_profiler, "ApplyDspFX");
}


void SoundLibrary3dSoftware::CalcChannelVolumes(int _channelIndex, 
												float *_left, float *_right)
{
	SoftwareChannel *channel = &m_channels[_channelIndex];

    float calculatedVolume = -(5.0f - channel->m_volume * 0.5f);
    calculatedVolume += m_masterVolume * 0.001f;

	if( calculatedVolume < -10.0f ) calculatedVolume = -10.0f;
	if( calculatedVolume > 0.0f ) calculatedVolume = 0.0f;
	calculatedVolume = expf(calculatedVolume);

	if ( _channelIndex < m_numChannels - m_numMusicChannels )
	{
		Vector3<float> to = channel->m_pos - m_listenerPos;
		float dist = to.Mag();
		to /= dist;
		float dotRight = to * m_listenerRight;
		*_right = (dotRight + 1.0f) * 0.5f;
		*_left = 1.0f - *_right;

		if (dist > channel->m_minDist) 
		{
			float dropOff = channel->m_minDist / dist;
			*_left *= dropOff;
			*_right *= dropOff;
		}
	}
	
	if ( _channelIndex >= m_numChannels - m_numMusicChannels )
	{
		*_left = 0.9f;
		*_right = 0.9f;
	}

	*_left *= calculatedVolume; 
	*_right *= calculatedVolume; 
}


void SoundLibrary3dSoftware::MixStereo(signed short *_inBuf, unsigned int _numSamples,
                                       float _volLeft, float _volRight, float _relativeFreq )
{
    float *left = m_left;
    float *right = m_right;
    int nearestSample;

    for (int j = 0; j < _numSamples; ++j)
    {
        nearestSample = (int)(j * 2 * _relativeFreq);
        if( nearestSample % 2 == 1 ) nearestSample += 1;

        *left += (float)_inBuf[nearestSample] * _volLeft;
        *right += (float)_inBuf[nearestSample+1] * _volRight;
        left++;	right++;
    }
}


void SoundLibrary3dSoftware::MixSameFreqFixedVol(signed short *_inBuf, unsigned int _numSamples,
											 	 float _volLeft, float _volRight)
{
	float *finalLeft = &m_left[_numSamples - 1];
	float *left = m_left;
	float *right = m_right;
	while (left <= finalLeft)
	{
		*left += (float)*_inBuf * _volLeft;
		*right += (float)*_inBuf * _volRight;
		left++;	right++; _inBuf++;
	}
}


void SoundLibrary3dSoftware::MixDiffFreqFixedVol(signed short *_inBuf, unsigned int _numSamples, 
												 float _volLeft, float _volRight, float _relativeFreq)
{
	float *left = m_left;
	float *right = m_right;
	int nearestSample;

	for (int j = 0; j < _numSamples; ++j)
	{
		nearestSample = (int)(j * _relativeFreq); // Was using Round()  // TODO: Find a way to anti-alias
		*left += (float)_inBuf[nearestSample] * _volLeft;
		*right += (float)_inBuf[nearestSample] * _volRight;
		left++;	right++;
	}
}


void SoundLibrary3dSoftware::MixSameFreqRampVol(signed short *_inBuf, unsigned int _numSamples, 
												float _volL1, float _volR1, float _volL2, float _volR2)
{
	float volLeft = _volL1;
	float volRight = _volR1;
	float volLeftInc = (_volL2 - _volL1) / (float)_numSamples;
	float volRightInc = (_volR2 - _volR1) / (float)_numSamples;
	float *finalLeft = &m_left[_numSamples - 1];
	float *left = m_left;
	float *right = m_right;
	while (left <= finalLeft)
	{
		*left += (float)*_inBuf * volLeft;
		*right += (float)*_inBuf * volRight;
		left++;	right++; _inBuf++;
		volLeft += volLeftInc;
		volRight += volRightInc;
	}
}


void SoundLibrary3dSoftware::MixDiffFreqRampVol(signed short *_inBuf, unsigned int _numSamples,
												float _volL1, float _volR1, float _volL2, float _volR2, 
												float _relativeFreq)
{
	float volLeft = _volL1;
	float volRight = _volR1;
	float volLeftInc = (_volL2 - _volL1) / (float)_numSamples;
	float volRightInc = (_volR2 - _volR1) / (float)_numSamples;
	float *left = m_left;
	float *right = m_right;
	int nearestSample;
	for (int j = 0; j < _numSamples; ++j)
	{
		nearestSample = (int)(j * _relativeFreq);	// Was using Round()
		*left += (float)_inBuf[nearestSample] * volLeft;
		*right += (float)_inBuf[nearestSample] * volRight;
		left++;	right++;
		volLeft += volLeftInc;
		volRight += volRightInc;
	}
}


void SoundLibrary3dSoftware::EnableCallback(bool _enabled)
{
	g_soundLibrary2d->SetCallback( _enabled ? SoundLib3dSoftwareCallbackWrapper : NULL );
}


void SoundLibrary3dSoftware::Callback(StereoSample *_buf, unsigned int _numSamples)
{
	if (!m_channels) return;

	double duration = (double)_numSamples / (double)g_soundLibrary2d->GetFreq();

	GetChannelData(duration);
	ApplyDspFX(duration);

	// START_PROFILE2(m_profiler, "Mix");
	memset(m_left, 0, sizeof(float) * _numSamples);
	memset(m_right, 0, sizeof(float) * _numSamples);

	// Merge all the channel's into one stereo stream, converting to floats to
	// prevent overflows and reap the benefits of huge FPU power on Athlons
	// and modern Pentiums.
	for (int i = 0; i < m_numChannels; ++i)
	{
		float volLeft, volRight;
		CalcChannelVolumes(i, &volLeft, &volRight);
		float deltaVolLeft  = 0.0f;
		float deltaVolRight = 0.0f;
		if (m_channels[i].m_forceVolumeJump)
		{
			deltaVolLeft  = volLeft  - m_channels[i].m_oldVolLeft;
			deltaVolRight = volRight - m_channels[i].m_oldVolRight;
		}
		m_channels[i].m_forceVolumeJump = false;
		float const maxDelta = 0.003f;

		// Skip this channel if it contains silence
		if (!m_channels[i].m_containsSilence) 
		{
            float relativeFreq = (float)m_channels[i].m_freq / (float)g_soundLibrary2d->GetFreq();
            signed short *inBuf = m_channels[i].m_buffer;

			// Determine which mixer function to use - some are faster than others
			if( i >= m_numChannels - m_numMusicChannels )
            {
                MixStereo(inBuf, _numSamples, volLeft, volRight, relativeFreq );
            }
            else if (fabsf(deltaVolLeft) < maxDelta && fabsf(deltaVolRight) < maxDelta)
			{
				if (NearlyEquals(relativeFreq, 1.0f))
				{
					MixSameFreqFixedVol(inBuf, _numSamples, volLeft, volRight);
				}
				else
				{
					MixDiffFreqFixedVol(inBuf, _numSamples, volLeft, volRight, relativeFreq);
				}
			}
			else
			{
				if (NearlyEquals(relativeFreq, 1.0f))
				{
					MixSameFreqRampVol(inBuf, _numSamples, 
									   m_channels[i].m_oldVolLeft, m_channels[i].m_oldVolRight,
									   volLeft, volRight);
				}
				else
				{
					MixDiffFreqRampVol(inBuf, _numSamples, 
									   m_channels[i].m_oldVolLeft, m_channels[i].m_oldVolRight,
									   volLeft, volRight, relativeFreq);
				}
			}
		}

		m_channels[i].m_oldVolLeft = volLeft;
		m_channels[i].m_oldVolRight = volRight;
	}

	// Scan the left and right floating point versions of the output buffer to
	// find the largest sample
	float largest = 0.0f;
	for (int i = 0; i < _numSamples; ++i)
	{
		if (fabsf(m_left[i]) > largest)		largest = fabsf(m_left[i]);
		if (fabsf(m_right[i]) > largest)	largest = fabsf(m_right[i]);
	}

	// Convert the stereo stream from floats back to signed shorts
	float scale = 1.0f;
	if (largest > 32766)
	{
		scale = 32766.0f / largest;
	}

	if (scale > 1.0f)
	{
		scale = 1.0f;
	}
	for (int i = 0; i < _numSamples; ++i)
	{
		_buf[i].m_left = Round(m_left[i] * scale);
		_buf[i].m_right = Round(m_right[i] * scale);
	}

	// END_PROFILE2(m_profiler, "Mix");
}


bool SoundLibrary3dSoftware::Hardware3DSupport()
{
    return false;
}


int SoundLibrary3dSoftware::GetMaxChannels()
{           
    return 64;
}


int SoundLibrary3dSoftware::GetCPUOverhead()
{
    return 1;
}


float SoundLibrary3dSoftware::GetChannelHealth(int _channel)
{
	return 1.0f;
}


int SoundLibrary3dSoftware::GetChannelBufSize(int _channel) const
{
	return 0;
}

float SoundLibrary3dSoftware::GetChannelVolume(int _channel) const
{
	return m_channels[_channel].m_volume;
}

float SoundLibrary3dSoftware::GetChannelRelFreq (int _channel) const
{
   return 1.0f;
}

void SoundLibrary3dSoftware::SetChannel3DMode( int _channel, int _mode )
{
	AppAssert ( _channel >= 0 );
	m_channels[_channel].m_3DMode = _mode;
}


void SoundLibrary3dSoftware::SetChannelPosition( int _channel, Vector3<float> const &_pos, Vector3<float> const &_vel )
{
	AppAssert ( _channel >= 0 );
	m_channels[_channel].m_pos = _pos;
}


void SoundLibrary3dSoftware::SetChannelFrequency( int _channel, int _frequency )
{
	AppAssert ( _channel >= 0 );
	m_channels[_channel].m_freq = _frequency;
}


void SoundLibrary3dSoftware::SetChannelMinDistance( int _channel, float _minDistance )
{
	AppAssert ( _channel >= 0 );
	m_channels[_channel].m_minDist = _minDistance;
}


void SoundLibrary3dSoftware::SetChannelVolume( int _channel, float _volume )
{
	AppAssert ( _channel >= 0 );
	m_channels[_channel].m_volume = _volume;
}

void SoundLibrary3dSoftware::SetListenerPosition( Vector3<float> const &_pos, 
                                        Vector3<float> const &_front,
                                        Vector3<float> const &_up,
                                        Vector3<float> const &_vel )
{   
	m_listenerPos = _pos;
	m_listenerFront = _front;
	m_listenerUp = _up;
	m_listenerRight = m_listenerUp ^ m_listenerFront;
}


void SoundLibrary3dSoftware::ResetChannel( int _channel )
{
	m_channels[_channel].m_forceVolumeJump = true;
}


void SoundLibrary3dSoftware::EnableDspFX(int _channel, int _numFilters, int const *_filterTypes)
{
    AppReleaseAssert( _numFilters > 0, "Bad argument passed to EnableFilters" );

	SoftwareChannel *channel = &m_channels[_channel];
    for( int i = 0; i < _numFilters; ++i )
    {
		AppDebugAssert(_filterTypes[i] >= 0 && _filterTypes[i] < NUM_FILTERS);
		
		channel->m_dspFX[_filterTypes[i]].m_chainIndex = i;

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


void SoundLibrary3dSoftware::UpdateDspFX( int _channel, int _filterType, int _numParams, float const *_params )
{
	SoftwareChannel *channel = &m_channels[_channel];

	if (!channel->m_dspFX[_filterType].m_userFilter) return;

    channel->m_dspFX[_filterType].m_userFilter->SetParameters( _params );
}


void SoundLibrary3dSoftware::DisableDspFX( int _channel )
{
	SoftwareChannel *channel = &m_channels[_channel];
	for (int i = 0; i < NUM_FILTERS; ++i)
	{
		SoundLibFilterSoftware *filter = &channel->m_dspFX[i];
		if (filter->m_chainIndex != -1)
		{
			filter->m_chainIndex = -1;
			delete filter->m_userFilter; filter->m_userFilter = NULL;
		}
	}
}


void SoundLibrary3dSoftware::StartRecordToFile(char const *_filename)
{
	g_soundLibrary2d->StartRecordToFile(_filename);
}


void SoundLibrary3dSoftware::EndRecordToFile()
{
	g_soundLibrary2d->EndRecordToFile();
}

