#include "lib/universal_include.h"
#include "lib/debug_utils.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/preferences.h"
#include "lib/resource.h"
#include "lib/string_utils.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/window_manager.h"
#include "lib/language_table.h"
#include "loading_screen.h"

#include "interface/save_on_quit_window.h"
#include "interface/message_dialog.h"

#include "sound/sample_cache.h"
#include "sound/sound_library_3d.h"
#include "sound/soundsystem.h"
#include "sound/sound_stream_decoder.h"
#include "lib/system_info.h"

#if defined(WIN32)
	#include "sound/sound_library_2d_win32.h"
#else
	#include "sound/sound_library_2d_sdl.h"
#endif

#ifdef HAVE_DSOUND
#include "sound/sound_library_3d_dsound.h"
#endif

#ifdef HAVE_XAUDIO
#include "sound/sound_library_3d_xaudio.h"
#endif

#ifdef HAVE_OPENAL
#include "sound/sound_library_3d_openal.h"
#endif

#include "sound/sound_library_3d_software.h"

#include "app.h"
#include "main.h"
#include "camera.h"
#include "location.h"
#include "global_world.h"

#include "worldobject/entity.h"
#include "worldobject/crate.h"

#ifdef TARGET_OS_MACOSX
#include "work_queue.h"
#include <pthread.h>
#include <sys/types.h>
#endif


#define SOUNDSYSTEM_UPDATEPERIOD    0.05f

//#define SOUNDSYSTEM_VERIFY                          // Define this to make the sound system
                                                    // Perform a series of (very slow)
                                                    // tests every frame for correctness


//*****************************************************************************
// Class SoundEventBlueprint
//*****************************************************************************

SoundEventBlueprint::SoundEventBlueprint()
:   m_eventName(NULL),
    m_instance(NULL)
{
}

SoundEventBlueprint::~SoundEventBlueprint()
{
	delete m_instance; 
	m_instance = NULL;

	delete [] m_eventName;
	m_eventName = NULL;
}


void SoundEventBlueprint::SetEventName( char *_name )
{
    if( m_eventName )
    {
        delete [] m_eventName;
        m_eventName = NULL;
    }

    if( _name ) m_eventName = newStr( _name );
}


bool SoundEventBlueprint::TeamMatch( int _teamId )
{
    if( !m_instance ) return false;

    if( _teamId == 255 ) return true;

    switch( m_instance->m_teamMatchType )
    {
        case SoundInstance::AllTeams:
            return true;
    
        case SoundInstance::MyTeamOnly:
            return( g_app->m_globalWorld &&
                    _teamId == g_app->m_globalWorld->m_myTeamId );

        case SoundInstance::EnemyTeamOnly:
            return( g_app->m_globalWorld &&
                    _teamId != g_app->m_globalWorld->m_myTeamId );
    }


    return false;
    
}



//*****************************************************************************
// Class SoundSourceBlueprint
//*****************************************************************************

SoundSourceBlueprint::~SoundSourceBlueprint()
{
	m_events.EmptyAndDelete();
}

int SoundSourceBlueprint::GetSoundSoundType ( char const *_name )
{
    for( int i = 0; i < NumOtherSoundSources; ++i )
    {
        if( stricmp( _name, GetSoundSourceName(i) ) == 0 )
        {
            return i;
        }
    }
    return -1;
}


char *SoundSourceBlueprint::GetSoundSourceName( int _type )
{
    char *names[] = { 
                        "Laser",
                        "Grenade",
                        "Rocket",
                        "AirStrikeBomb",
                        "Spirit",
                        "Sepulveda",
                        "Gesture",
                        "Ambience",
                        "Music",
                        "Interface",
                        "CratePowerup",
						"MultiwiniaInterface"
                    };

    AppDebugAssert( _type >= 0 && _type < NumOtherSoundSources );
    return names[_type];
}


void SoundSourceBlueprint::ListSoundEvents( int _type, LList<char *> *_list )
{
    switch( _type )
    {
        case TypeLaser:
            _list->PutData( "Create" );
            _list->PutData( "Richochet" );
            _list->PutData( "HitGround" );
            _list->PutData( "HitEntity" );
            _list->PutData( "KilledEntity" );
            _list->PutData( "InjuredEntity" );
            _list->PutData( "HitBuilding" );
            break;

        case TypeGrenade:
            _list->PutData( "Create" );
            _list->PutData( "Bounce" );
            _list->PutData( "Flash" );
            _list->PutData( "Explode" );
            _list->PutData( "ExplodeController" );
            break;
            
        case TypeRocket:
            _list->PutData( "Create" );
            _list->PutData( "Explode" );
            break;

        case TypeAirstrikeBomb:
            _list->PutData( "Create" );
            _list->PutData( "Bounce" );
            _list->PutData( "Flash" );
            _list->PutData( "Explode" );
            break;
            
        case TypeSpirit:
            _list->PutData( "Create" );
            _list->PutData( "PickedUp" );
            _list->PutData( "Dropped" );
            _list->PutData( "PlacedIntoEgg" );
            _list->PutData( "EggDestroyed" );
            _list->PutData( "BeginAscent" );
            break;

        case TypeSepulveda:
            _list->PutData( "Appear" );
            _list->PutData( "Disappear" );
            _list->PutData( "TextAppear" );
            break;

        case TypeGesture:
            _list->PutData( "GestureBegin" );
            _list->PutData( "GestureEnd" );
            _list->PutData( "GestureSuccess" );
            _list->PutData( "GestureFail" );
            break;

        case TypeAmbience:
            _list->PutData( "EnterLocation" );
            _list->PutData( "ExitLocation" );
            _list->PutData( "EnterGlobalWorld" );
            break;

        case TypeMusic:
            _list->PutData( "Cutscene1" );
            _list->PutData( "Cutscene2" );
            _list->PutData( "Cutscene3" );
            _list->PutData( "Cutscene4" );
            _list->PutData( "Cutscene5" );
            _list->PutData( "LoaderRaytrace" );
            _list->PutData( "LoaderSoul" );
            _list->PutData( "LoaderFodder" );
            _list->PutData( "LoaderSpeccy" );
            _list->PutData( "LoaderMatrix" );
            _list->PutData( "LoaderGameOfLife" );
            _list->PutData( "StartSequence" );
            _list->PutData( "EndSequence" );
            _list->PutData( "Credits" );
            _list->PutData( "Demo2Intro" );
            _list->PutData( "Demo2Mid" );
            break;

        case TypeInterface:
            _list->PutData( "Show" );
            _list->PutData( "Hide" );
            _list->PutData( "Slide" );
            //_list->PutData( "TaskManagerSelectTask" );
            //_list->PutData( "TaskManagerDeselectTask" );
            _list->PutData( "DeleteTask" );
            _list->PutData( "MouseOverIcon" );     
            _list->PutData( "ShowLogo" );
            break;

        case TypeCratePowerup:
            for( int i = 0; i < Crate::NumCrateRewards; ++i )
            {
                _list->PutData( Crate::GetName(i) );
            }
            break;

		case TypeMultiwiniaInterface:
			_list->PutData( "CountdownTick" );
            _list->PutData( "CountdownZeroTick" );
			_list->PutData( "GameOver" );
			_list->PutData( "YouWon" );
			_list->PutData( "YouLost" );
            _list->PutData( "SelectionCircle" );
            _list->PutData( "SelectUnit" );
            _list->PutData( "ToggleUnitState" );
            _list->PutData( "LobbyAmbience" );
            _list->PutData( "MenuRollOver" );
            _list->PutData( "MenuSelect" );
            _list->PutData( "MenuCancel" );
            _list->PutData( "MouseError" );
            _list->PutData( "ChatMessage" );
			break;
    }
}


//*****************************************************************************
// Class DspBlueprint
//*****************************************************************************

DspBlueprint::~DspBlueprint()
{
	m_params.EmptyAndDelete();
}

char *DspBlueprint::GetParameter( int _param, float *_min, float *_max, float *_default, int *_dataType )
{
    if( m_params.ValidIndex(_param) )
    {
        DspParameterBlueprint *sb = m_params[_param];
        if( _min ) *_min = sb->m_min;
        if( _max ) *_max = sb->m_max;
        if( _default ) *_default = sb->m_default;
        if( _dataType ) *_dataType = sb->m_dataType;
        return sb->m_name;
    }
    return NULL;
}


//*****************************************************************************
// Class SampleGroup
//*****************************************************************************

void SampleGroup::SetName( char *_name )
{
    strcpy( m_name, _name );
}


void SampleGroup::AddSample( char *_sample )
{
    char *sampleCopy = newStr( _sample );
    m_samples.PutData( sampleCopy );
}

SampleGroup::~SampleGroup()
{
	m_samples.EmptyAndDeleteArray();
}

//*****************************************************************************
// Class SoundSystem
//*****************************************************************************

SoundSystem::SoundSystem()
:   m_timeSync(0.0f),
    m_channels(NULL),
    m_eventProfiler(NULL),
    m_mainProfiler(NULL),
	m_quitWithoutSave(false),
    m_propagateBlueprints(false),
    m_numChannels(0),
	m_music(NULL),
	m_requestedMusic(NULL),
	m_isInitialized(false),
	m_isPreloaded(false)
{    
    m_eventProfiler = new Profiler();
    m_mainProfiler = new Profiler();

    m_initializedEvent = new NetEvent();
}


SoundSystem::~SoundSystem()
{
    delete m_eventProfiler;
    delete m_mainProfiler;
    if( m_channels )
        delete [] m_channels;
    
    m_sounds.EmptyAndDelete();
    m_entityBlueprints.EmptyAndDelete();
    m_buildingBlueprints.EmptyAndDelete();
    m_otherBlueprints.EmptyAndDelete();
    m_filterBlueprints.EmptyAndDelete();
    m_sampleGroups.EmptyAndDelete();

    if( g_soundLibrary3d )
        delete g_soundLibrary3d;
    g_soundLibrary3d = NULL;
    if( g_soundLibrary2d )
        delete g_soundLibrary2d;
    g_soundLibrary2d = NULL;

    delete m_initializedEvent;
}


