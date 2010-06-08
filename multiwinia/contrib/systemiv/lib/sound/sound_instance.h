#ifndef INCLUDED_SOUND_INSTANCE_H
#define INCLUDED_SOUND_INSTANCE_H

#include "lib/math/vector3.h"
#include "sound_parameter.h"
#include "soundsystem_interface.h"


class SoundInstance;
class SoundSampleDecoder;
class DspEffect;
class DspHandle;
class SoundSampleHandle;



// ============================================================================
// class SoundInstanceId

class SoundInstanceId
{
public:
    static int  m_nextUniqueId;
    int         m_index;
    int         m_uniqueId;

public:
    SoundInstanceId();

    void SetInvalid();

	bool                    operator == (SoundInstanceId const &w) const;		
    SoundInstanceId const   &operator = (SoundInstanceId const &w);

    static int GenerateUniqueId();
};



// ============================================================================
// class SoundInstance

class SoundInstance
{
public:
    enum                            // Position Types
    {
        Type2D,
        Type3DStationary,
        Type3DAttachedToObject,
        TypeInEditor,
        TypeMusic,
        Type3DRandom,
        NumPositionTypes
    };

    enum                            // Instance types
    {
        Polyphonic,        
        MonophonicRandom,
        MonophonicNearest,
        NumInstanceTypes
    };

    enum                            // Loop Types
    {
        SinglePlay,
        Looped,
        LoopedADSR,
        NumLoopTypes
    };

    enum                            // Source types
    {
        Sample,
        SampleGroupRandom,
        NumSourceTypes
    };

    enum                            // ADSR State types
    {
        StateAttack,
        StateSustain,
        StateRelease
    };
    
    SoundInstanceId     m_id;

    char                m_soundName[256];
    char                m_sampleName[256];
    int                 m_positionType;				
    int                 m_instanceType;
    int                 m_loopType;
    int                 m_sourceType;
    
    int                 m_restartAttempts;          // Used to give single play sounds more than one chance to start
    
    float               m_minDistance;              // Distance (m) at which sound begins to attenuate
    SoundParameter      m_volume;                   // Channel volume, logarithmic, 0.0f (silence) to 10.0f (full)   
    SoundParameter      m_attack;                   // ADSR
    SoundParameter      m_sustain;                  // ADSR
    SoundParameter      m_release;                  // ADSR
    int                 m_adsrState;                // ADSR
    float               m_adsrTimer;                // ADSR
    float               m_channelVolume;            // m_volume after considering ADSR
    float               m_perceivedVolume;          // m_channelVolume after considering 3d location
    
    SoundParameter      m_freq;

    SoundParameter      m_loopDelay;
    float               m_loopDelayTimer;
    bool                m_restartOccured;
    
    Vector3<float>          m_pos;
    Vector3<float>          m_vel;    
    LList<SoundObjectId>    m_objIds;
    SoundObjectId           m_objId;                // The selected objId from the list

    float               m_calculatedPriority;       
    int                 m_channelIndex;   

    SoundSampleHandle	*m_soundSampleHandle;
    SoundInstance       *m_parent;                  // The blueprint from which I was copied

    LList               <DspHandle *> m_dspFX;

    char				*m_eventName;
        
    void    OpenStream  (bool _keepCurrentStream);  // Handles sound groups, file types etc
    
public:
    SoundInstance();
    ~SoundInstance();
    
    void    SetSoundName        ( char const *_name );
    void    SetEventName        ( char const *_objectType, char const *_eventName );

    void    Copy                ( SoundInstance *_copyMe );
    bool    StartPlaying        ( int _channelIndex );
    bool    IsPlaying           ();
    bool    Advance             ();                                     // Only call me if we are actually playing
    bool    AdvanceLoop         ();                                     // true if loop phase is complete and we can write more samples
    void    BeginRelease        ( bool _final );                        // Enters Release phase of sound
    void    StopPlaying         ();                                     // Stops sound immediately (if playing)

    int     GetChannelIndex		();
    char    *GetDescriptor		();
    float   GetTimeRemaining    ();                                     // Time in seconds remaining in the sample (-1 if invalid sample)

    bool    Update3DPosition            ();                             // Will only set the pos if required
    bool    UpdateChannelVolume         ();                             // Takes into account ADSR. Returns true if done
    void    CalculatePerceivedVolume    ();                             // Fills in m_perceivedVolume
    
    void    ForceParameter      ( SoundParameter &_param, float value );        // Forces the instance values eg vel, pos
    bool    UpdateParameter     ( SoundParameter &_param );                     // Returns true if any change occured
    
    void    PropagateBlueprints ( bool _forceRestart );                         // Call this to update all looping sounds
	bool    IsPlayable          ();                                             // Check that everything necessary for playing the sound is available

    SoundObjectId GetAttachedObject();

    static char *GetPositionTypeName    ( int _type );
    static char *GetInstanceTypeName    ( int _type );
    static char *GetLoopTypeName        ( int _type );
    static char *GetSourceTypeName      ( int _type );
};



// ============================================================================
// class DspHandle

#define MAX_PARAMS  13

class DspHandle
{
public:
    int             m_type;                         // enum'd in SoundLibrary3d
    
    SoundParameter  m_params[MAX_PARAMS];
    SoundInstance   *m_parent;

public:
    DspHandle();

    void    Copy        (DspHandle *_copyMe);
    void    Initialise  (SoundInstance *_parent);
    void    Advance     ();
};



#endif
