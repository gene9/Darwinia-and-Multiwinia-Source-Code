#ifndef _included_soundsystem_h
#define _included_soundsystem_h

#include "lib/tosser/fast_darray.h"
#include "lib/tosser/llist.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/vector3.h"
#include "lib/netlib/net_mutex.h"

#include "sound/sound_instance.h"
#include "sound/sound_parameter.h"

#include "worldobject/worldobject.h"


class Entity;
class Building;
class TextReader;
class SoundInstance;
class Profiler;
class TextFileWriter;
class NetEvent;
class WorkQueue;

//*****************************************************************************
// Class SoundEventBlueprint
//*****************************************************************************

class SoundEventBlueprint
{
public:
    char            *m_eventName;
    SoundInstance   *m_instance;
    
public:
    SoundEventBlueprint();
	~SoundEventBlueprint();

    bool    TeamMatch   ( int _teamId );
    void    SetEventName( char *_name );
};


//*****************************************************************************
// Class SoundSourceBlueprint
//*****************************************************************************

class SoundSourceBlueprint
{
public:
    enum                    // Other types of Sound Source
    {
        TypeLaser,
        TypeGrenade,
        TypeRocket,
        TypeAirstrikeBomb,
        TypeSpirit,
        TypeSepulveda,
        TypeGesture,
        TypeAmbience,
        TypeMusic,
        TypeInterface,
        TypeCratePowerup,
		TypeMultiwiniaInterface,
        NumOtherSoundSources
    };
    
public:
	~SoundSourceBlueprint();
    LList       <SoundEventBlueprint *> m_events;

    static int   GetSoundSoundType ( char const *_name );
    static char *GetSoundSourceName( int _type );
    static void  ListSoundEvents   ( int _type, LList<char *> *_list );
};


//*****************************************************************************
// Class DspParameterBlueprint
//*****************************************************************************

class DspParameterBlueprint
{
public:
    char    m_name[256];
    float   m_min;
    float   m_max;
    float   m_default;
    int     m_dataType;                     // 0 = float, 1 = long
};


//*****************************************************************************
// Class DspBlueprint
//*****************************************************************************

class DspBlueprint
{
public:
	~DspBlueprint();

    char    m_name[256];
    DArray  <DspParameterBlueprint *> m_params;

public:
    char *GetParameter   ( int _param, float *_min=NULL, float *_max=NULL, float *_default=NULL, int *_dataType=NULL );
};


//*****************************************************************************
// Class SampleGroup
//*****************************************************************************

class SampleGroup
{
public:
    char    m_name[256];
    LList   <char *> m_samples;

public:
	~SampleGroup            ();
    void    SetName         ( char *_name );
    void    AddSample       ( char *_sample );
};


//*****************************************************************************
// Class SoundSystem
//*****************************************************************************

class SoundSystem
{
public:    
	enum
	{
		SoundSourceNoError,											// Remember to update g_soundSourceErrors in soundsystem.cpp
		SoundSourceNotMono,
		SoundSourceFileDoesNotExist,
		SoundSourceFilenameHasSpace,
		SoundSourceNumErrors
	};

    float           m_timeSync;
    bool            m_propagateBlueprints;                          // If true, looping sounds will sync with blueprints
    Profiler        *m_mainProfiler;
	Profiler		*m_eventProfiler;
	bool			m_quitWithoutSave;

    Vector3         m_editorPos;
    SoundInstanceId m_editorInstanceId;
    
    FastDArray      <SoundInstance *> m_sounds;						// All the sounds that want to play
	SoundInstance	*m_music;										// There can only be one piece of music at a time
	SoundInstance	*m_requestedMusic;

    SoundInstanceId *m_channels;
    int             m_numChannels;

    DArray      <SoundSourceBlueprint *> m_entityBlueprints;        // Indexed on Entity::m_type
    DArray      <SoundSourceBlueprint *> m_buildingBlueprints;      // Indexed on Building::m_type
    DArray      <SoundSourceBlueprint *> m_otherBlueprints;         // Indexed on SoundSourceBlueprint    
    DArray      <DspBlueprint *> m_filterBlueprints;        // Indexed on SoundLibrary3d::FX types
    