void SoundSystem::Initialise()
{
#ifndef TARGET_OS_MACOSX
    LoadEffects();
    LoadBlueprints();
#endif
	
#ifdef TARGET_OS_MACOSX
	m_PreloadQueue = new WorkQueue();
	m_PreloadQueue->Add( &SoundSystem::BackgroundSoundLoad, this);
#endif
	
    RestartSoundLibraryInternal();
}



void SoundSystem::RestartSoundLibrary()
{
    //
    // Don't restart the Sound Library if it isn't started is the first place (SoundSystem::Initialise need to be done)

#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
    NetLockMutex lock( m_mutex );
#endif
	if( !m_isInitialized )
        return;

    RestartSoundLibraryInternal();
}


void SoundSystem::RestartSoundLibraryInternal()
{
	//
    // Shut down existing sound library

    if( g_soundLibrary3d )
        delete g_soundLibrary3d;
    g_soundLibrary3d = NULL;
    if( g_soundLibrary2d )
        delete g_soundLibrary2d;
    g_soundLibrary2d = NULL;

    //
    // Delete the channels _after_ we shut the sound library down.
    // If we're multithreaded, there are nasty crashes if we delete
    // these before the system shuts down.
    if( m_channels )
        delete [] m_channels;
    m_channels = NULL;
    m_numChannels = 0;

    //
    // Start up a new sound library

	int mixrate     = g_prefsManager->GetInt("SoundMixFreq", 22050);
	int volume      = g_prefsManager->GetInt("SoundMasterVolume", 255);
    m_numChannels   = g_prefsManager->GetInt("SoundChannels", 32);
    int hw3d        = g_prefsManager->GetInt("SoundHW3D", 0);
    const char *libName   = g_prefsManager->GetString("SoundLibrary", "dsound");
	int bufSize		= 20000;

	g_soundLibrary2d = NULL;
	g_soundLibrary3d = NULL;

#if defined( TARGET_MSVC )
	if( g_systemInfo->m_audioInfo.m_preferredDevice == -1 || stricmp(libName, "none") == 0 )
	{
		g_soundLibrary2d = new SoundLibrary2dNoSound;
		g_soundLibrary3d = new SoundLibrary3dNoSound;
	}
	else
	{
		g_soundLibrary2d = new SoundLibrary2dWin32;
		g_soundLibrary3d = NULL;

#ifdef HAVE_DSOUND
		if (stricmp(libName, "dsound") == 0)    
		{
			g_soundLibrary3d = new SoundLibrary3dDirectSound();	
		}
#endif
		if( !g_soundLibrary3d )
		{
			g_soundLibrary3d = new SoundLibrary3dSoftware();
		}
	}
#else
	if (stricmp(libName, "none") == 0)
	{
		g_soundLibrary2d = new SoundLibrary2dNoSound;
		g_soundLibrary3d = new SoundLibrary3dNoSound;
	}
#ifdef HAVE_OPENAL
	else if (stricmp(libName, "openal") == 0)
	{
		g_soundLibrary2d = new SoundLibrary2dNoSound;
		g_soundLibrary3d = new SoundLibrary3dOpenAL();
	}
#endif
	else
	{
		g_soundLibrary2d = new SoundLibrary2dSDL;
		g_soundLibrary3d = new SoundLibrary3dSoftware();
	}
#endif	
	
    g_soundLibrary3d->SetMasterVolume( volume );
    bool success = g_soundLibrary3d->Initialise( mixrate, m_numChannels, hw3d, bufSize, bufSize * 10 );
    if( !success && hw3d )
    {
        AppDebugOut( "Failed to set requested sound settings\n" );
        MessageDialog *dialog = new MessageDialog( "Error", "Failed to set requested sound settings" );
        EclRegisterWindow( dialog );
        dialog->m_x = 100;
        dialog->m_y = 100;

        g_prefsManager->SetInt("SoundHW3D", 0);
        RestartSoundLibraryInternal();
        return;
    }
    else
    {
        AppReleaseAssert(success, "Couldn't start the sound library");
    }

    m_numChannels = g_soundLibrary3d->GetNumMainChannels();

    if( g_systemInfo->m_audioInfo.m_preferredDevice != -1 )
		AppReleaseAssert( m_numChannels > 0, "Too few channels" );

    m_channels = new SoundInstanceId[m_numChannels];    
    
    g_soundLibrary3d->SetMainCallback( &SoundLibraryMainCallback );
    g_soundLibrary3d->SetMusicCallback( &SoundLibraryMusicCallback );

    m_isInitialized = true;
}


bool SoundSystem::IsInitialized()
{
	return m_isInitialized;
}


void SoundSystem::WaitIsInitialized()
{
	while( !m_isInitialized )
	{
		m_initializedEvent->Wait( 1000 );

		g_app->CheckSounds();
	}
}


void SoundSystem::TriggerInitialize()
{
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( m_mutex );
#endif
	m_initializedEvent->Signal();
}


void SoundSystem::StopAllDSPEffects()
{
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( m_mutex );
#endif
    for( int i = 0; i < m_sounds.Size(); ++i )
    {
        if( m_sounds.ValidIndex(i) )
        {
            SoundInstance *instance = m_sounds[i];
            while( instance->m_dspFX.ValidIndex(0) )
            {
                DspHandle *handle = instance->m_dspFX[0];
                instance->m_dspFX.RemoveData(0);
                delete handle;
            }
        }
    }
}


bool SoundSystem::SoundLibraryMainCallback( unsigned int _channel, signed short *_data, int _dataCapacity,
									        unsigned int _numSamples, int *_silenceRemaining )
{
    if( !g_app->m_soundSystem ) return false;

	SoundInstanceId soundId = g_app->m_soundSystem->m_channels[_channel];
    SoundInstance *instance = g_app->m_soundSystem->GetSoundInstance( soundId );

    if( instance && instance->m_cachedSampleHandle )
    {
		g_app->m_soundSystem->m_eventProfiler->StartProfile(instance->m_eventName);
   
		//
        // Fill the space with sample data

		AppDebugAssert(_numSamples <= _dataCapacity);
 		int numSamplesWritten = instance->m_cachedSampleHandle->Read( _data, _numSamples );

        if( numSamplesWritten < _numSamples )
        {
            signed short *loopStart = _data + numSamplesWritten;
            unsigned int numSamplesRemaining = _numSamples - numSamplesWritten;
            
            if( instance->m_loopType == SoundInstance::Looped ||
                instance->m_loopType == SoundInstance::LoopedADSR )
            {
                while( numSamplesRemaining > 0 )
                {
                    bool looped = instance->AdvanceLoop();
                    if( looped )
                    {
						AppDebugAssert((loopStart - _data) + numSamplesRemaining <= _dataCapacity);
                        unsigned int numWritten = instance->m_cachedSampleHandle->Read( loopStart, numSamplesRemaining );
                        loopStart += numWritten;
                        numSamplesRemaining -= numWritten;
                    }
                    else
                    {
                        g_soundLibrary3d->WriteSilence( loopStart, numSamplesRemaining );
                        numSamplesRemaining = 0;
                    }
                }
            }
            else if( instance->m_loopType == SoundInstance::SinglePlay)
            {
                if( numSamplesWritten > 0 )
                {
                    // The sound just came to an end, so write a whole buffers worth of silence
                    g_soundLibrary3d->WriteSilence( loopStart, numSamplesRemaining );
                    *_silenceRemaining = g_soundLibrary3d->GetChannelBufSize(_channel) - numSamplesRemaining;
                }
                else
                {
                    // The sound came to an end and now we are writing silence
                    g_soundLibrary3d->WriteSilence( loopStart, numSamplesRemaining );
                    *_silenceRemaining -= numSamplesRemaining;
                    if( *_silenceRemaining <= 0 )
                    {
                        instance->BeginRelease(false);
                    }
                }
            }
        }

		g_app->m_soundSystem->m_eventProfiler->EndProfile(instance->m_eventName);
		return true;
    }
    else
    {
        g_soundLibrary3d->WriteSilence( _data, _numSamples );
		return false;
    }
	return true;
}


bool SoundSystem::SoundLibraryMusicCallback( signed short *_data, int _dataCapacity, unsigned int _numSamples, int *_silenceRemaining )
{
    if( !g_app->m_soundSystem ) return false;

    SoundInstance *instance = g_app->m_soundSystem->m_music;

    if( instance && instance->m_cachedSampleHandle )
    {
#ifdef PROFILER_ENABLED
		g_app->m_soundSystem->m_eventProfiler->StartProfile(instance->m_eventName);
#endif

   
		//
        // Fill the space with sample data

		AppDebugAssert(_numSamples <= _dataCapacity);
		int numSamplesWritten = instance->m_cachedSampleHandle->Read( _data, _numSamples );

        if( numSamplesWritten < _numSamples )
        {
            signed short *loopStart = _data + numSamplesWritten;
            unsigned int numSamplesRemaining = _numSamples - numSamplesWritten;
            
            if( instance->m_loopType == SoundInstance::Looped ||
                instance->m_loopType == SoundInstance::LoopedADSR )
            {
                while( numSamplesRemaining > 0 )
                {
                    bool looped = instance->AdvanceLoop();
                    if( looped )
                    {
						AppDebugAssert((loopStart - _data) + numSamplesRemaining <= _dataCapacity);
                        unsigned int numWritten = instance->m_cachedSampleHandle->Read( loopStart, numSamplesRemaining );
                        loopStart += numWritten;
                        numSamplesRemaining -= numWritten;
                    }
                    else
                    {
                        g_soundLibrary3d->WriteSilence( loopStart, numSamplesRemaining );
                        numSamplesRemaining = 0;
                    }
                }
            }
            else if( instance->m_loopType == SoundInstance::SinglePlay)
            {
                if( numSamplesWritten > 0 )
                {
                    // The sound just came to an end, so write a whole buffers worth of silence
                    g_soundLibrary3d->WriteSilence( loopStart, numSamplesRemaining );
                    *_silenceRemaining = g_soundLibrary3d->GetChannelBufSize(g_soundLibrary3d->m_musicChannelId) - numSamplesRemaining;
                }
                else
                {
                    // The sound came to an end and now we are writing silence
                    g_soundLibrary3d->WriteSilence( loopStart, numSamplesRemaining );
                    *_silenceRemaining -= numSamplesRemaining;
                    if( *_silenceRemaining <= 0 )
                    {
                        instance->BeginRelease(false);
                    }
                }
            }
        }

		g_app->m_soundSystem->m_eventProfiler->EndProfile(instance->m_eventName);
		return true;
    }
    else
    {
        g_soundLibrary3d->WriteSilence( _data, _numSamples );
		return false;
    }
	return true;
}


