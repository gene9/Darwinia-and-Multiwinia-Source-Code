#include "lib/universal_include.h"
#include "lib/debug_utils.h"
#include "lib/filesys_utils.h"
#include "lib/file_writer.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/preferences.h"
#include "lib/resource.h"
#include "lib/string_utils.h"
#include "lib/text_stream_readers.h"
#include "lib/window_manager.h"
#include "lib/language_table.h"

#include "interface/save_on_quit_window.h"

#include "sound/sample_cache.h"
#include "sound/sound_library_2d.h"	// FIXME
#include "sound/sound_library_3d.h"
#include "sound/soundsystem.h"
#include "sound/sound_stream_decoder.h"
#include "sound/sound_library_3d_dsound.h"
#include "sound/sound_library_3d_software.h"


#include "app.h"
#include "main.h"
#include "camera.h"
#include "location.h"

#include "worldobject/entity.h"


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


void SoundEventBlueprint::SetEventName( char *_name )
{
    if( m_eventName )
    {
        delete [] m_eventName;
        m_eventName = NULL;
    }

    if( _name ) m_eventName = NewStr( _name );
}



//*****************************************************************************
// Class SoundSourceBlueprint
//*****************************************************************************

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
                        "Interface"
                    };

    DarwiniaDebugAssert( _type >= 0 && _type < NumOtherSoundSources );
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
    }
}


//*****************************************************************************
// Class DspBlueprint
//*****************************************************************************

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
    char *sampleCopy = NewStr( _sample );
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
	m_requestedMusic(NULL)
{    
#ifdef PROFILER_ENABLED
    m_eventProfiler = new Profiler();
    m_mainProfiler = new Profiler();
#endif
}


SoundSystem::~SoundSystem()
{
#ifdef PROFILER_ENABLED
    delete m_eventProfiler;
    delete m_mainProfiler;
#endif
    delete [] m_channels;
    
    m_sounds.EmptyAndDelete();
    m_entityBlueprints.EmptyAndDelete();
    m_buildingBlueprints.EmptyAndDelete();
    m_otherBlueprints.EmptyAndDelete();
    m_filterBlueprints.EmptyAndDelete();
    m_sampleGroups.EmptyAndDelete();

    delete g_soundLibrary3d;
    delete g_soundLibrary2d;
    g_soundLibrary3d = NULL;
    g_soundLibrary2d = NULL;
}


void SoundSystem::Initialise()
{
    LoadEffects();
    LoadBlueprints();

    RestartSoundLibrary();
}


void SoundSystem::RestartSoundLibrary()
{
    //
    // Shut down existing sound library

    if( m_channels )
    {
        delete [] m_channels;
        delete g_soundLibrary3d;
        delete g_soundLibrary2d;
        g_soundLibrary2d = NULL;
        g_soundLibrary3d = NULL;
    }
    
    
    //
    // Start up a new sound library

	int mixrate     = g_prefsManager->GetInt("SoundMixFreq", 22050);
	int volume      = g_prefsManager->GetInt("SoundMasterVolume", 255);
    m_numChannels   = g_prefsManager->GetInt("SoundChannels", 32);
    int hw3d        = g_prefsManager->GetInt("SoundHW3D", 0);
    char *libName   = g_prefsManager->GetString("SoundLibrary", "dsound");
	int bufSize		= 20000;

	g_soundLibrary2d = new SoundLibrary2d;
	g_soundLibrary3d = NULL;
	
#ifdef HAVE_DSOUND
	if (stricmp(libName, "dsound") == 0)    g_soundLibrary3d = new SoundLibrary3dDirectSound();	
#endif
	if (!g_soundLibrary3d)                  g_soundLibrary3d = new SoundLibrary3dSoftware();
	
    g_soundLibrary3d->SetMasterVolume( volume );
    g_soundLibrary3d->Initialise( mixrate, m_numChannels, hw3d, bufSize, bufSize * 10 );
    
    m_numChannels = g_soundLibrary3d->GetNumMainChannels();	
    m_channels = new SoundInstanceId[m_numChannels];    
    
    g_soundLibrary3d->SetMainCallback( &SoundLibraryMainCallback );
    g_soundLibrary3d->SetMusicCallback( &SoundLibraryMusicCallback );
}


