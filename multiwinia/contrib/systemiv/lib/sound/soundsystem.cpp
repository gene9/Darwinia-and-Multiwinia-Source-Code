#include "lib/universal_include.h"
#include "lib/tosser/btree.h"
#include "lib/profiler.h"
#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "lib/hi_res_time.h"
#include "lib/math/random_number.h"

#include "soundsystem.h"
#include "soundsystem_interface.h"
#include "sound_sample_bank.h"
#include "sound_library_2d.h"
#include "sound_library_3d.h"
#include "sound_sample_decoder.h"
#include "sound_blueprint_manager.h"

#ifdef WIN32
#include "sound_library_3d_dsound.h"
#else
#include "sound_library_2d_sdl.h"
#include "sound_library_3d_software.h"
#endif

#define SOUNDSYSTEM_UPDATEPERIOD    0.05f

//#define SOUNDSYSTEM_VERIFY                        // Define this to make the sound system
                                                    // Perform a series of (very slow)
                                                    // tests every frame for correctness

SoundSystem *g_soundSystem = NULL;


//*****************************************************************************
// Class SoundSystem
//*****************************************************************************

SoundSystem::SoundSystem()
:   m_timeSync(0.0f),
    m_channels(NULL),
    m_numChannels(0),
    m_numMusicChannels(0),
    m_interface(NULL),
    m_propagateBlueprints(false)
{
    AppSeedRandom( (unsigned int) GetHighResTime() );
}


SoundSystem::~SoundSystem()
{
    delete [] m_channels;

    m_sounds.EmptyAndDelete();

    delete g_soundLibrary3d;
    g_soundLibrary3d = NULL;
#ifndef TARGET_MSVC
	delete g_soundLibrary2d;
	g_soundLibrary2d = NULL;
#endif
}


void SoundSystem::Initialise( SoundSystemInterface *_interface )
{
    m_interface = _interface;

    g_soundSampleBank = new SoundSampleBank();

    m_blueprints.LoadEffects();
    m_blueprints.LoadBlueprints();

#ifndef TARGET_MSVC
	g_soundLibrary2d = NULL;
#endif
    RestartSoundLibrary();
}