void SoundSystem::LoadEffects()
{
    m_filterBlueprints.SetSize( SoundLibrary3d::NUM_FILTERS );
    
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( m_mutex );
#endif
    TextReader *in = g_app->m_resource->GetTextReader( "effects.txt" );
	AppReleaseAssert(in && in->IsOpen(), "Couldn't load effects.txt");

    while( in->ReadLine() )
    {
        if( !in->TokenAvailable() ) continue;
        char *effect = in->GetNextToken();
        AppDebugAssert( stricmp( effect, "EFFECT" ) == 0 );

        DspBlueprint *bp = new DspBlueprint();
        m_filterBlueprints.PutData( bp );
        strcpy( bp->m_name, in->GetNextToken() );

        in->ReadLine();
        char *param = in->GetNextToken();
        while( stricmp( param, "END" ) != 0 )
        {
            DspParameterBlueprint *sb = new DspParameterBlueprint;
            bp->m_params.PutData( sb );

            strcpy( sb->m_name,     param );
            sb->m_min       = atof( in->GetNextToken() );
            sb->m_max       = atof( in->GetNextToken() );
            sb->m_default   = atof( in->GetNextToken() );
            sb->m_dataType  = atoi( in->GetNextToken() );
            
            in->ReadLine();
            param = in->GetNextToken();
        }
    }

	delete in;
}


bool SoundSystem::AreBlueprintsModified()
{
    // Currently broken due to mod support
    // ie sounds_new.txt and sounds.txt coule be anywhere,
    // you just don't know anymore :)
    return false;

#ifdef SOUND_EDITOR
	SaveBlueprints("sounds_new.txt");
	bool result = AreFilesIdentical("data/sounds.txt", "data/sounds_new.txt");
	if (result == false)
	{
		SaveOnQuitWindow *saveWin = new SaveOnQuitWindow("editor_savesettings");
		EclRegisterWindow(saveWin);
	}
	return !result;
#else
	return false;
#endif
}

void SoundSystem::LoadBlueprints()
{
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( m_mutex );
#endif
    m_entityBlueprints.SetSize( Entity::NumEntityTypes );
    m_buildingBlueprints.SetSize( Building::NumBuildingTypes );
    m_otherBlueprints.SetSize( SoundSourceBlueprint::NumOtherSoundSources );
    
    TextReader *in = g_app->m_resource->GetTextReader( "sounds.txt" );
	AppReleaseAssert(in && in->IsOpen(), "Couldn't open sounds.txt");

	char objectName[128];

    while( in->ReadLine() )
    {
        if( !in->TokenAvailable() ) continue;
        char *group = in->GetNextToken();
        char *type = in->GetNextToken();
        bool event = false;
        SoundSourceBlueprint *ssb = NULL;
        
        if( stricmp( group, "ENTITY" ) == 0 )
        {
			strncpy(objectName, type, 127);
            int entityType = Entity::GetTypeId( type );
            AppDebugAssert( entityType >= 0 && entityType < Entity::NumEntityTypes );
            AppDebugAssert( !m_entityBlueprints.ValidIndex( entityType ) );
            
            ssb = new SoundSourceBlueprint();
            m_entityBlueprints.PutData( ssb, entityType );
            event = true;
        }
        else if( stricmp( group, "BUILDING" ) == 0 )
        {
			strncpy(objectName, type, 127);
            int buildingType = Building::GetTypeId( type );
            AppDebugAssert( buildingType >= 0 && buildingType < Building::NumBuildingTypes );
            AppDebugAssert( !m_buildingBlueprints.ValidIndex( buildingType ) );

            ssb = new SoundSourceBlueprint();
            m_buildingBlueprints.PutData( ssb, buildingType );
            event = true;
        }
        else if( stricmp( group, "OTHER" ) == 0 )
        {
			strncpy(objectName, type, 127);
            int otherType = SoundSourceBlueprint::GetSoundSoundType( type );
            AppDebugAssert( otherType >= 0 && otherType < SoundSourceBlueprint::NumOtherSoundSources );
            AppDebugAssert( !m_otherBlueprints.ValidIndex( otherType ) );

            ssb = new SoundSourceBlueprint();
            m_otherBlueprints.PutData( ssb, otherType );
            event = true;
        }
        else if( stricmp( group, "SAMPLEGROUP" ) == 0 )
        {
			strncpy(objectName, "sample group", 127);
            SampleGroup *sampleGroup = NewSampleGroup( type );
            ParseSampleGroup( in, sampleGroup );
        }

        if( event )
        {
            while( in->ReadLine() )
            {
                if( in->TokenAvailable() )
                {
                    char *word = in->GetNextToken();
                    if( stricmp( word, "END" ) == 0 ) break;
                    AppDebugAssert( stricmp( word, "EVENT"  ) == 0 );
                    ParseSoundEvent( in, ssb, objectName );
					
                }
            }            
        }
    }
        

	// 
	// Verify the data we just loaded - make sure all the samples exist, are
	// in the right format etc.

	//LoadtimeVerify();


    //
    // Fill in the non-specified sound sources with blanks

    for( int i = 0; i < Entity::NumEntityTypes; ++i )
    {
        if( !m_entityBlueprints.ValidIndex(i) )
        {
            SoundSourceBlueprint *ssb = new SoundSourceBlueprint();
            m_entityBlueprints.PutData( ssb, i );
        }
    }

    for( int i = 0; i < Building::NumBuildingTypes; ++i )
    {
        if( !m_buildingBlueprints.ValidIndex(i) )
        {
            SoundSourceBlueprint *ssb = new SoundSourceBlueprint();
            m_buildingBlueprints.PutData( ssb, i );
        }
    }

    for( int i = 0; i < SoundSourceBlueprint::NumOtherSoundSources; ++i )
    {
        if( !m_otherBlueprints.ValidIndex(i) )
        {
            SoundSourceBlueprint *ssb = new SoundSourceBlueprint();
            m_otherBlueprints.PutData( ssb, i );
        }
    }

	// load 'em up
	
	
	delete in;
}


void SoundSystem::SaveBlueprints()
{
	SaveBlueprints("sounds.txt");
}


void SoundSystem::SaveBlueprints(char const *_filename)
{
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( m_mutex );
#endif
    TextFileWriter *file = g_app->m_resource->GetTextFileWriter( _filename, false );
    // Not always possible - this may be a release version with a data.dat
    if( !file ) return;
     
    for( int i = 0; i < m_entityBlueprints.Size(); ++i )
    {
        AppDebugAssert( m_entityBlueprints.ValidIndex(i) );
        SoundSourceBlueprint *ssb = m_entityBlueprints[i];
        
        if( ssb->m_events.Size() > 0 )
        {
            file->printf( "# =========================================================\n" );                        
            file->printf( "ENTITY %s\n", Entity::GetTypeName(i) );
        
            for( int j = 0; j < ssb->m_events.Size(); ++j )
            {
                SoundEventBlueprint *seb = ssb->m_events[j];
                WriteSoundEvent( file, seb );
            }

            file->printf( "END\n\n\n" );
        }
    }

    for( int i = 0; i < m_buildingBlueprints.Size(); ++i )
    {
        AppDebugAssert( m_buildingBlueprints.ValidIndex(i) );
        SoundSourceBlueprint *ssb = m_buildingBlueprints[i];
        
        if( ssb->m_events.Size() > 0 )
        {
            file->printf( "# =========================================================\n" );
            file->printf( "BUILDING %s\n", Building::GetTypeName(i) );
        
            for( int j = 0; j < ssb->m_events.Size(); ++j )
            {
                SoundEventBlueprint *seb = ssb->m_events[j];
                WriteSoundEvent( file, seb );
            }

            file->printf( "END\n\n\n" );
        }
    }

    for( int i = 0; i < m_otherBlueprints.Size(); ++i )
    {
        AppDebugAssert( m_otherBlueprints.ValidIndex(i) );
        SoundSourceBlueprint *ssb = m_otherBlueprints[i];
        
        if( ssb->m_events.Size() > 0 )
        {
            file->printf( "# =========================================================\n" );
            file->printf( "OTHER %s\n", SoundSourceBlueprint::GetSoundSourceName(i) );
        
            for( int j = 0; j < ssb->m_events.Size(); ++j )
            {
                SoundEventBlueprint *seb = ssb->m_events[j];
                WriteSoundEvent( file, seb );
            }

            file->printf( "END\n\n\n" );
        }
    }

    for( int i = 0; i < m_sampleGroups.Size(); ++i )
    {
        if( m_sampleGroups.ValidIndex(i) )
        {
            SampleGroup *group = m_sampleGroups[i];

            file->printf( "# =========================================================\n" );
            file->printf( "SAMPLEGROUP %s\n", group->m_name );
            
            WriteSampleGroup( file, group );

            file->printf( "END\n\n\n" );
            
        }
    }

    delete file;
}