void SoundSystem::StopAllDSPEffects()
{
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


bool SoundSystem::SoundLibraryMainCallback( unsigned int _channel, signed short *_data, 
									        unsigned int _numSamples, int *_silenceRemaining )
{
    if( !g_app->m_soundSystem ) return false;

	SoundInstanceId soundId = g_app->m_soundSystem->m_channels[_channel];
    SoundInstance *instance = g_app->m_soundSystem->GetSoundInstance( soundId );

    if( instance && instance->m_cachedSampleHandle )
    {
#ifdef PROFILER_ENABLED
		g_app->m_soundSystem->m_eventProfiler->StartProfile(instance->m_eventName);
#endif

   
		//
        // Fill the space with sample data

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

#ifdef PROFILER_ENABLED
		g_app->m_soundSystem->m_eventProfiler->EndProfile(instance->m_eventName);
#endif
		return true;
    }
    else
    {
        g_soundLibrary3d->WriteSilence( _data, _numSamples );
		return false;
    }
}


bool SoundSystem::SoundLibraryMusicCallback( signed short *_data, unsigned int _numSamples, int *_silenceRemaining )
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

#ifdef PROFILER_ENABLED
		g_app->m_soundSystem->m_eventProfiler->EndProfile(instance->m_eventName);
#endif
		return true;
    }
    else
    {
        g_soundLibrary3d->WriteSilence( _data, _numSamples );
		return false;
    }
}


