#ifndef INCLUDED_SOUND_LIBRARY_3D_XAUDIO_H
#define INCLUDED_SOUND_LIBRARY_3D_XAUDIO_H


#include "sound/sound_library_3d.h"
#include <x3daudio.h>


//*****************************************************************************
// Class SoundLibrary3dXAudio
//*****************************************************************************

class XAudioChannel;

class SoundLibrary3dXAudio: public SoundLibrary3d
{
public:
    SoundLibrary3dXAudio();
    ~SoundLibrary3dXAudio();

    bool Initialise         (int _mixFreq, int _numChannels, 
                             bool hw3d, int _mainBufNumSamples, int _musicBufNumSamples);

    bool Hardware3DSupport	();
    int  GetMaxChannels		();
    int  GetCPUOverhead		();
    float GetChannelHealth  (int _channel);					// 0.0 = BAD, 1.0 = GOOD
	int GetChannelBufSize	(int _channel) const;
        
    void ResetChannel       (int _channel);					// Refills entire channel with data immediately

    void SetChannel3DMode   (int _channel, int _mode);		// 0 = 3d, 1 = head relative, 2 = disabled
    void SetChannelPosition (int _channel, Vector3 const &_pos, Vector3 const &_vel);    
    void SetChannelFrequency(int _channel, int _frequency);
    void SetChannelMinDistance( int _channel, float _minDistance);
    void SetChannelVolume   (int _channel, float _volume);	// logarithmic, 0.0 - 10.0, 0=practially silent

    void EnableDspFX        (int _channel, int _numFilters, int const *_filterTypes);
    void UpdateDspFX        (int _channel, int _filterType, int _numParams, float const *_params);
    void DisableDspFX       (int _channel);
    
    void SetListenerPosition(Vector3 const &_pos, Vector3 const &_front,
                             Vector3 const &_up, Vector3 const &_vel);

    void Advance            ();

private:

	void AdvanceChannel		( int _channel );
	bool CallSoundCallback	( int _channel, short *_samples, int _numSamples, int *_silenceRemaining );

private:

	XAudioChannel			*m_channels;

	// 3D Settings:
	X3DAUDIO_HANDLE			m_x3dInstance;
    X3DAUDIO_LISTENER       m_listener;
	bool					m_3dSettingsChanged;

};


#endif