void SoundSystem::ParseSoundEvent( TextReader *_in, SoundSourceBlueprint *_source, char const *_entityName )
{
	AppDebugOut("BEGIN ParseSoundEvent: %s\n", _entityName);
    SoundEventBlueprint *seb = new SoundEventBlueprint();
    seb->SetEventName( _in->GetNextToken() );
    
    seb->m_instance = new SoundInstance();
	seb->m_instance->SetEventName(_entityName, seb->m_eventName);

    int oldUserPriority;            // backwards compatability, no longer used
    
    _in->ReadLine();
    char *fieldName = _in->GetNextToken();
    while( stricmp( fieldName, "END" ) != 0 )
    {
        if ( stricmp( fieldName, "SOUNDNAME" ) == 0 )      
		{
			char *soundName = _in->GetNextToken();
			StrToLower(soundName);
            const char *extensionRemoved = RemoveExtension( soundName );
			seb->m_instance->SetSoundName( extensionRemoved );
		}
        else if ( stricmp( fieldName, "SOURCETYPE" ) == 0 )     seb->m_instance->m_sourceType       = atoi( _in->GetNextToken() );
        else if ( stricmp( fieldName, "POSITIONTYPE" ) == 0 )   seb->m_instance->m_positionType     = atoi( _in->GetNextToken() );
        else if ( stricmp( fieldName, "INSTANCETYPE" ) == 0 )   seb->m_instance->m_instanceType     = atoi( _in->GetNextToken() );
        else if ( stricmp( fieldName, "LOOPTYPE" ) == 0 )       seb->m_instance->m_loopType         = atoi( _in->GetNextToken() );
        else if ( stricmp( fieldName, "TEAMMATCHTYPE" ) == 0 )  seb->m_instance->m_teamMatchType    = atoi( _in->GetNextToken() );
        else if ( stricmp( fieldName, "PRIORITY" ) == 0 )       oldUserPriority                     = atoi( _in->GetNextToken() );
        else if ( stricmp( fieldName, "MINDISTANCE" ) == 0 )    seb->m_instance->m_minDistance      = atof( _in->GetNextToken() );
        else if ( stricmp( fieldName, "VOLUME" ) == 0 )         seb->m_instance->m_volume.Read( _in );
        else if ( stricmp( fieldName, "FREQUENCY" ) == 0 )	    seb->m_instance->m_freq.Read( _in );
        else if ( stricmp( fieldName, "ATTACK" ) == 0 )         seb->m_instance->m_attack.Read( _in );
        else if ( stricmp( fieldName, "SUSTAIN" ) == 0 )        seb->m_instance->m_sustain.Read( _in );
        else if ( stricmp( fieldName, "RELEASE" ) == 0 )        seb->m_instance->m_release.Read( _in );
        else if ( stricmp( fieldName, "LOOPDELAY" ) == 0 )      seb->m_instance->m_loopDelay.Read( _in );
        else if ( stricmp( fieldName, "EFFECT" ) == 0 )         ParseSoundEffect( _in, seb );
        else												    AppDebugAssert( false );
        
        // This is bad, we have a looping sound that won't be attached
        // to any one object
//        AppDebugAssert( !( seb->m_instance->m_loopType &&
//                        seb->m_instance->m_positionType != SoundInstance::Type3DAttachedToObject ) );

        _in->ReadLine();
        fieldName = _in->GetNextToken();
    }
        
    _source->m_events.PutData( seb );
	AppDebugOut("END ParseSoundEvent: %s\n", _entityName);

}


void SoundSystem::ParseSoundEffect( TextReader *_in, SoundEventBlueprint *_blueprint )
{
    char *effectName = _in->GetNextToken();
    int fxType = -1;
	AppDebugOut("BEGIN ParseSoundEffect: %s\n", effectName);

	
    for( int i = 0; i < m_filterBlueprints.Size(); ++i )
    {
        DspBlueprint *seb = m_filterBlueprints[i];
        if( stricmp( seb->m_name, effectName ) == 0 )
        {
            fxType = i;
            break;
        }
    }
    
    AppDebugAssert( fxType != -1 );
    
    DspHandle *effect = new DspHandle();
    effect->m_type = fxType;

    int paramIndex = 0;
    while( true )
    {
        _in->ReadLine();
        char *paramName = _in->GetNextToken();
        if( stricmp( paramName, "END" ) == 0 ) break;        
        effect->m_params[paramIndex].Read( _in );
        ++paramIndex;        
    }
    _blueprint->m_instance->m_dspFX.PutData( effect );
	AppDebugOut("END ParseSoundEffect: %s\n", effectName);
}


void SoundSystem::ParseSampleGroup( TextReader *_in, SampleGroup *_group )
{
	AppDebugOut("BEGIN ParseSampleGroup: %s\n", _group->m_name);
    while( true )
    {
        _in->ReadLine();
        char *paramType = _in->GetNextToken();
        if( stricmp( paramType, "END" ) == 0 )
        {
            break;
        }

        char *sample = _in->GetNextToken();
		StrToLower(sample);
        char *extensionRemoved = (char *) RemoveExtension( sample );
		_group->AddSample( extensionRemoved );
		AppDebugOut("AddSample: %s\n", extensionRemoved);
    }
	AppDebugOut("END ParseSampleGroup: %s\n", _group->m_name);

}


void SoundSystem::WriteSoundEvent( TextFileWriter *_file, SoundEventBlueprint *_event )
{
    AppDebugAssert( _event );
    AppDebugAssert( _event->m_instance );

    _file->printf( "\tEVENT %-20s\n"
                    "\t\tSOUNDNAME          %s\n"
                    "\t\tSOURCETYPE         %d\n"
                    "\t\tPOSITIONTYPE       %d\n"
                    "\t\tINSTANCETYPE       %d\n"
                    "\t\tLOOPTYPE           %d\n"
                    "\t\tTEAMMATCHTYPE      %d\n"
                    "\t\tMINDISTANCE        %2.2f\n",
                                        _event->m_eventName,
                                        _event->m_instance->m_soundName,
                                        _event->m_instance->m_sourceType,
                                        _event->m_instance->m_positionType,
                                        _event->m_instance->m_instanceType,
                                        _event->m_instance->m_loopType,
                                        _event->m_instance->m_teamMatchType,
                                        _event->m_instance->m_minDistance
                                                                            );

    _event->m_instance->m_volume.Write      ( _file, "VOLUME", 2 );
    
    if( !_event->m_instance->m_freq.IsFixedValue( 1.0f ) )      _event->m_instance->m_freq.Write        ( _file, "FREQUENCY", 2 );
    if( !_event->m_instance->m_attack.IsFixedValue( 0.0f ) )    _event->m_instance->m_attack.Write      ( _file, "ATTACK", 2 );
    if( !_event->m_instance->m_sustain.IsFixedValue( 0.0f ) )   _event->m_instance->m_sustain.Write     ( _file, "SUSTAIN", 2 );
    if( !_event->m_instance->m_release.IsFixedValue( 0.0f ) )   _event->m_instance->m_release.Write     ( _file, "RELEASE", 2 );
    if( !_event->m_instance->m_loopDelay.IsFixedValue( 0.0f ) ) _event->m_instance->m_loopDelay.Write   ( _file, "LOOPDELAY", 2 );

    for( int i = 0; i < _event->m_instance->m_dspFX.Size(); ++i )
    {
        DspHandle *effect = _event->m_instance->m_dspFX[i];
        DspBlueprint *blueprint = m_filterBlueprints[ effect->m_type ];
        
        _file->printf( "\t\tEFFECT             %s\n", blueprint->m_name );
        int paramIndex = 0;
        while( true )
        {
            char *paramName = blueprint->GetParameter( paramIndex );            
            if( !paramName ) break;
            SoundParameter *param = &effect->m_params[ paramIndex ];
            param->Write( _file, paramName, 3 );
            ++paramIndex;
        }
        _file->printf( "\t\tEND\n" );
    }

    _file->printf( "\tEND" );
    _file->printf( "\n" );
}


void SoundSystem::WriteSampleGroup( TextFileWriter *_file, SampleGroup *_group )
{
    for( int i = 0; i < _group->m_samples.Size(); ++i )
    {
        char *sample = _group->m_samples[i];
        _file->printf( "\tSAMPLE  %s\n", sample );
    }
}


bool SoundSystem::InitialiseSound( SoundInstance *_instance )
{
    bool createNewSound = true;

    if( _instance->m_instanceType != SoundInstance::Polyphonic )
    {
        // This is a monophonic sound, so look for an exisiting
        // instance of the same sound
        for( int i = 0; i < m_sounds.Size(); ++i )
        {
            if( m_sounds.ValidIndex(i) )
            {
                SoundInstance *thisInstance = m_sounds[i];
                if( thisInstance->m_instanceType != SoundInstance::Polyphonic &&
                    stricmp( thisInstance->m_eventName, _instance->m_eventName ) == 0 )
                {
                    for( int j = 0; j < _instance->m_objIds.Size(); ++j )
                    {
                        WorldObjectId *id = _instance->m_objIds[j];
                        thisInstance->m_objIds.PutData( new WorldObjectId(*id) );
                    }
                    createNewSound = false;
                    break;
                }
            }
        }
    }   

    if( createNewSound )
    {
        _instance->m_id.m_index = m_sounds.PutData( _instance );
        _instance->m_id.m_uniqueId = SoundInstanceId::GenerateUniqueId();
        _instance->m_restartAttempts = 3;           //int( 1.0f + (float) _instance->m_volume.GetOutput() / 5.0f );
        return true;
    }
    else
    {
        return false;
    }
}


int SoundSystem::FindBestAvailableChannel()
{
    int emptyChannelIndex = -1;                                         // Channel without a current sound or requested sound

    int lowestPriorityChannelIndex = -1;                                // Channel with very low priority
    float lowestPriorityChannel = 999999.9f;

    for( int i = 0; i < m_numChannels; ++i )
    {
        SoundInstanceId soundId = m_channels[i];
        SoundInstance *currentSound = GetSoundInstance( soundId );

        float channelPriority = 0.0f;
        if( currentSound ) 
        {
            channelPriority = currentSound->m_calculatedPriority;
        }
        else
        {
            emptyChannelIndex = i;
            break;
        }

        if( channelPriority < lowestPriorityChannel )
        {
            lowestPriorityChannel = channelPriority;
            lowestPriorityChannelIndex = i;
        }
    }
    

    //
    // Did we find any empty channels?

    if( emptyChannelIndex != -1 )
    {
        return emptyChannelIndex;
    }
    else
    {
        return lowestPriorityChannelIndex;
    }
	return 0;
}