void SoundSystem::LoadEffects()
{
    m_filterBlueprints.SetSize( SoundLibrary3d::NUM_FILTERS );
    
    TextReader *in = g_app->m_resource->GetTextReader( "effects.txt" );
	DarwiniaReleaseAssert(in && in->IsOpen(), "Couldn't load effects.txt");

    while( in->ReadLine() )
    {
        if( !in->TokenAvailable() ) continue;
        char *effect = in->GetNextToken();
        DarwiniaDebugAssert( stricmp( effect, "EFFECT" ) == 0 );

        DspBlueprint *bp = new DspBlueprint();
        m_filterBlueprints.PutData( bp );
        strcpy( bp->m_name, in->GetNextToken() );

        in->ReadLine();
        char *param = in->GetNextToken();
        while( stricmp( param, "END" ) != 0 )
        {
            DspParameterBlueprint *sb = new DspParameterBlueprint();
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
		SaveOnQuitWindow *saveWin = new SaveOnQuitWindow(LANGUAGEPHRASE("editor_savesettings"));
		EclRegisterWindow(saveWin);
	}
	return !result;
#else
	return false;
#endif
}


void SoundSystem::LoadBlueprints()
{
    m_entityBlueprints.SetSize( Entity::NumEntityTypes );
    m_buildingBlueprints.SetSize( Building::NumBuildingTypes );
    m_otherBlueprints.SetSize( SoundSourceBlueprint::NumOtherSoundSources );
    
    TextReader *in = g_app->m_resource->GetTextReader( "sounds.txt" );
	DarwiniaReleaseAssert(in && in->IsOpen(), "Couldn't open sounds.txt");

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
            DarwiniaDebugAssert( entityType >= 0 && entityType < Entity::NumEntityTypes );
            DarwiniaDebugAssert( !m_entityBlueprints.ValidIndex( entityType ) );
            
            ssb = new SoundSourceBlueprint();
            m_entityBlueprints.PutData( ssb, entityType );
            event = true;
        }
        else if( stricmp( group, "BUILDING" ) == 0 )
        {
			strncpy(objectName, type, 127);
            int buildingType = Building::GetTypeId( type );
            DarwiniaDebugAssert( buildingType >= 0 && buildingType < Building::NumBuildingTypes );
            DarwiniaDebugAssert( !m_buildingBlueprints.ValidIndex( buildingType ) );

            ssb = new SoundSourceBlueprint();
            m_buildingBlueprints.PutData( ssb, buildingType );
            event = true;
        }
        else if( stricmp( group, "OTHER" ) == 0 )
        {
			strncpy(objectName, type, 127);
            int otherType = SoundSourceBlueprint::GetSoundSoundType( type );
            DarwiniaDebugAssert( otherType >= 0 && otherType < SoundSourceBlueprint::NumOtherSoundSources );
            DarwiniaDebugAssert( !m_otherBlueprints.ValidIndex( otherType ) );

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
                    DarwiniaDebugAssert( stricmp( word, "EVENT"  ) == 0 );
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

	delete in;
}


void SoundSystem::SaveBlueprints()
{
	SaveBlueprints("sounds.txt");
}


void SoundSystem::SaveBlueprints(char const *_filename)
{
    FileWriter *file = g_app->m_resource->GetFileWriter( _filename, false );
    // Not always possible - this may be a release version with a data.dat
    if( !file ) return;
     
    for( int i = 0; i < m_entityBlueprints.Size(); ++i )
    {
        DarwiniaDebugAssert( m_entityBlueprints.ValidIndex(i) );
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
        DarwiniaDebugAssert( m_buildingBlueprints.ValidIndex(i) );
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
        DarwiniaDebugAssert( m_otherBlueprints.ValidIndex(i) );
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
        else if ( stricmp( fieldName, "PRIORITY" ) == 0 )       oldUserPriority                     = atoi( _in->GetNextToken() );
        else if ( stricmp( fieldName, "MINDISTANCE" ) == 0 )    seb->m_instance->m_minDistance      = atof( _in->GetNextToken() );
        else if ( stricmp( fieldName, "VOLUME" ) == 0 )         seb->m_instance->m_volume.Read( _in );
        else if ( stricmp( fieldName, "FREQUENCY" ) == 0 )	    seb->m_instance->m_freq.Read( _in );
        else if ( stricmp( fieldName, "ATTACK" ) == 0 )         seb->m_instance->m_attack.Read( _in );
        else if ( stricmp( fieldName, "SUSTAIN" ) == 0 )        seb->m_instance->m_sustain.Read( _in );
        else if ( stricmp( fieldName, "RELEASE" ) == 0 )        seb->m_instance->m_release.Read( _in );
        else if ( stricmp( fieldName, "LOOPDELAY" ) == 0 )      seb->m_instance->m_loopDelay.Read( _in );
        else if ( stricmp( fieldName, "EFFECT" ) == 0 )         ParseSoundEffect( _in, seb );
        else												    DarwiniaDebugAssert( false );
        
        // This is bad, we have a looping sound that won't be attached
        // to any one object
//        DarwiniaDebugAssert( !( seb->m_instance->m_loopType &&
//                        seb->m_instance->m_positionType != SoundInstance::Type3DAttachedToObject ) );

        _in->ReadLine();
        fieldName = _in->GetNextToken();
    }
        
    _source->m_events.PutData( seb );
}


void SoundSystem::ParseSoundEffect( TextReader *_in, SoundEventBlueprint *_blueprint )
{
    char *effectName = _in->GetNextToken();
    int fxType = -1;

    for( int i = 0; i < m_filterBlueprints.Size(); ++i )
    {
        DspBlueprint *seb = m_filterBlueprints[i];
        if( stricmp( seb->m_name, effectName ) == 0 )
        {
            fxType = i;
            break;
        }
    }
    
    DarwiniaDebugAssert( fxType != -1 );
    
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
}


void SoundSystem::ParseSampleGroup( TextReader *_in, SampleGroup *_group )
{
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
    }
}