    DArray      <SampleGroup *> m_sampleGroups;
	
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	// The OS X sound system uses a pull model where samples are obtained from a callback
	// on a secondary thread. This requires a lock to avoid conflicts with the main
	// game loop.
	NetMutex	m_mutex;
#endif

private:
    bool            m_isInitialized;
    NetEvent        *m_initializedEvent;
	bool			m_isPreloaded;
	
protected:
    void ParseSoundEvent		( TextReader *_in, SoundSourceBlueprint *_source, char const *_entityName );
    void ParseSoundEffect		( TextReader *_in, SoundEventBlueprint *_blueprint );
    void ParseSampleGroup       ( TextReader *_in, SampleGroup *_group );

    void WriteSoundEvent		( TextFileWriter *_file, SoundEventBlueprint *_event );
    void WriteSampleGroup       ( TextFileWriter *_file, SampleGroup *_group );
    
    void SaveBlueprints			( char const *_filename );

    int FindBestAvailableChannel();
    
    static bool SoundLibraryMainCallback( unsigned int _channel, 
                                          signed short *_data,
										  int _dataCapacity, 
                                          unsigned int _numSamples,
                                          int *_silenceRemaining );
    static bool SoundLibraryMusicCallback(signed short *_data,
										  int _dataCapacity, 
                                          unsigned int _numSamples,
                                          int *_silenceRemaining );

    void RestartSoundLibraryInternal();

public:
    SoundSystem();
    ~SoundSystem();

	void Initialise				();
    void RestartSoundLibrary    ();
    bool IsInitialized          ();

    void WaitIsInitialized      ();
    void TriggerInitialize      ();

    void LoadEffects			();
    void LoadBlueprints			();
    void SaveBlueprints			();
	bool AreBlueprintsModified	();
    
    void Advance();

    bool InitialiseSound        ( SoundInstance *_instance );                   // Sets up sound, adds to instance list
    void ShutdownSound          ( SoundInstance *_instance );                   // Stops / deletes sound + removes refs

    int  IsSoundPlaying         ( SoundInstanceId _id );
    int  NumInstancesPlaying    ( WorldObjectId _id, char *_eventName );
    int  NumInstances           ( WorldObjectId _id, char *_eventName );

    int  NumSoundInstances      ();
    int  NumChannelsUsed        ();
    int  NumSoundsDiscarded     ();

    void TriggerEntityEvent     ( Entity *_entity,      char *_eventName );
    void TriggerBuildingEvent   ( Building *_building,  char *_eventName );
    void TriggerOtherEvent      ( WorldObject *_other,  char *_eventName, int _type );
    void TriggerOtherEvent      ( Vector3 const &pos,   char *_eventName, int _type, int _teamId );

    void StopAllSounds          ( WorldObjectId _id, char *_eventName=NULL );        // Pass in NULL to stop every event. 
                                                                                     // Full event name required, eg "Darwinian SeenThreat"

    void StopAllDSPEffects      ();

    void TriggerDuplicateSound  ( SoundInstance *_instance );

    void PropagateBlueprints    ();                                             // Call this to update all looping sounds
    void RuntimeVerify          ();												// Verifies that the sound system has screwed it's own datastructures
	void LoadtimeVerify			();												// Verifies that the data load from sounds.txt is OK
	char const *IsSoundSourceOK	(char const *_soundName);						// Tests that file names and file formats are OK, returns an error code from the SoundSource enum
	bool IsSampleUsed           (char const *_soundName);                       // Looks to see if that sound name is used in any blueprints 

    SampleGroup *GetSampleGroup     ( const char *_name );
    SampleGroup *NewSampleGroup     ( char *_name=NULL );
    bool        RenameSampleGroup   ( char *_oldName, char *_newName );

    SoundInstance *GetSoundInstance( SoundInstanceId id );

    static bool NotReadyForGameSoundsYet();

	
#ifdef TARGET_OS_MACOSX
public:
	void PreloadSounds();
	WorkQueue* m_PreloadQueue;	// we use the work queue to spawn a thread that loads sounds in the background

private:
	void BackgroundSoundLoad();	
	void LoadSound(char* soundName, bool cache, bool lowcpu);
#endif
};


#endif