void SoundSystem::ShutdownSound( SoundInstance *_instance )
{
    if( m_sounds.ValidIndex( _instance->m_id.m_index ) )
    {
        m_sounds.RemoveData( _instance->m_id.m_index );
    }


    _instance->StopPlaying();
    delete _instance;
}


SoundInstance *SoundSystem::GetSoundInstance( SoundInstanceId id )
{
    if( !m_sounds.ValidIndex(id.m_index) ) 
    {
        return NULL;
    }

    SoundInstance *found = m_sounds[id.m_index];
    if( found->m_id == id )
    {
        return found;
    }
    return NULL;
}


bool SoundSystem::NotReadyForGameSoundsYet()
{
	if( !g_app->m_soundsLoaded || g_loadingScreen->IsRendering() ||
        NetGetCurrentThreadId() != g_app->m_mainThreadId ) 
	{
		return true;
	}

#ifdef TESTBED_ENABLED
	// Disable sound for testbed
	if( !g_app->GetTestBedRendering() )
		return true;
#endif

	return false;
}


void SoundSystem::TriggerEntityEvent( Entity *_entity, char *_eventName )
{
    if( !m_channels ) return;
	if( NotReadyForGameSoundsYet() ) return;

#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( m_mutex );
#endif
	m_mainProfiler->StartProfile( "TriggerEntityEvent");
    WorldObjectId objId( _entity->m_id );

    if( m_entityBlueprints.ValidIndex( _entity->m_type ) )
    {   
        SoundSourceBlueprint *sourceBlueprint = m_entityBlueprints[ _entity->m_type ];
        for( int i = 0; i < sourceBlueprint->m_events.Size(); ++i )
        {
            SoundEventBlueprint *seb = sourceBlueprint->m_events[i];
            if( stricmp( seb->m_eventName, _eventName ) == 0 &&
                seb->TeamMatch( _entity->m_id.GetTeamId() ) )
            {
                AppDebugAssert( seb->m_instance );
                SoundInstance *instance = new SoundInstance();
                instance->Copy( seb->m_instance );
                instance->m_objIds.PutData( new WorldObjectId(objId) );
                instance->m_pos = _entity->m_pos;
                instance->m_vel = _entity->m_vel;
                bool success = InitialiseSound  ( instance );                
                if( !success ) ShutdownSound    ( instance );                
            }
        }
    }

	m_mainProfiler->EndProfile("TriggerEntityEvent");
}

#ifdef TARGET_OS_MACOSX

void SoundSystem::LoadSound(char* soundName, bool cache, bool lowcpu)
{
#ifdef VERBOSE_DEBUG
	AppDebugOut("checking sound: %s\n", soundName);
#endif
	if(soundName && g_app->m_resource->SoundExists(soundName))
	{
#ifdef VERBOSE_DEBUG
		AppDebugOut("PreloadingSound: %s\n", soundName);
#endif
		CachedSampleHandle* sampleHandle = g_cachedSampleManager.GetSample(soundName);
		// if there is a sample and we're supposed to cache it
		// and it hasn't already been loaded or started loading.
		if(sampleHandle && cache && (sampleHandle->m_cachedSample->LoadState() == kSampleLoadState_NotLoaded))
			sampleHandle->m_cachedSample->Preload(lowcpu);
	}
}

void SoundSystem::PreloadSounds(void)
{	
	AppDebugOut("Preloading Sounds");
#ifdef VERBOSE_DEBUG
	AppDebugOut("Preloading entity sounds"); 
#endif
	double loadAudioStart = GetHighResTime();
	
	double typeLoadStart = GetHighResTime();
	for(int j = 0; j < m_entityBlueprints.Size(); j++)
	{
		if( m_entityBlueprints.ValidIndex(j) )
		{
#ifdef VERBOSE_DEBUG
			AppDebugOut("Preloading entity: %s\n", Entity::GetTypeName(j));
#endif
			SoundSourceBlueprint *sourceBlueprint = m_entityBlueprints[j];
			for( int i = 0; i < sourceBlueprint->m_events.Size(); ++i )
			{
				SoundEventBlueprint *seb = sourceBlueprint->m_events[i];
				LoadSound(seb->m_instance->m_soundName, true, false);
			}
		}
		if(g_app->m_requestQuit)
			return;
	}
	AppDebugOut("entity load time: %f\n", GetHighResTime() - typeLoadStart);
	typeLoadStart = GetHighResTime();

	for(int j = 0; j < m_buildingBlueprints.Size(); j++)
	{
		if( m_buildingBlueprints.ValidIndex(j) )
		{  
#ifdef VERBOSE_DEBUG
			AppDebugOut("Preloading building: %s\n", Building::GetTypeName(j));
#endif
			SoundSourceBlueprint *sourceBlueprint = m_buildingBlueprints[j];
			for( int i = 0; i < sourceBlueprint->m_events.Size(); ++i )
			{
				SoundEventBlueprint *seb = sourceBlueprint->m_events[i];
				LoadSound(seb->m_instance->m_soundName, true, false);
			}			
		}
		if(g_app->m_requestQuit)
			return;
	}
#ifdef VERBOSE_DEBUG
	AppDebugOut("building load time: %f\n", GetHighResTime() - typeLoadStart);
	typeLoadStart = GetHighResTime();
	
	AppDebugOut("Preloading other sounds"); 
#endif
	for(int j = 0; j < m_otherBlueprints.Size(); j++)
	{
		if( m_otherBlueprints.ValidIndex(j) )
		{   
			// we don't preload music
			if((j != SoundSourceBlueprint::TypeMusic) &&
			   (j != SoundSourceBlueprint::TypeGesture) && 
			   (j != SoundSourceBlueprint::TypeSepulveda) && 
			   (j != SoundSourceBlueprint::TypeInterface) && 
			   (j != SoundSourceBlueprint::TypeMultiwiniaInterface))
			{
				//AppDebugOut("Preloading other: %s\n", SoundSourceBlueprint::GetSoundSourceName(j));
				SoundSourceBlueprint *sourceBlueprint = m_otherBlueprints[j];
				
				for( int i = 0; i < sourceBlueprint->m_events.Size(); ++i )
				{
					SoundEventBlueprint *seb = sourceBlueprint->m_events[i];
					LoadSound(seb->m_instance->m_soundName, true, false);
				}
			}
		}
		if(g_app->m_requestQuit)
			return;
	}
#ifdef VERBOSE_DEBUG
	AppDebugOut("other load time: %f\n", GetHighResTime() - typeLoadStart);
	typeLoadStart = GetHighResTime();
	
	AppDebugOut("Preloading sample groups"); 
#endif
	int size = m_sampleGroups.Size();
	for(int j = 0; j < size; j++)
	{
		if( m_sampleGroups.ValidIndex(j) )
		{   
			SampleGroup *sampleGroup = m_sampleGroups[j];
			
			for( int i = 0; i < 1; ++i )
			{
				char *soundName = sampleGroup->m_samples[i];
				// we only cache the first sound in each sample group
				LoadSound(soundName, (i == 0), false);
			}
		}
		if(g_app->m_requestQuit)
			return;
	}
#ifdef VERBOSE_DEBUG
	AppDebugOut("samples load time: %f\n", GetHighResTime() - typeLoadStart);
#endif
	AppDebugOut("Preloaded Audio: %f\n", GetHighResTime() - loadAudioStart);
	m_isPreloaded = true;
}

void SoundSystem::BackgroundSoundLoad(void)
{
	// we drop our thread priority down pretty low so we don't impact the rest of the game too much...
	struct sched_param sp;
	int policy = 0;
	
    memset(&sp, 0, sizeof(struct sched_param));
   
	if (pthread_getschedparam(pthread_self(), &policy, &sp) != 0)
	{
		AppDebugOut("Failed to lower background sound loading thread's priority.\n");	
	}
    else 
	{
		sp.sched_priority = 0;	// we set this thread to the lowest priority possible
		if (pthread_setschedparam(pthread_self(), SCHED_RR, &sp)  != 0) 
		{
			AppDebugOut("Failed to lower background sound loading thread's priority.\n");
		}
	}
	
	
	// for now we do nothing
	AppDebugOut("BackgroundSoundLoad\n");
	AppDebugOut("Preloading sample groups"); 
	double loadAudioStart = GetHighResTime();
	int size = m_sampleGroups.Size();
	for(int j = 0; j < size; j++)
	{
		if( m_sampleGroups.ValidIndex(j) )
		{   
			SampleGroup *sampleGroup = m_sampleGroups[j];

			// we start at 1, because we've already loaded the first sound on the main thread
			for( int i = 1; i < sampleGroup->m_samples.Size(); ++i )
			{
				char *soundName = sampleGroup->m_samples[i];
				// we only cache the first sound in each sample group
				LoadSound(soundName, true, true);
				if(g_app->m_requestQuit)
					return;
			}
		}
		if(g_app->m_requestQuit)
			return;
	}
	double loadAudioEnd = GetHighResTime();
	AppDebugOut("Preloaded Audio: %f\n", loadAudioEnd - loadAudioStart);
}

#endif

void SoundSystem::TriggerBuildingEvent( Building *_building, char *_eventName )
{
    if( !m_channels ) return;
	if( NotReadyForGameSoundsYet() ) return;
	
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( m_mutex );
#endif
 	m_mainProfiler->StartProfile( "TriggerBuildingEvent");
    if( m_buildingBlueprints.ValidIndex( _building->m_type ) )
    {   
        SoundSourceBlueprint *sourceBlueprint = m_buildingBlueprints[ _building->m_type ];
        for( int i = 0; i < sourceBlueprint->m_events.Size(); ++i )
        {
            SoundEventBlueprint *seb = sourceBlueprint->m_events[i];
            if( stricmp( seb->m_eventName, _eventName ) == 0 &&
                seb->TeamMatch( _building->m_id.GetTeamId() ) )
            {
                AppDebugAssert( seb->m_instance );
                SoundInstance *instance = new SoundInstance();
                instance->Copy( seb->m_instance );
                instance->m_objIds.PutData( new WorldObjectId(_building->m_id) );
                instance->m_pos = _building->m_pos;
                bool success = InitialiseSound  ( instance );
                if( !success ) ShutdownSound    ( instance );                
            }
        }
    }

	m_mainProfiler->EndProfile( "TriggerBuildingEvent");
}