void SoundSystem::WriteSoundEvent( FileWriter *_file, SoundEventBlueprint *_event )
{
    DarwiniaDebugAssert( _event );
    DarwiniaDebugAssert( _event->m_instance );

    _file->printf( "\tEVENT %-20s\n"
                    "\t\tSOUNDNAME          %s\n"
                    "\t\tSOURCETYPE         %d\n"
                    "\t\tPOSITIONTYPE       %d\n"
                    "\t\tINSTANCETYPE       %d\n"
                    "\t\tLOOPTYPE           %d\n"
                    "\t\tMINDISTANCE        %2.2f\n",
                                        _event->m_eventName,
                                        _event->m_instance->m_soundName,
                                        _event->m_instance->m_sourceType,
                                        _event->m_instance->m_positionType,
                                        _event->m_instance->m_instanceType,
                                        _event->m_instance->m_loopType,
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


void SoundSystem::WriteSampleGroup( FileWriter *_file, SampleGroup *_group )
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
}


void SoundSystem::ShutdownSound( SoundInstance *_instance )
{
    if( m_sounds.ValidIndex( _instance->m_id.m_index ) )
    {
        m_sounds.MarkNotUsed( _instance->m_id.m_index );
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


void SoundSystem::TriggerEntityEvent( Entity *_entity, char *_eventName )
{
    if( !m_channels ) return;

	START_PROFILE(m_mainProfiler, "TriggerEntityEvent");
    WorldObjectId objId( _entity->m_id );

    if( m_entityBlueprints.ValidIndex( _entity->m_type ) )
    {   
        SoundSourceBlueprint *sourceBlueprint = m_entityBlueprints[ _entity->m_type ];
        for( int i = 0; i < sourceBlueprint->m_events.Size(); ++i )
        {
            SoundEventBlueprint *seb = sourceBlueprint->m_events[i];
            if( stricmp( seb->m_eventName, _eventName ) == 0 )
            {
                DarwiniaDebugAssert( seb->m_instance );
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

	END_PROFILE(m_mainProfiler, "TriggerEntityEvent");
}

void SoundSystem::TriggerBuildingEvent( Building *_building, char *_eventName )
{
    if( !m_channels ) return;
    
 	START_PROFILE(m_mainProfiler, "TriggerBuildingEvent");
    if( m_buildingBlueprints.ValidIndex( _building->m_type ) )
    {   
        SoundSourceBlueprint *sourceBlueprint = m_buildingBlueprints[ _building->m_type ];
        for( int i = 0; i < sourceBlueprint->m_events.Size(); ++i )
        {
            SoundEventBlueprint *seb = sourceBlueprint->m_events[i];
            if( stricmp( seb->m_eventName, _eventName ) == 0 )
            {
                DarwiniaDebugAssert( seb->m_instance );
                SoundInstance *instance = new SoundInstance();
                instance->Copy( seb->m_instance );
                instance->m_objIds.PutData( new WorldObjectId(_building->m_id) );
                instance->m_pos = _building->m_pos;
                bool success = InitialiseSound  ( instance );
                if( !success ) ShutdownSound    ( instance );                
            }
        }
    }

	END_PROFILE(m_mainProfiler, "TriggerBuildingEvent");
}


void SoundSystem::TriggerOtherEvent( WorldObject *_other, char *_eventName, int _type )
{
    if( !m_channels ) return;

	START_PROFILE(m_mainProfiler, "TriggerOtherEvent");

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
            if( stricmp( seb->m_eventName, _eventName ) == 0 )
            {
				// We have a match
                DarwiniaDebugAssert( seb->m_instance );
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

	END_PROFILE(m_mainProfiler, "TriggerOtherEvent");
}


void SoundSystem::TriggerDuplicateSound ( SoundInstance *_instance )
{
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
    if( strstr( _eventName, "Music" ) )
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
                    if( !_eventName || stricmp(instance->m_eventName, _eventName) == 0 )
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

    DarwiniaDebugAssert( instance1 );
    DarwiniaDebugAssert( instance2 );

    
    if      ( instance1->m_perceivedVolume < instance2->m_perceivedVolume )     return +1;
    else if ( instance1->m_perceivedVolume > instance2->m_perceivedVolume )     return -1;
    else                                                                        return 0;
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

    if( !m_channels )
	{
		return;
	}

#ifdef PROFILER_ENABLED
    m_mainProfiler->Advance();
    m_eventProfiler->Advance();
#endif

    m_timeSync += g_advanceTime;
    if( m_timeSync >= SOUNDSYSTEM_UPDATEPERIOD )
    {
        m_timeSync -= SOUNDSYSTEM_UPDATEPERIOD;
        
        START_PROFILE(g_app->m_profiler, "Advance SoundSystem");        
		

		//
		// Advance music

		if (m_music)
		{
            bool amIDone = m_music->Advance();
            if( amIDone )
            {                    
			    START_PROFILE(g_app->m_profiler, "Shutdown Music" );
                ShutdownSound( m_music );
				m_music = NULL;
			    END_PROFILE(g_app->m_profiler, "Shutdown Music" );
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

        START_PROFILE(g_app->m_profiler,  "Propagate Blueprints" );
        if( m_propagateBlueprints )
        {
            PropagateBlueprints();
        }
        END_PROFILE(g_app->m_profiler,  "Propagate Blueprints" );


        //
		// First pass : Recalculate all Perceived Sound Volumes
		// Throw away sounds that have had their chance
        // Build a list of instanceIDs for sorting

        START_PROFILE(g_app->m_profiler, "Allocate Sorted Array" );
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
        END_PROFILE(g_app->m_profiler, "Allocate Sorted Array" );

        START_PROFILE(g_app->m_profiler, "Perceived Volumes" );
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
        END_PROFILE(g_app->m_profiler, "Perceived Volumes" );


        //        
		// Sort sounds into perceived volume order
        // NOTE : There are exactly numSortedId elements in sortedIds.  
        // NOTE : It isn't safe to assume numSortedIds == m_sounds.NumUsed()
        
        START_PROFILE(g_app->m_profiler, "Sort Samples" );
        qsort( sortedIds, numSortedIds, sizeof(SoundInstanceId), SoundInstanceCompare );
        END_PROFILE(g_app->m_profiler, "Sort Samples" );


        //
		// Second pass : Recalculate all Sound Priorities starting with the nearest sounds
        // Reduce priorities as more of the same sounds are played

        BTree<float> existingInstances;
        
                
        //                
        // Also look out for the highest priority new sound to swap in

        START_PROFILE(g_app->m_profiler, "Recalculate Priorities" );

        SoundInstance *newInstance = NULL;
        float highestInstancePriority = 0.0f;

        for( int i = 0; i < numSortedIds; ++i )
        {
            SoundInstanceId id = sortedIds[i];
            SoundInstance *instance = GetSoundInstance( id );
            DarwiniaDebugAssert( instance );

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

        END_PROFILE(g_app->m_profiler, "Recalculate Priorities" );
        
     
        if( newInstance )
        {            
            // Find worst old sound to get rid of  
            START_PROFILE(g_app->m_profiler, "Find best Channel" );
            int bestAvailableChannel = FindBestAvailableChannel();
            END_PROFILE(g_app->m_profiler, "Find best Channel" );


            START_PROFILE(g_app->m_profiler, "Stop Old Sound" );
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
            END_PROFILE(g_app->m_profiler, "Stop Old Sound" );


            START_PROFILE(g_app->m_profiler, "Start New Sound" );
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
            END_PROFILE(g_app->m_profiler, "Start New Sound" );
            
            START_PROFILE(g_app->m_profiler, "Reset Channel" );
            g_soundLibrary3d->ResetChannel( bestAvailableChannel);
            END_PROFILE(g_app->m_profiler, "Reset Channel" );
        }


        //
        // Advance all sound channels

        START_PROFILE(g_app->m_profiler, "Advance All Channels" );
        for( int i = 0; i < m_numChannels; ++i )
        {            
            SoundInstanceId soundId = m_channels[i];            
            SoundInstance *currentSound = GetSoundInstance( soundId );                         
            if( currentSound )
            {
                bool amIDone = currentSound->Advance();
                if( amIDone )
                {                    
			        START_PROFILE(g_app->m_profiler, "Shutdown Sound" );
                    ShutdownSound( currentSound );
			        END_PROFILE(g_app->m_profiler, "Shutdown Sound" );
                }    
            }
        }
        END_PROFILE(g_app->m_profiler, "Advance All Channels" );


		//
        // Update our listener position

        START_PROFILE(g_app->m_profiler, "UpdateListener" );    

        Vector3 camUp = g_app->m_camera->GetUp();
        if( g_prefsManager->GetInt("SoundSwapStereo",0) == 0 )
        {
            camUp *= -1.0f;
        }

        Vector3 camVel = g_app->m_camera->GetVel() * 0.2f;
        g_soundLibrary3d->SetListenerPosition( g_app->m_camera->GetPos(), 
                                               g_app->m_camera->GetFront(),
                                               camUp, camVel );
        
        END_PROFILE(g_app->m_profiler, "UpdateListener" );    


        //
        // Advance our sound library

        START_PROFILE(g_app->m_profiler, "SoundLibrary3d Advance" );
        g_soundLibrary3d->Advance();
        END_PROFILE(g_app->m_profiler, "SoundLibrary3d Advance" );
      
#ifdef SOUNDSYSTEM_VERIFY
        RuntimeVerify();
#endif

        END_PROFILE(g_app->m_profiler, "Advance SoundSystem");                    
    }
}

/*
void SoundSystem::Advance()
{

	if (g_app->m_requestQuit && !m_quitWithoutSave)
	{
		if (AreBlueprintsModified())
		{
			g_app->m_requestQuit = false;
		}
	}

    if( !m_channels ) return;

#ifdef PROFILER_ENABLED
    m_mainProfiler->Advance();
    m_eventProfiler->Advance();
#endif
    
    m_timeSync += g_advanceTime;
    if( m_timeSync >= SOUNDSYSTEM_UPDATEPERIOD )
    {
        m_timeSync -= SOUNDSYSTEM_UPDATEPERIOD;

        START_PROFILE(g_app->m_profiler, "Advance SoundSystem");                    

        //
        // Resync with blueprints (changed by editor)

        START_PROFILE(g_app->m_profiler,  "Propagate Blueprints" );
        if( m_propagateBlueprints )
        {
            PropagateBlueprints();
        }
        END_PROFILE(g_app->m_profiler,  "Propagate Blueprints" );
        

        //
        // Build a list of change requests, ordered on priority

        START_PROFILE(g_app->m_profiler,  "HandleRequests" );    

        int maxChannelChanges = 1;
        int numChannelChanges = 0;

        LList<int> newRequests;                                 // Indexes of the channels

        for( int i = 0; i < m_numChannels; ++i )
        {            
            SoundChannel *channel = &m_channels[i];            
            SoundInstance *requestedSound = GetSoundInstance( channel->m_requestedSound );
            
            if( requestedSound )                                                   
            {
                bool inserted = false;
                for( int x = 0; x < newRequests.Size(); ++x )
                {
                    if( x > maxChannelChanges ) break;
                    int channelIndex = newRequests[x];
                    SoundChannel *thisChannel = &m_channels[channelIndex];
                    SoundInstance *thisRequestedSound = GetSoundInstance( thisChannel->m_requestedSound );
                    DarwiniaDebugAssert( thisRequestedSound );
                    if( requestedSound->m_calculatedPriority > thisRequestedSound->m_calculatedPriority )
                    {
                        newRequests.PutDataAtIndex( i, x );
                        inserted = true;
                        break;
                    }
                }
                if( !inserted && newRequests.Size() < maxChannelChanges )
                {
                    newRequests.PutDataAtEnd( i );
                }
            }
        }
       
        END_PROFILE(g_app->m_profiler,  "HandleRequests" );    

        
        //
        // Start the highest priority new requests

        START_PROFILE(g_app->m_profiler,  "StartNewSound" );    

        while( newRequests.Size() > 0 && 
               numChannelChanges < maxChannelChanges )
        {
            int channelIndex = newRequests[0];
            newRequests.RemoveData(0);
         
            SoundChannel *channel = &m_channels[channelIndex];          
            SoundInstance *currentSound = GetSoundInstance( channel->m_currentSound );            
            if( currentSound && !currentSound->m_loopType )
            {
                ShutdownSound( currentSound );
            }
            else if( currentSound )
            {
                currentSound->StopPlaying();
            }            
            channel->m_currentSound.SetInvalid();

            SoundInstance *requestedSound = GetSoundInstance( channel->m_requestedSound );
            bool success = requestedSound->StartPlaying( channelIndex );
            if( success )
            {
                channel->m_currentSound = channel->m_requestedSound;
            }
            else
            {
                // This is fairly bad, the sound failed to play
                // Which means it failed to load, or to go into a channel
                ShutdownSound( requestedSound );
            }
            channel->m_requestedSound.SetInvalid();

            g_soundLibrary3d->ResetChannel( channelIndex );
            numChannelChanges++;

#ifdef SOUNDSYSTEM_VERIFY
			RuntimeVerify();
#endif
        }

        END_PROFILE(g_app->m_profiler,  "StartNewSound" );    
        

        //
        // Advance all our channels
        // Clear out all the sound requests that have failed to start

        START_PROFILE(g_app->m_profiler,  "Advance Channels" );    

        for( int i = 0; i < m_numChannels; ++i )
        {            
            SoundChannel *channel = &m_channels[i];            

            SoundInstance *currentSound = GetSoundInstance( channel->m_currentSound );                         
            if( currentSound )
            {
                bool amIDone = currentSound->Advance();
                if( amIDone )
                {                    
                    ShutdownSound( currentSound );
                }    
            }

            SoundInstance *requestedSound = GetSoundInstance( channel->m_requestedSound );
            channel->m_requestedSound.SetInvalid();
            if(  requestedSound && 
                !requestedSound->m_loopType &&
                 requestedSound->m_restartAttempts <= 0 )
            {
                ShutdownSound( requestedSound );            
            }
        }

        END_PROFILE(g_app->m_profiler,  "Advance Channels" );    


        //
        // Recalculate all sound priorities
        // If we're attached to a bogus object then give up trying
        
        START_PROFILE(g_app->m_profiler,  "UpdatePriority" );    
        
        for( int i = 0; i < m_sounds.Size(); ++i )
        {
            if( m_sounds.ValidIndex(i) )
            {
                SoundInstance *instance = m_sounds[i];
                instance->RecalculatePriority();               
                
                if( instance->m_positionType == SoundInstance::Type3DAttachedToObject &&
                    !instance->GetAttachedObject() )
                {
                    ShutdownSound( instance );
                }
            }
        }

        END_PROFILE(g_app->m_profiler,  "UpdatePriority" );    


        //
        // If we're a looping sound and we're not playing, try to start us playing now
        // If we're not looping but still have restart attempts left, try to restart now

        START_PROFILE(g_app->m_profiler,  "Restart loops" );
        for( int i = 0; i < m_sounds.Size(); ++i )
        {
            if( m_sounds.ValidIndex(i) )
            {
                SoundInstance *instance = m_sounds[i];                
                {                
                    if( !instance->IsPlaying() )
                    {
                        if( !instance->m_loopType && instance->m_restartAttempts <= 0 )
                        {
                            ShutdownSound( instance );
                        }
                        else if( instance->m_loopType || instance->m_restartAttempts > 0 )
                        {
                            bool success = RequestSound( instance );
                            if( !success && !instance->m_loopType && instance->m_restartAttempts <= 0 )
                            {
                                ShutdownSound( instance );
                            }
                        }
                    }
                }
            }
        }
        END_PROFILE(g_app->m_profiler,  "Restart loops" );


        //
        // Update our listener position

        START_PROFILE(g_app->m_profiler, "UpdateListener" );    

        Vector3 camUp = g_app->m_camera->GetUp();
        if( g_prefsManager->GetInt("SoundSwapStereo",0) == 1 )
        {
            camUp.y *= -1.0f;
        }

        g_soundLibrary3d->SetListenerPosition( g_app->m_camera->GetPos(), 
                                            g_app->m_camera->GetFront(),
                                            camUp,
                                            g_app->m_camera->GetVel() );
        
        END_PROFILE(g_app->m_profiler, "UpdateListener" );    


        //
        // Advance our sound library

        START_PROFILE(g_app->m_profiler, "SoundLibrary3d Commit" );
        g_soundLibrary3d->CommitChanges();
        END_PROFILE(g_app->m_profiler, "SoundLibrary3d Commit" );
        START_PROFILE(g_app->m_profiler, "SoundLibrary3d Advance" );
        g_soundLibrary3d->Advance();
        END_PROFILE(g_app->m_profiler, "SoundLibrary3d Advance" );
        

#ifdef SOUNDSYSTEM_VERIFY
        RuntimeVerify();
#endif

        END_PROFILE(g_app->m_profiler, "Advance SoundSystem");   
    }
}
*/


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
        DarwiniaDebugAssert( !currentSound || 
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
                        DarwiniaDebugAssert( !(id1 == id2) );
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
    DarwiniaReleaseAssert( !errorFound, "Errors found in sounds.txt : refer to sounderrors.txt for details" );

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
    for( int i = 0; i < m_sounds.Size(); ++i )
    {
        if( m_sounds.ValidIndex(i) )
        {
            SoundInstance *instance = m_sounds[i];
            instance->PropagateBlueprints();
        }
    }
}
    

SampleGroup *SoundSystem::GetSampleGroup ( char *_name )
{
    for( int i = 0; i < m_sampleGroups.Size(); ++i )
    {
        if( m_sampleGroups.ValidIndex(i) )
        {
            SampleGroup *group = m_sampleGroups[i];
            if( strcmp( group->m_name, _name ) == 0 )
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

