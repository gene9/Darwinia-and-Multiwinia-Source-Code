#ifndef INCLUDED_SOUND_LIBRARY_3D_OPENAL_H
#define INCLUDED_SOUND_LIBRARY_3D_OPENAL_H


#include "sound/sound_library_3d.h"

#ifdef TARGET_OS_MSVC
  #include <al.h>
  #include <alc.h>
#else
  #include <AL/al.h>
  #include <AL/alc.h>
#endif

#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
#include "lib/netlib/net_lib.h"
#endif

class OpenALChannel;

//*****************************************************************************
// Class SoundLibrary3dOpenAL
//*****************************************************************************

class SoundLibrary3dOpenAL: public SoundLibrary3d
{
protected:    
	OpenALChannel  *m_channels;
	OpenALChannel	*m_musicChannel;
	ALCdevice	*m_device;
	ALCcontext	*m_context;
	
	Vector3		m_listenerFront;
	Vector3		m_listenerUp;
	
	int		m_prevMasterVolume;
	/*Vector3		m_prevListenerPos;
	Vector3		m_prevListenerFront;
	Vector3		m_prevListenerUp;*/
	
	char		*m_tempBuffer;
      
protected:
	void PopulateBuffer		(int _channel, int _fromSample, int _numSamples, bool _isMusic, char *buffer);
	void CommitChangesGlobal	();							// Commits global volume changes.
	void CommitChangesChannel	(OpenALChannel *channel);				// Commits all pos/or/vel etc changes
	void AdvanceChannel		(int _channel, int _frameNum);
	void ActuallyResetChannel	(int _channel, OpenALChannel *channel);
	int  GetNumFilters		(int _channel);
	void Verify				();

public:
    SoundLibrary3dOpenAL();
    ~SoundLibrary3dOpenAL();

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
	void ActuallyAdvance		();							// Actually updates the sound system.
};


#endif