void SoundSystem::TriggerOtherEvent( WorldObject *_other, char *_eventName, int _type )
{
    if( !m_channels ) return;
	if( NotReadyForGameSoundsYet() ) return;
	
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( m_mutex );
#endif
	m_mainProfiler->StartProfile( "TriggerOtherEvent");

	static int musicType = -1;
	if (musicType == -1)
	{
		musicType = SoundSourceBlueprint::GetSoundSoundType("music");
	}

    if( m_otherBlueprints.ValidIndex( _type ) )
    {   
		// Search for the event blueprint matching _eventName
        SoundSourceBlueprint *sourceBlueprint = m_otherBlueprints[ _type ];
        for( int i = 0; i < sourceBlueprint->m_events.Size(); ++i )
        {
            SoundEventBlueprint *seb = sourceBlueprint->m_events[i];
            if( stricmp( seb->m_eventName, _eventName ) == 0 &&
                seb->TeamMatch( _other ? _other->m_id.GetTeamId() : 255 ) )
            {
				// We have a match
                AppDebugAssert( seb->m_instance );
                SoundInstance *instance = new SoundInstance();
                instance->Copy( seb->m_instance );                
                if( _type == musicType )
				{
                    //if( m_music && stricmp( m_music->m_eventName+6, _eventName ) == 0 )
                    if( m_music && stricmp( m_music->m_soundName, seb->m_instance->m_soundName ) == 0 )
                    {
                        // The music is already playing
                    }
                    else
                    {
                        m_requestedMusic = instance;
					    if (m_music)
					    {
						    m_music->BeginRelease(true);
					    }
                    }
				}
				else
				{
					if( _other )
					{
						instance->m_pos = _other->m_pos;
						instance->m_objIds.PutData( new WorldObjectId( _other->m_id ) );
					}
					bool success = InitialiseSound  ( instance );
					if( !success ) ShutdownSound    ( instance );                
				}
            }
        }
    }

	m_mainProfiler->EndProfile( "TriggerOtherEvent");
}



void SoundSystem::TriggerOtherEvent( Vector3 const &pos, char *_eventName, int _type, int _teamId )
{
    if( !m_channels ) return;
	if( NotReadyForGameSoundsYet() ) return;
	
    m_mainProfiler->StartProfile( "TriggerOtherEvent");

    if( m_otherBlueprints.ValidIndex( _type ) )
    {   
        // Search for the event blueprint matching _eventName
        SoundSourceBlueprint *sourceBlueprint = m_otherBlueprints[ _type ];
        for( int i = 0; i < sourceBlueprint->m_events.Size(); ++i )
        {
            SoundEventBlueprint *seb = sourceBlueprint->m_events[i];
            if( stricmp( seb->m_eventName, _eventName ) == 0 &&
                seb->TeamMatch( _teamId ) )
            {
                // We have a match
                AppDebugAssert( seb->m_instance );
                SoundInstance *instance = new SoundInstance();
                instance->Copy( seb->m_instance );                
                instance->m_pos = pos;

                bool success = InitialiseSound  ( instance );
                if( !success ) ShutdownSound    ( instance );                
            }
        }
    }

    m_mainProfiler->EndProfile("TriggerOtherEvent");
}


void SoundSystem::TriggerDuplicateSound ( SoundInstance *_instance )
{
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( m_mutex );
#endif
    SoundInstance *newInstance = new SoundInstance();
    newInstance->Copy( _instance );
    newInstance->m_parent = _instance->m_parent;
    newInstance->m_pos = _instance->m_pos;
    newInstance->m_vel = _instance->m_vel;

    for( int i = 0; i < _instance->m_objIds.Size(); ++i )
    {
        WorldObjectId *id = _instance->m_objIds[i];
        newInstance->m_objIds.PutData( new WorldObjectId(*id) );
    }

    bool success = InitialiseSound( newInstance );
    if( success && newInstance->m_positionType == SoundInstance::TypeInEditor )
    {
        m_editorInstanceId = newInstance->m_id;        
    }
    else if( !success )
    {
        ShutdownSound( newInstance);
    }
}


void SoundSystem::StopAllSounds( WorldObjectId _id, char *_eventName )
{
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( m_mutex );
#endif
    if( _eventName && strstr( _eventName, "Music" ) )
    {
        if( m_music ) m_music->BeginRelease(true);        
    }
    else
    {
        for( int i = 0; i < m_sounds.Size(); ++i )
        {
            if( m_sounds.ValidIndex(i) )
            {
                SoundInstance *instance = m_sounds[i];
                
                if( instance->m_objId == _id )
                {                    
                    if( !_eventName || strstr(instance->m_eventName, _eventName) )
                    {
                        if( instance->IsPlaying() )
                        {
                            instance->BeginRelease(true);
                        }
                        else
                        {
                            ShutdownSound( instance );
                        }
                    }
                }
            }
        }
    }
}


int SoundSystem::IsSoundPlaying( SoundInstanceId _id )
{
    for( int i = 0; i < m_numChannels; ++i )
    {            
        SoundInstanceId soundId = m_channels[i];            
        if( _id == soundId )
        {
            return i;
        }
    }

    return -1;               
}


int SoundSystem::NumInstancesPlaying( WorldObjectId _id, char *_eventName )
{
    int result = 0;

    for( int i = 0; i < m_numChannels; ++i )
    {            
        SoundInstanceId soundId = m_channels[i];            
        SoundInstance *instance = GetSoundInstance( soundId );
        bool instanceMatch = !_id.IsValid() || instance->m_objId == _id;

        if( instance && 
            instanceMatch &&
            stricmp( instance->m_eventName, _eventName ) == 0 )
        {
            ++result;
        }                
    }
    return result;
}


int SoundSystem::NumInstances( WorldObjectId _id, char *_eventName )
{
    int result = 0;

    for( int i = 0; i < m_sounds.Size(); ++i )
    {            
        if( m_sounds.ValidIndex(i) )
        {
            SoundInstance *instance = m_sounds[i];
            bool instanceMatch = !_id.IsValid() || instance->m_objId == _id;

            if( instance && 
                instanceMatch &&
                stricmp( instance->m_eventName, _eventName ) == 0 )
            {
                ++result;
            }                
        }
    }
    return result;
  
}


int SoundSystem::NumSoundInstances()
{
    return m_sounds.NumUsed();
}


int SoundSystem::NumChannelsUsed()
{
    int numUsed = 0;
    for( int i = 0; i < m_numChannels; ++i )
    {
        SoundInstanceId id = m_channels[i];
        if( GetSoundInstance( id ) != NULL )
        {
            numUsed++;
        }
    }
    return numUsed;
}


int SoundSystem::NumSoundsDiscarded()
{
    return NumSoundInstances() - NumChannelsUsed();
}


int SoundInstanceCompare(const void *elem1, const void *elem2 )
{
    SoundInstanceId id1 = *((SoundInstanceId *) elem1);
    SoundInstanceId id2 = *((SoundInstanceId *) elem2);
    
    SoundInstance *instance1 = g_app->m_soundSystem->GetSoundInstance( id1 );
    SoundInstance *instance2 = g_app->m_soundSystem->GetSoundInstance( id2 );

    AppDebugAssert( instance1 );
    AppDebugAssert( instance2 );

    
    if      ( instance1->m_perceivedVolume < instance2->m_perceivedVolume )     return +1;
    else if ( instance1->m_perceivedVolume > instance2->m_perceivedVolume )     return -1;
    else                                                                        return 0;
	return 0;
}