void SoundSystem::RestartSoundLibrary()
{
    //
    // Shut down existing sound library

	delete [] m_channels;
	delete g_soundLibrary3d;
	g_soundLibrary3d = NULL;
#ifndef TARGET_MSVC
	delete g_soundLibrary2d;
	g_soundLibrary2d = NULL;
#endif

    //
    // Start up a new sound library

	int mixrate         = g_preferences->GetInt("SoundMixFreq", 22050);
	int volume          = g_preferences->GetInt("SoundMasterVolume", 255);
    m_numChannels       = g_preferences->GetInt("SoundChannels", 32);
    m_numMusicChannels  = g_preferences->GetInt("SoundMusicChannels", 12 );
    int hw3d            = g_preferences->GetInt("SoundHW3D", 0);
    char *libName       = g_preferences->GetString("SoundLibrary", "dsound");

#ifdef TARGET_MSVC
	g_soundLibrary3d = new SoundLibrary3dDirectSound();
#else
	g_soundLibrary2d = new SoundLibrary2dSDL();
	g_soundLibrary3d = new SoundLibrary3dSoftware();
#endif

    g_soundLibrary3d->SetMasterVolume( volume );
    g_soundLibrary3d->Initialise( mixrate, m_numChannels, m_numMusicChannels, hw3d );
    g_soundLibrary3d->SetDopplerFactor( 0.0f );

    m_numChannels = g_soundLibrary3d->GetNumChannels();
    m_numMusicChannels = g_soundLibrary3d->GetNumMusicChannels();
    m_channels = new SoundInstanceId[m_numChannels];    

    g_soundLibrary3d->SetMainCallback( &SoundLibraryMainCallback );
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


bool SoundSystem::IsMusicChannel( int _channelId )
{
    return( _channelId >= m_numChannels - m_numMusicChannels );
}


bool SoundSystem::SoundLibraryMainCallback( unsigned int _channel, signed short *_data, 
									        unsigned int _numSamples, int *_silenceRemaining )
{
    if( !g_soundSystem ) return false;

	SoundInstanceId soundId = g_soundSystem->m_channels[_channel];
    SoundInstance *instance = g_soundSystem->GetSoundInstance( soundId );
    bool stereo = g_soundSystem->IsMusicChannel(_channel);
    
    if( instance && instance->m_soundSampleHandle )
    {
		//
        // Fill the space with sample data

        float relFreq = g_soundLibrary3d->GetChannelRelFreq(_channel);
		int numSamplesWritten = instance->m_soundSampleHandle->Read( _data, _numSamples, stereo, relFreq);

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
                        unsigned int numWritten = instance->m_soundSampleHandle->Read( loopStart, numSamplesRemaining, stereo , relFreq);
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

		return true;
    }
    else
    {
        g_soundLibrary3d->WriteSilence( _data, _numSamples );
		return false;
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
                        SoundObjectId id = *_instance->m_objIds.GetPointer(j);
                        thisInstance->m_objIds.PutData( id );
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
        _instance->m_restartAttempts = 3;           
        return true;
    }
    else
    {
        return false;
    }
}


int SoundSystem::FindBestAvailableChannel( bool _music )
{
    int emptyChannelIndex = -1;                                         // Channel without a current sound or requested sound
    int lowestPriorityChannelIndex = -1;                                // Channel with very low priority
    float lowestPriorityChannel = 999999.9f;

    int lowestChannel = 0;
    int highestChannel = m_numChannels - m_numMusicChannels;

    if( _music )
    {
        lowestChannel = m_numChannels - m_numMusicChannels;
        highestChannel = m_numChannels;
    }

    for( int i = lowestChannel; i < highestChannel; ++i )
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


void SoundSystem::TriggerEvent( SoundObjectId _objId, char *_eventName )
{
    if( !m_channels ) return;

	START_PROFILE("TriggerEvent");
    
    char *objectType = m_interface->GetObjectType(_objId);
    if( objectType )
    {
        SoundEventBlueprint *seb = m_blueprints.GetBlueprint(objectType);
        if( seb )
        {
            for( int i = 0; i < seb->m_events.Size(); ++i )
            {
                SoundInstanceBlueprint *sib = seb->m_events[i];
                if( stricmp( sib->m_eventName, _eventName ) == 0 )
                {
                    Vector3<float> pos, vel;
                    m_interface->GetObjectPosition( _objId, pos, vel );

                    SoundInstance *instance = new SoundInstance();
                    instance->Copy( sib->m_instance );
                    instance->m_objIds.PutData( _objId );
                    instance->m_pos = pos;
                    instance->m_vel = vel;
                    bool success = InitialiseSound  ( instance );                
                    if( !success ) ShutdownSound    ( instance );                
                }
            }
        }
    }

	END_PROFILE("TriggerEvent");
}


void SoundSystem::TriggerEvent( char *_type, char *_eventName )
{
    TriggerEvent( _type, _eventName, Vector3<float>::ZeroVector() );
}


void SoundSystem::TriggerEvent( char *_type, char *_eventName, Vector3<float> const &_pos )
{
    if( !m_channels ) return;

	START_PROFILE("TriggerEvent");
    
    SoundEventBlueprint *seb = m_blueprints.GetBlueprint(_type);
    if( seb )
    {
        for( int i = 0; i < seb->m_events.Size(); ++i )
        {
            SoundInstanceBlueprint *sib = seb->m_events[i];
            if( stricmp( sib->m_eventName, _eventName ) == 0 )
            {
                SoundInstance *instance = new SoundInstance();
                instance->Copy( sib->m_instance );
                instance->m_pos = _pos;
                bool success = InitialiseSound  ( instance );                
                if( !success ) ShutdownSound    ( instance );                
            }
        }
    }

	END_PROFILE("TriggerEvent");
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
        SoundObjectId id = *_instance->m_objIds.GetPointer(i);
        newInstance->m_objIds.PutData( id );
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


void SoundSystem::StopAllSounds( SoundObjectId _id, char *_eventName )
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


int SoundSystem::NumInstancesPlaying( char *_sampleName )
{
    int result = 0;

    for( int i = 0; i < m_numChannels; ++i )
    {            
        SoundInstanceId soundId = m_channels[i];            
        SoundInstance *instance = GetSoundInstance(soundId);
        if( instance && 
            stricmp(instance->m_sampleName, _sampleName) == 0 )
        {
            ++result;
        }
    }   

    return result;
}


int SoundSystem::NumInstancesPlaying( SoundObjectId _id, char *_eventName )
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


int SoundSystem::NumInstances( SoundObjectId _id, char *_eventName )
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
    
    SoundInstance *instance1 = g_soundSystem->GetSoundInstance( id1 );
    SoundInstance *instance2 = g_soundSystem->GetSoundInstance( id2 );

    AppDebugAssert( instance1 );
    AppDebugAssert( instance2 );

    
    if      ( instance1->m_perceivedVolume < instance2->m_perceivedVolume )     return +1;
    else if ( instance1->m_perceivedVolume > instance2->m_perceivedVolume )     return -1;
    else                                                                        return 0;
}


void SoundSystem::Advance()
{
    if( !m_channels )
	{
		return;
	}

    float timeNow = GetHighResTime();
    if( timeNow >= m_timeSync )
    {
        m_timeSync = timeNow + SOUNDSYSTEM_UPDATEPERIOD;
        
        START_PROFILE("SoundSystem");        
		
        //
        // Resync with blueprints (changed by editor)

        START_PROFILE("Propagate Blueprints" );
        if( m_propagateBlueprints )
        {
            PropagateBlueprints();
        }
        END_PROFILE("Propagate Blueprints" );


        //
		// First pass : Recalculate all Perceived Sound Volumes
		// Throw away sounds that have had their chance
        // Build a list of instanceIDs for sorting

        START_PROFILE("Allocate Sorted Array" );
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
        END_PROFILE("Allocate Sorted Array" );

        START_PROFILE("Perceived Volumes" );
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
                         !instance->GetAttachedObject().IsValid() )                
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
        END_PROFILE("Perceived Volumes" );


        //        
		// Sort sounds into perceived volume order
        // NOTE : There are exactly numSortedId elements in sortedIds.  
        // NOTE : It isn't safe to assume numSortedIds == m_sounds.NumUsed()
        
        START_PROFILE("Sort Samples" );
        qsort( sortedIds, numSortedIds, sizeof(SoundInstanceId), SoundInstanceCompare );
        END_PROFILE("Sort Samples" );


        //
		// Second pass : Recalculate all Sound Priorities starting with the nearest sounds
        // Reduce priorities as more of the same sounds are played

        BTree<float> existingInstances;
        
                
        //                
        // Also look out for the highest priority new sound to swap in

        START_PROFILE("Recalculate Priorities" );

        LList<SoundInstance *> newInstances;
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

            if( !instance->IsPlaying() )
            {
                if( instance->m_positionType == SoundInstance::TypeMusic )
                {
                    newInstances.PutData( instance );
                }
                else if( instance->m_calculatedPriority > highestInstancePriority )
                {
                    newInstance = instance;
                    highestInstancePriority = instance->m_calculatedPriority;
                }
            }
        }

        if( newInstance )
        {
            newInstances.PutData( newInstance );
        }

        END_PROFILE("Recalculate Priorities" );
        
     
        for( int i = 0; i < newInstances.Size(); ++i )
        {            
            SoundInstance *newInstance = newInstances[i];
            bool isMusic = (newInstance->m_positionType == SoundInstance::TypeMusic);

            // Find worst old sound to get rid of  
            START_PROFILE("Find best Channel" );
            int bestAvailableChannel = FindBestAvailableChannel( isMusic );
            END_PROFILE("Find best Channel" );

			// FindBestAvailableChannel can return -1, so let's not access an invalid index later on.
			if ( bestAvailableChannel < 0 )
				continue;

            START_PROFILE("Stop Old Sound" );
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
            END_PROFILE("Stop Old Sound" );


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
            END_PROFILE("Start New Sound" );
            
            START_PROFILE("Reset Channel" );
            g_soundLibrary3d->ResetChannel( bestAvailableChannel);
            END_PROFILE("Reset Channel" );
        }


        //
        // Advance all sound channels

        START_PROFILE("Advance All Channels" );
        for( int i = 0; i < m_numChannels; ++i )
        {            
            SoundInstanceId soundId = m_channels[i];            
            SoundInstance *currentSound = GetSoundInstance( soundId );                         
            if( currentSound )
            {
                bool amIDone = currentSound->Advance();
                if( amIDone )
                {                    
			        START_PROFILE("Shutdown Sound" );
                    ShutdownSound( currentSound );
			        END_PROFILE("Shutdown Sound" );
                }    
            }
        }
        END_PROFILE("Advance All Channels" );


		//
        // Update our listener position

        START_PROFILE("UpdateListener" );    

        Vector3<float> camPos, camVel, camUp, camFront;
        bool cameraDefined = m_interface->GetCameraPosition( camPos, camFront, camUp, camVel );

        if( cameraDefined )
        {
            if( g_preferences->GetInt("SoundSwapStereo",0) == 0 )
            {
                camUp *= -1.0f;
            }

            g_soundLibrary3d->SetListenerPosition( camPos, camFront, camUp, camVel );
        }
        
        END_PROFILE("UpdateListener" );    


        //
        // Advance our sound library

        START_PROFILE("SoundLibrary3d Advance" );
        g_soundLibrary3d->Advance();
        END_PROFILE("SoundLibrary3d Advance" );
      
#ifdef SOUNDSYSTEM_VERIFY
        RuntimeVerify();
#endif

        END_PROFILE("SoundSystem");                    
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
    }
}


void SoundSystem::PropagateBlueprints( bool _forceRestart )
{
    for( int i = m_sounds.Size() - 1; i >= 0; --i )
    {
        if( m_sounds.ValidIndex(i) )
        {
            SoundInstance *instance = m_sounds[i];

			if( instance->IsPlayable() )
			{
				instance->PropagateBlueprints( _forceRestart );
			}
			else
			{
				ShutdownSound( instance );
			}
        }
    }
}
    

void SoundSystem::EnableCallback( bool _enabled )
{
	g_soundLibrary3d->EnableCallback(_enabled);
}
