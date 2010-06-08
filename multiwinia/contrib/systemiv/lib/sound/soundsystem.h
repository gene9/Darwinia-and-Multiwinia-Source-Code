#ifndef _included_soundsystem_h
#define _included_soundsystem_h

#include "lib/tosser/fast_darray.h"
#include "lib/tosser/llist.h"
#include "lib/tosser/hash_table.h"
#include "lib/math/vector3.h"

#include "sound_instance.h"
#include "sound_parameter.h"
#include "soundsystem_interface.h"
#include "sound_blueprint_manager.h"


#ifdef TARGET_MSVC
#define SOUND_USE_DSOUND_FREQUENCY_STUFF                // Set DirectSound buffer frequencies directly
                                                         // If not set, our sound library will do the Frequency adjustments in software
#endif
#if defined(TARGET_OS_MACOSX) || defined(TARGET_OS_LINUX)
#define SOUND_USE_DSOUND_FREQUENCY_STUFF
#endif

class TextReader;
class SoundInstance;
class TextFileWriter;
class SoundSystemInterface;
class SoundEventBlueprint;
class SampleGroup;
class DspBlueprint;
class SoundBlueprintManager;


//*****************************************************************************
// Class SoundSystem
//*****************************************************************************

class SoundSystem
{
public:    
    SoundBlueprintManager   m_blueprints;
    SoundSystemInterface    *m_interface;
    
    float                   m_timeSync;   
    bool                    m_propagateBlueprints;                          // If true, looping sounds will sync with blueprints
    Vector3<float>          m_editorPos;
    SoundInstanceId         m_editorInstanceId;
    
    FastDArray              <SoundInstance *> m_sounds;						// All the sounds that want to play

    SoundInstanceId         *m_channels;
    int                     m_numChannels;
    int                     m_numMusicChannels;
    
protected:    
    int FindBestAvailableChannel( bool _music );
    
    bool IsMusicChannel         ( int _channelId );

    static bool SoundLibraryMainCallback( unsigned int _channel, signed short *_data, 
                                          unsigned int _numSamples, int *_silenceRemaining );

public:
    SoundSystem();
    ~SoundSystem();

	void Initialise				( SoundSystemInterface *_interface );
    void RestartSoundLibrary    ();
    
    void Advance();

    bool InitialiseSound        ( SoundInstance *_instance );                   // Sets up sound, adds to instance list
    void ShutdownSound          ( SoundInstance *_instance );                   // Stops / deletes sound + removes refs

    void TriggerEvent           ( SoundObjectId _id, char *_eventName );
    void TriggerEvent           ( char *_type, char *_eventName );
    void TriggerEvent           ( char *_type, char *_eventName, Vector3<float> const &_pos );

    void StopAllSounds          ( SoundObjectId _id, char *_eventName=NULL );        // Pass in NULL to stop every event. 
                                                                                     // Full event name required, eg "Darwinian SeenThreat"
	void EnableCallback         ( bool _enabled );

    void StopAllDSPEffects      ();

    void TriggerDuplicateSound  ( SoundInstance *_instance );

    
    int  IsSoundPlaying         ( SoundInstanceId _id );
    int  NumInstancesPlaying    ( char *_sampleName );
    int  NumInstancesPlaying    ( SoundObjectId _id, char *_eventName );
    int  NumInstances           ( SoundObjectId _id, char *_eventName );

    int  NumSoundInstances      ();
    int  NumChannelsUsed        ();
    int  NumSoundsDiscarded     ();
    
    void PropagateBlueprints    (bool _forceRestart=false);                     // Call this to update all looping sounds
    void RuntimeVerify          ();												// Verifies that the sound system has screwed it's own datastructures
	bool IsSampleUsed           (char const *_soundName);                       // Looks to see if that sound name is used in any blueprints 

    SoundInstance *GetSoundInstance( SoundInstanceId id );
};



extern SoundSystem *g_soundSystem;

#endif