void SoundSystem::Advance()
{
	if (g_app->m_requestQuit && !m_quitWithoutSave)
	{
		if (AreBlueprintsModified())
		{
			g_app->m_requestQuit = false;
		}
	}

	if( !m_channels || g_systemInfo->m_audioInfo.m_preferredDevice == -1 )
	{
		return;
	}

    m_mainProfiler->Advance();
    m_eventProfiler->Advance();

    m_timeSync += g_advanceTime;
    if( m_timeSync >= SOUNDSYSTEM_UPDATEPERIOD )
    {
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
		NetLockMutex lock( m_mutex );
#endif
        m_timeSync -= SOUNDSYSTEM_UPDATEPERIOD;
        
        START_PROFILE( "Advance SoundSystem");        
		

		//
		// Advance music

		if (m_music)
		{
            bool amIDone = m_music->Advance();
            if( amIDone )
            {                    
			    START_PROFILE( "Shutdown Music" );
                ShutdownSound( m_music );
				m_music = NULL;
			    END_PROFILE( "Shutdown Music" );
            }    
		}

		if (m_music == NULL && m_requestedMusic != NULL)
		{
			m_music = m_requestedMusic;
			m_requestedMusic = NULL;
			m_music->OpenStream(false);
			m_music->m_channelIndex = g_soundLibrary3d->m_musicChannelId;
			g_soundLibrary3d->ResetChannel(m_music->m_channelIndex);
			g_soundLibrary3d->SetChannelFrequency(m_music->m_channelIndex, m_music->m_cachedSampleHandle->m_cachedSample->m_freq);
		}


        //
        // Resync with blueprints (changed by editor)

        START_PROFILE(  "Propagate Blueprints" );
        if( m_propagateBlueprints )
        {
            PropagateBlueprints();
        }
        END_PROFILE(  "Propagate Blueprints" );


        //
		// First pass : Recalculate all Perceived Sound Volumes
		// Throw away sounds that have had their chance
        // Build a list of instanceIDs for sorting

        START_PROFILE( "Allocate Sorted Array" );
        static int sortedArraySize = 128;
        static SoundInstanceId *sortedIds = NULL;
        if( m_sounds.NumUsed() > sortedArraySize )
        {
            delete [] sortedIds;
            sortedIds = NULL;
            while( sortedArraySize < m_sounds.NumUsed() )
                sortedArraySize *= 2;
        }
        if( !sortedIds )
        {            
            sortedIds = new SoundInstanceId[ sortedArraySize ];
        }

        int numSortedIds = 0;
        END_PROFILE( "Allocate Sorted Array" );

        START_PROFILE( "Perceived Volumes" );
        for( int i = 0; i < m_sounds.Size(); ++i )
        {
            if( m_sounds.ValidIndex(i) )
            {
                SoundInstance *instance = m_sounds[i];
                if( !instance->IsPlaying() && !instance->m_loopType ) instance->m_restartAttempts--;
                if( instance->m_restartAttempts < 0 )
                {
                    ShutdownSound( instance );
                }
                else if( instance->m_positionType == SoundInstance::Type3DAttachedToObject &&
                        !instance->GetAttachedObject() )
                {
                    ShutdownSound( instance );
                }
                else
                {
                    instance->CalculatePerceivedVolume();
                    sortedIds[numSortedIds] = instance->m_id;
                    numSortedIds++;
                }
            }
        }
        END_PROFILE( "Perceived Volumes" );


        //        
		// Sort sounds into perceived volume order
        // NOTE : There are exactly numSortedId elements in sortedIds.  
        // NOTE : It isn't safe to assume numSortedIds == m_sounds.NumUsed()
        
        START_PROFILE( "Sort Samples" );
        qsort( sortedIds, numSortedIds, sizeof(SoundInstanceId), SoundInstanceCompare );
        END_PROFILE( "Sort Samples" );


        //
		// Second pass : Recalculate all Sound Priorities starting with the nearest sounds
        // Reduce priorities as more of the same sounds are played

        BTree<float> existingInstances;
        
                
        //                
        // Also look out for the highest priority new sound to swap in

        START_PROFILE( "Recalculate Priorities" );

        SoundInstance *newInstance = NULL;
        float highestInstancePriority = 0.0f;

        for( int i = 0; i < numSortedIds; ++i )
        {
            SoundInstanceId id = sortedIds[i];
            SoundInstance *instance = GetSoundInstance( id );
            AppDebugAssert( instance );

            instance->m_calculatedPriority = instance->m_perceivedVolume;

            BTree<float> *existingInstance = existingInstances.LookupTree( instance->m_eventName );
            if( existingInstance )
            {
                instance->m_calculatedPriority *= existingInstance->data;
                existingInstance->data *= 0.75f;
            }
            else
            {
                existingInstances.PutData( instance->m_eventName, 0.75f );
            }


            if( !instance->IsPlaying() && instance->m_calculatedPriority > highestInstancePriority )
            {
                newInstance = instance;
                highestInstancePriority = instance->m_calculatedPriority;
            }
        }

        END_PROFILE( "Recalculate Priorities" );
        
     
        if( newInstance )
        {            
            // Find worst old sound to get rid of  
            START_PROFILE( "Find best Channel" );
            int bestAvailableChannel = FindBestAvailableChannel();
            END_PROFILE( "Find best Channel" );


            START_PROFILE( "Stop Old Sound" );
			// Stop the old sound
            SoundInstance *existingInstance = GetSoundInstance( m_channels[bestAvailableChannel] );
            if( existingInstance && !existingInstance->m_loopType )
            {
                ShutdownSound( existingInstance );
            }
            else if( existingInstance )
            {
                existingInstance->StopPlaying();
            }            
            END_PROFILE( "Stop Old Sound" );


            START_PROFILE( "Start New Sound" );
			// Start the new sound
            bool success = newInstance->StartPlaying( bestAvailableChannel );
            if( success )
            {
                m_channels[bestAvailableChannel] = newInstance->m_id;
            }
            else
            {
                // This is fairly bad, the sound failed to play
                // Which means it failed to load, or to go into a channel
                ShutdownSound( newInstance );
            }
            END_PROFILE( "Start New Sound" );
            
            START_PROFILE( "Reset Channel" );
            g_soundLibrary3d->ResetChannel( bestAvailableChannel);
            END_PROFILE( "Reset Channel" );
        }


        //
        // Advance all sound channels

        START_PROFILE( "Advance All Channels" );
        for( int i = 0; i < m_numChannels; ++i )
        {            
            SoundInstanceId soundId = m_channels[i];            
            SoundInstance *currentSound = GetSoundInstance( soundId );                         
            if( currentSound )
            {
                bool amIDone = currentSound->Advance();
                if( amIDone )
                {                    
			        START_PROFILE( "Shutdown Sound" );
                    ShutdownSound( currentSound );
			        END_PROFILE( "Shutdown Sound" );
                }    
            }
        }
        END_PROFILE( "Advance All Channels" );

        g_soundLibrary3d->m_forceVolumeUpdate = false;


		//
        // Update our listener position

        START_PROFILE( "UpdateListener" );    

        Vector3 camUp = g_app->m_camera->GetUp();
        if( g_prefsManager->GetInt("SoundSwapStereo",0) == 0 )
        {
            camUp *= -1.0f;
        }

        Vector3 camVel = g_app->m_camera->GetVel() * 0.2f;
        g_soundLibrary3d->SetListenerPosition( g_app->m_camera->GetPos(), 
                                               g_app->m_camera->GetFront(),
                                               camUp, camVel );
        
        END_PROFILE( "UpdateListener" );    


        //
        // Advance our sound library

        START_PROFILE( "SoundLibrary3d Advance" );
        g_soundLibrary3d->Advance();
        END_PROFILE( "SoundLibrary3d Advance" );
      
#ifdef SOUNDSYSTEM_VERIFY
        RuntimeVerify();
#endif

        END_PROFILE( "Advance SoundSystem");                    
    }
}

void SoundSystem::RuntimeVerify()
{
    //
    // Make sure there aren't any SoundInstances on more than one channel
    // Make sure all playing samples have sensible channel handles

/*
    for( int i = 0; i < m_numChannels; ++i )
    {
        SoundInstanceId id1 = m_channels[i];
        SoundInstance *currentSound = GetSoundInstance( id1 );
        AppDebugAssert( !currentSound || 
                     !(currentSound->IsPlaying() && currentSound->m_channelIndex == -1) );
        
        if( currentSound )
        {                
            for( int j = 0; j < m_numChannels; ++j )
            {
                if( i != j )
                {
                    SoundInstanceId id2 = m_channels[j];
                    if( GetSoundInstance(id2) )
                    {
                        AppDebugAssert( !(id1 == id2) );
                    }
                }
            }
        }
    }
*/

    //
    // Make sure all sounds that believe they are playing have an opened sound stream

    for( int i = 0; i < m_numChannels; ++i )
    {
        SoundInstanceId id1 = m_channels[i];
        SoundInstance *currentSound = GetSoundInstance( id1 );
        if( currentSound && !currentSound->m_cachedSampleHandle )
        {
            int b = 10;
        }
    }
}


void SoundSystem::LoadtimeVerify()
{
	// If you want to comment this out then comment out the call.
    
	//
	// Verify that the samples referred to in SampleGroups are valid

    FILE *soundErrors = fopen( "sounderrors.txt", "wt" );
    bool errorFound = false;
    
	for (int i = 0; i < m_sampleGroups.Size(); ++i)
	{
		if (!m_sampleGroups.ValidIndex(i)) continue;
		
		SampleGroup *sg = m_sampleGroups[i];
		int size = sg->m_samples.Size();
		for (int j = 0; j < size; ++j)
		{
			char *soundName = sg->m_samples[j];
			char const *err = IsSoundSourceOK(soundName);
			if(err != NULL )
            {
                fprintf( soundErrors, "%s: %s In sound group %s\n", 
							            err, soundName, sg->m_name);
                errorFound = true;
            }
		}
	}

	
	//
	// Verify that the samples referred to in SoundEventBlueprints are valid

	// Entities
	for (int i = 0; i < m_entityBlueprints.Size(); ++i)
	{
		if (!m_entityBlueprints.ValidIndex(i)) continue;

		SoundSourceBlueprint *ssb = m_entityBlueprints[i];
		int size = ssb->m_events.Size();
		
		for (int j = 0; j < size; ++j)
		{
			SoundEventBlueprint *seb = ssb->m_events[j];
			SoundInstance *si = seb->m_instance;
            if ( si->m_sourceType == SoundInstance::Sample )
			{
				char const *err = IsSoundSourceOK(si->m_soundName);
				if(err != NULL )
                {
                    fprintf( soundErrors, "%s: %s In entity %s, event %s\n", 
					        err, si->m_soundName, Entity::GetTypeName(i), seb->m_eventName);
                    errorFound = true;
                }
			}
			else
			{
				// Just need to verify that the group exists here
				SampleGroup *sg = GetSampleGroup(si->m_soundName);
				if(!sg)
                {
                    fprintf( soundErrors, "Sound Group %s does not exist. "
								    "Referenced by entity %s, event %s\n",
								    si->m_soundName, Entity::GetTypeName(i), 
								    seb->m_eventName);
                    errorFound = true;
                }
			}
		}
	}

	// Buildings
	for (int i = 0; i < m_buildingBlueprints.Size(); ++i)
	{
		if (!m_buildingBlueprints.ValidIndex(i)) continue;

		SoundSourceBlueprint *ssb = m_buildingBlueprints[i];
		int size = ssb->m_events.Size();
		
		for (int j = 0; j < size; ++j)
		{
			SoundEventBlueprint *seb = ssb->m_events[j];
			SoundInstance *si = seb->m_instance;
            if( si->m_sourceType == SoundInstance::Sample )
			{
				char const *err = IsSoundSourceOK(si->m_soundName);
				if(err != NULL)
                {
                    fprintf( soundErrors, "%s: %s In building %s, event %s\n", 
					            err, si->m_soundName, Building::GetTypeName(i), seb->m_eventName);
                    errorFound = true;
                }
			}
			else
			{
				// Just need to verify that the group exists here
				SampleGroup *sg = GetSampleGroup(si->m_soundName);
				if(!sg)
                {
                    fprintf( soundErrors, "Sound Group %s does not exist. "
								    "Referenced by building %s, event %s\n",
								    si->m_soundName, Building::GetTypeName(i), 
								    seb->m_eventName);
                    errorFound = true;
                }
			}
		}
	}

	// Others
	for (int i = 0; i < m_otherBlueprints.Size(); ++i)
	{
		if (!m_otherBlueprints.ValidIndex(i)) continue;

		SoundSourceBlueprint *ssb = m_otherBlueprints[i];
		int size = ssb->m_events.Size();
		
		for (int j = 0; j < size; ++j)
		{
			SoundEventBlueprint *seb = ssb->m_events[j];
			SoundInstance *si = seb->m_instance;
            if( si->m_sourceType == SoundInstance::Sample )
			{
				char const *err = IsSoundSourceOK(si->m_soundName);
				if(err != NULL)
                {
                    fprintf( soundErrors, "%s: %s In Others %s, event %s\n", 
					        err, si->m_soundName, ssb->GetSoundSourceName(i), seb->m_eventName);
                    errorFound = true;
                }
			}
			else
			{
				// Just need to verify that the group exists here
				SampleGroup *sg = GetSampleGroup(si->m_soundName);
				if(!sg)
                {
                    fprintf( soundErrors, "Sound Group %s does not exist. "
								    "Referenced by \"others\" %s, event %s\n",
								    si->m_soundName, ssb->GetSoundSourceName(i), 
								    seb->m_eventName);
                    errorFound = true;
                }
			}
		}
	}


    fclose( soundErrors );
    AppReleaseAssert( !errorFound, "Errors found in sounds.txt : refer to sounderrors.txt for details" );
}


