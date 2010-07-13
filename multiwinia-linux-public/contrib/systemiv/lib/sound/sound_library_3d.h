#ifndef INCLUDED_SOUND_LIBRARY_3D_H
#define INCLUDED_SOUND_LIBRARY_3D_H


//*****************************************************************************
// This module provides a 3D sound stage with dynamic filtering on each channel
// separately. Typically around 32 channels can be played simultaneously. 
// These channels are positioned in the 3D sound
// stage by means of distance attenuation, panning, low-pass filtering and 
// reverb. Some of these effects may not be available on low end hardware.
// Currently two implementations are available; one that wraps DirectSound's 
// 3D API and one does all the mixing and positioning itself and sends the
// output to a SoundLibrary2D implementation.
//*****************************************************************************


#include "lib/math/vector3.h"


//*****************************************************************************
// Class SoundLibrary3d
//*****************************************************************************

class SoundLibrary3d
{
protected:    
	int					m_numChannels;			// Total number of channels 
    int                 m_numMusicChannels;
	int					m_sampleRate;
	int					m_masterVolume;
	Vector3<float>		m_listenerPos;			// Records the most recent value passed into SetListenerPos
   
    bool                (*m_mainCallback) (unsigned int, signed short *, unsigned int, int *);
        
public:
    enum
    {
        DSP_DSOUND_CHORUS,			// 0
        DSP_DSOUND_COMPRESSOR,
        DSP_DSOUND_DISTORTION,		// 2
        DSP_DSOUND_ECHO,
        DSP_DSOUND_FLANGER,			// 4
        DSP_DSOUND_GARGLE,
        DSP_DSOUND_I3DL2REVERB,		// 6
        DSP_DSOUND_PARAMEQ,
        DSP_DSOUND_WAVESREVERB,		// 8
        DSP_RESONANTLOWPASS,
        DSP_BITCRUSHER,				// 10
		DSP_GARGLE,
		DSP_ECHO,					// 12
		DSP_SIMPLE_REVERB,
        DSP_SMOOTHER,
        NUM_FILTERS
    };

    bool                m_hw3dDesired;

public:
    SoundLibrary3d();
    virtual ~SoundLibrary3d();

    virtual void    Initialise                  (int _mixFreq, int _numChannels, int _numMusicChannels, bool hw3d) = 0;
    virtual void    EnableCallback              (bool _enabled);
	void		    SetMainCallback             (bool (*_callback) (unsigned int, signed short *, unsigned int, int *));
	virtual int		GetMasterVolume				() const;
    virtual void	SetMasterVolume             (int _volume);

    virtual bool    Hardware3DSupport	        () = 0;
    virtual int     GetMaxChannels		        () = 0;    
    virtual int     GetCPUOverhead		        () = 0;
    virtual float   GetChannelHealth            (int _channel) = 0;					            // 0.0 = BAD, 1.0 = GOOD
	virtual int     GetChannelBufSize	        (int _channel) const = 0;
    virtual float   GetChannelVolume            (int _channel) const = 0;
    virtual float   GetChannelRelFreq           (int _channel) const = 0;
    int			    GetSampleRate		        ();

	int			    GetNumChannels              () const;
    int             GetNumMusicChannels         () const;
        
    virtual void    ResetChannel                (int _channel) = 0;					            // Refills entire channel with data immediately

    virtual void    SetChannel3DMode            (int _channel, int _mode) = 0;		
    virtual void    SetChannelPosition          (int _channel, Vector3<float> const &_pos, Vector3<float> const &_vel) = 0;    
    virtual void    SetChannelFrequency         (int _channel, int _frequency) = 0;
    virtual void    SetChannelMinDistance       ( int _channel, float _minDistance) = 0;
    virtual void    SetChannelVolume            (int _channel, float _volume) = 0;	            // logarithmic, 0.0f - 10.0f

    virtual void    EnableDspFX                 (int _channel, int _numFilters, int const *_filterTypes) = 0;
    virtual void    UpdateDspFX                 (int _channel, int _filterType, int _numParams, float const *_params) = 0;
    virtual void    DisableDspFX                (int _channel) = 0;
    
    virtual void    SetListenerPosition         (Vector3<float> const &_pos, Vector3<float> const &_front,
									             Vector3<float> const &_up, Vector3<float> const &_vel) = 0;

    virtual void    SetDopplerFactor            ( float _doppler );

    virtual void    Advance			            () = 0;
    
    void		    WriteSilence                (signed short *_data, unsigned int _numSamples);
    
	virtual void    StartRecordToFile	        (char const *_filename) {}
	virtual void    EndRecordToFile	            () {}
};


extern SoundLibrary3d *g_soundLibrary3d;


#endif