static char const *g_soundSourceErrors[SoundSystem::SoundSourceNumErrors] =
{
	NULL,
	"Sound sample not mono",
	"Sound sample does not exist",
	"Sound sample name contains spaces"
};


char const *SoundSystem::IsSoundSourceOK(char const *_soundName)
{
#ifdef _DEBUG
	if (strchr(_soundName, ' '))
	{
		// File name contains a space
		return g_soundSourceErrors[SoundSourceFilenameHasSpace];
	}

	char fullPath[256] = "sounds/";
	strcat(fullPath, _soundName);

	SoundStreamDecoder *sound = g_app->m_resource->GetSoundStreamDecoder(fullPath);
	if (!sound)
	{
		// File does not exist
		return g_soundSourceErrors[SoundSourceFileDoesNotExist];
	}

	if (sound->m_numChannels != 1)
	{
		// File isn't mono
		return g_soundSourceErrors[SoundSourceNotMono];
	}
	delete sound;
#endif

	// Everything is dandy
	return g_soundSourceErrors[SoundSourceNoError];
	return "";
}


bool SoundSystem::IsSampleUsed(char const *_soundName)
{
    //
    // Entity blueprints

    for( int i = 0; i < m_entityBlueprints.Size(); ++i )
    {
        if( m_entityBlueprints.ValidIndex(i) )
        {
            SoundSourceBlueprint *sourceBlueprint = m_entityBlueprints[i];
            for( int j = 0; j < sourceBlueprint->m_events.Size(); ++j )
            {
                SoundEventBlueprint *eventBlueprint = sourceBlueprint->m_events[j];
                if( eventBlueprint->m_instance )
                {
                    SoundInstance *instance = eventBlueprint->m_instance;
                    if( instance->m_sourceType == SoundInstance::Sample &&
                        stricmp( instance->m_soundName, _soundName ) == 0 )
                    {
                        return true;
                    }
                    else if( instance->m_sourceType != SoundInstance::Sample )
                    {
                        SampleGroup *group = GetSampleGroup( instance->m_soundName );
                        if( group )
                        {
                            for( int k = 0; k < group->m_samples.Size(); ++k )
                            {
                                char *thisSample = group->m_samples[k];
                                if( stricmp( thisSample, _soundName ) == 0 )
                                {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    
    //
    // Building blueprints

    for( int i = 0; i < m_buildingBlueprints.Size(); ++i )
    {
        if( m_buildingBlueprints.ValidIndex(i) )
        {
            SoundSourceBlueprint *sourceBlueprint = m_buildingBlueprints[i];
            for( int j = 0; j < sourceBlueprint->m_events.Size(); ++j )
            {
                SoundEventBlueprint *eventBlueprint = sourceBlueprint->m_events[j];
                if( eventBlueprint->m_instance )
                {
                    SoundInstance *instance = eventBlueprint->m_instance;
                    if( instance->m_sourceType == SoundInstance::Sample &&
                        stricmp( instance->m_soundName, _soundName ) == 0 )
                    {
                        return true;
                    }
                    else if( instance->m_sourceType != SoundInstance::Sample )
                    {
                        SampleGroup *group = GetSampleGroup( instance->m_soundName );
                        if( group )
                        {
                            for( int k = 0; k < group->m_samples.Size(); ++k )
                            {
                                char *thisSample = group->m_samples[k];
                                if( stricmp( thisSample, _soundName ) == 0 )
                                {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    //
    // Other blueprints

    for( int i = 0; i < m_otherBlueprints.Size(); ++i )
    {
        if( m_otherBlueprints.ValidIndex(i) )
        {
            SoundSourceBlueprint *sourceBlueprint = m_otherBlueprints[i];
            for( int j = 0; j < sourceBlueprint->m_events.Size(); ++j )
            {
                SoundEventBlueprint *eventBlueprint = sourceBlueprint->m_events[j];
                if( eventBlueprint->m_instance )
                {
                    SoundInstance *instance = eventBlueprint->m_instance;
                    if( instance->m_sourceType == SoundInstance::Sample &&
                        stricmp( instance->m_soundName, _soundName ) == 0 )
                    {
                        return true;
                    }
                    else if( instance->m_sourceType != SoundInstance::Sample )
                    {
                        SampleGroup *group = GetSampleGroup( instance->m_soundName );
                        if( group )
                        {
                            for( int k = 0; k < group->m_samples.Size(); ++k )
                            {
                                char *thisSample = group->m_samples[k];
                                if( stricmp( thisSample, _soundName ) == 0 )
                                {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}


void SoundSystem::PropagateBlueprints()
{
#ifdef INVOKE_CALLBACK_FROM_SOUND_THREAD
	NetLockMutex lock( m_mutex );
#endif
    for( int i = 0; i < m_sounds.Size(); ++i )
    {
        if( m_sounds.ValidIndex(i) )
        {
            SoundInstance *instance = m_sounds[i];
            instance->PropagateBlueprints();
        }
    }
}
    

SampleGroup *SoundSystem::GetSampleGroup ( const char *_name )
{
    for( int i = 0; i < m_sampleGroups.Size(); ++i )
    {
        if( m_sampleGroups.ValidIndex(i) )
        {
            SampleGroup *group = m_sampleGroups[i];
            if( stricmp( group->m_name, _name ) == 0 )
            {
                return group;
            }
        }
    }
    return NULL;
}


SampleGroup *SoundSystem::NewSampleGroup ( char *_name )
{
    SampleGroup *group = new SampleGroup();
    m_sampleGroups.PutData( group );

    if( _name )
    {
        group->SetName( _name );
        return group;
    }
    else
    {
        int i = 1;
        while( true )
        {
            char nameCandidate[256];
            sprintf( nameCandidate, "newsamplegroup%d", i );
            if( !GetSampleGroup( nameCandidate ) )
            {
                group->SetName( nameCandidate );
                return group;
            }
            ++i;
        }
    }
	return NULL;
}


bool SoundSystem::RenameSampleGroup( char *_oldName, char *_newName )
{
    //
    // Check the new name is unique

    if( GetSampleGroup( _newName ) )
    {
        return false;
    }


    //
    // Rename it

    SampleGroup *group = GetSampleGroup( _oldName );
    if( group )
    {
        group->SetName( _newName );

        //
        // Update entity blueprints
        
        for( int i = 0; i < m_entityBlueprints.Size(); ++i )
        {
            if( m_entityBlueprints.ValidIndex(i) )
            {
                SoundSourceBlueprint *blueprint = m_entityBlueprints[i];
                for( int j = 0; j < blueprint->m_events.Size(); ++j )
                {
                    SoundEventBlueprint *eventBlueprint = blueprint->m_events[j];
                    if( eventBlueprint->m_instance &&
                        eventBlueprint->m_instance->m_sourceType > SoundInstance::Sample &&
                        strcmp( eventBlueprint->m_instance->m_soundName, _oldName ) == 0 )
                    {
                        eventBlueprint->m_instance->SetSoundName( _newName );
                    }
                }
            }
        }

        //
        // Update building blueprints
        
        for( int i = 0; i < m_buildingBlueprints.Size(); ++i )
        {
            if( m_buildingBlueprints.ValidIndex(i) )
            {
                SoundSourceBlueprint *blueprint = m_buildingBlueprints[i];
                for( int j = 0; j < blueprint->m_events.Size(); ++j )
                {
                    SoundEventBlueprint *eventBlueprint = blueprint->m_events[j];
                    if( eventBlueprint->m_instance &&
                        eventBlueprint->m_instance->m_sourceType > SoundInstance::Sample &&
                        strcmp( eventBlueprint->m_instance->m_soundName, _oldName ) == 0 )
                    {
                        eventBlueprint->m_instance->SetSoundName( _newName );
                    }
                }
            }
        }

        //
        // Update other blueprints
        
        for( int i = 0; i < m_otherBlueprints.Size(); ++i )
        {
            if( m_otherBlueprints.ValidIndex(i) )
            {
                SoundSourceBlueprint *blueprint = m_otherBlueprints[i];
                for( int j = 0; j < blueprint->m_events.Size(); ++j )
                {
                    SoundEventBlueprint *eventBlueprint = blueprint->m_events[j];
                    if( eventBlueprint->m_instance &&
                        eventBlueprint->m_instance->m_sourceType > SoundInstance::Sample &&
                        strcmp( eventBlueprint->m_instance->m_soundName, _oldName ) == 0 )
                    {
                        eventBlueprint->m_instance->SetSoundName( _newName );
                    }
                }
            }
        }
    }

    return true;
}



