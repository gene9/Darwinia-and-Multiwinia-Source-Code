#include "lib/universal_include.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/math/math_utils.h"
#include "lib/math/random_number.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/profiler.h"
#include "lib/preferences.h"

#include "sound_sample_bank.h"
#include "sound_instance.h"
#include "soundsystem.h"
#include "sound_sample_decoder.h"
#include "sound_library_3d.h"
#include "sound_blueprint_manager.h"


// ============================================================================
// class DspHandle

DspHandle::DspHandle()
:   m_type(-1),
    m_parent(NULL)
{
}


void DspHandle::Copy( DspHandle *_copyMe )
{
    m_type = _copyMe->m_type;

    for( int i = 0; i < MAX_PARAMS; ++i )
    {
        SoundParameter param = _copyMe->m_params[i];
        m_params[i] = param;
        m_parent->UpdateParameter( m_params[i] );
    }
}


void DspHandle::Initialise( SoundInstance *_parent )
{
    m_parent = _parent;
}


void DspHandle::Advance()
{
    //
    // Build a parameter list

    float params[MAX_PARAMS];
    
    for( int i = 0; i < MAX_PARAMS; ++i )
    {
        m_parent->UpdateParameter( m_params[i] );
        int dataType;
        DspBlueprint *dspBlueprint = g_soundSystem->m_blueprints.GetDspBlueprint(m_type);
        AppAssert(dspBlueprint);
        dspBlueprint->GetParameter( i, NULL, NULL, NULL, &dataType );
        switch( dataType )
        {
            case 0 :        *((float *) &params[i]) = (float) m_params[i].GetOutput();        break;
            case 1 :        *((long *) &params[i])  = (long) m_params[i].GetOutput();         break;
            default:
                AppReleaseAssert( false, "Unknown datatype" );
        }
    }
   

    //
    // Update the DSP effect

    g_soundLibrary3d->UpdateDspFX( m_parent->m_channelIndex, m_type, MAX_PARAMS, params );
}



// ============================================================================
// class SoundInstanceId

int SoundInstanceId::m_nextUniqueId = 0;

SoundInstanceId::SoundInstanceId()
:   m_index(-1),
    m_uniqueId(-1)
{
}


int SoundInstanceId::GenerateUniqueId()
{
    ++m_nextUniqueId;
    return m_nextUniqueId;
}


void SoundInstanceId::SetInvalid()
{
    m_index = -1;
    m_uniqueId = -1;
}


bool SoundInstanceId::operator == (SoundInstanceId const &w) const
{
	return ( m_index == w.m_index &&
             m_uniqueId == w.m_uniqueId );
}


SoundInstanceId const &SoundInstanceId::operator = (SoundInstanceId const &w)
{
    m_index = w.m_index;
    m_uniqueId = w.m_uniqueId;
    return *this;
}


// ============================================================================
// class SoundInstance

SoundInstance::SoundInstance()
:   m_positionType(Type3DAttachedToObject),
    m_instanceType(Polyphonic),
    m_channelIndex(-1),
    m_loopType(SinglePlay),
    m_sourceType(Sample),
	m_soundSampleHandle(NULL),
    m_minDistance(100.0f),
    m_calculatedPriority(128.0f),
    m_channelVolume(0.0f),
    m_parent(NULL),
    m_adsrTimer(0.0f),
    m_adsrState(StateAttack),
    m_loopDelayTimer(0.0f),
    m_eventName(NULL),
    m_restartOccured(true),
    m_restartAttempts(0)
{
    SetSoundName( "[???]" );

    m_freq.m_outputLower = 1.0f;
    m_freq.Recalculate();

    m_volume.m_outputLower = 7.0f;
    m_volume.Recalculate();    
    m_attack.Recalculate();
    m_sustain.Recalculate();
    m_release.Recalculate();
    m_loopDelay.Recalculate();
    
    m_adsrState = StateAttack;
    m_adsrTimer = GetHighResTime();
}


SoundInstance::~SoundInstance()
{
    if( m_soundSampleHandle &&
        m_soundSampleHandle->m_soundSample &&
        m_soundSampleHandle->m_soundSample->m_numChannels == 2 &&
        g_soundSystem->NumInstancesPlaying(m_sampleName) == 0 )
    {
        // Its a stereo sample and we are done with it,
        // and nobody else is playing it,
        // so delete the cache to save memory        

        g_soundSampleBank->DiscardSample( m_sampleName );
    }

	delete m_soundSampleHandle;
    m_soundSampleHandle = NULL;
	
	m_dspFX.EmptyAndDelete();

	free(m_eventName);

    m_objIds.Empty();
}


void SoundInstance::SetSoundName( char const *_name )
{
    if( _name ) strcpy( m_soundName, _name );
}


void SoundInstance::SetEventName( char const *_objectType, char const *_eventName )
{
	AppDebugAssert(m_eventName == NULL);
    AppDebugAssert(_objectType && _eventName);
	AppDebugAssert(g_soundSystem);

	m_eventName = new char [strlen(_objectType) + strlen(_eventName) + 2];
	strcpy(m_eventName, _objectType);
	strcat(m_eventName, " ");
	strcat(m_eventName, _eventName);
}


char *SoundInstance::GetPositionTypeName( int _type )
{
    static char *types[] = {    "2D", 
                                "3DStationary",
                                "3DAttachedToObject",
                                "InEditor",
                                "Music",
                                "3DRandom"
                                };

    if( _type >= 0 && _type < NumPositionTypes )
    {
        return types[_type];
    }
    
    return NULL;
}


char *SoundInstance::GetInstanceTypeName( int _type )
{
    static char *types[] = {    "Polyphonic",
                                "MonophonicRandom",
                                "MonophonicNearest"
                            };

    if( _type >= 0 && _type < NumInstanceTypes )
    {
        return types[_type];
    }

    return NULL;
}


char *SoundInstance::GetLoopTypeName( int _type )
{
    static char *types[] = {    "PlayOnce",
                                "Loop",
                                "LoopADSR"
                                };

    if( _type >= 0 && _type < NumLoopTypes )
    {
        return types[ _type ];
    }

    return NULL;
}


char *SoundInstance::GetSourceTypeName( int _type )
{
    static char *types[] = {    "Sample",
                                "SampleGroupRandom"
                                };
    
    if( _type >= 0 && _type < NumSourceTypes )
    {
        return types[ _type ];
    }

    return NULL;
}


void SoundInstance::Copy( SoundInstance *_copyMe )
{
    SetSoundName    ( _copyMe->m_soundName );
    m_positionType  = _copyMe->m_positionType;
    m_instanceType  = _copyMe->m_instanceType;
    m_loopType      = _copyMe->m_loopType;
    m_sourceType    = _copyMe->m_sourceType;
    m_volume        = _copyMe->m_volume;
    m_minDistance   = _copyMe->m_minDistance;

	AppDebugAssert(_copyMe->m_eventName);
	m_eventName = strdup(_copyMe->m_eventName);

    m_freq.Copy( &_copyMe->m_freq );
    UpdateParameter( m_freq );

    m_volume.Copy( &_copyMe->m_volume );
    UpdateParameter( m_volume );

    m_attack.Copy( &_copyMe->m_attack );
    UpdateParameter( m_attack );

    m_sustain.Copy( &_copyMe->m_sustain );
    UpdateParameter( m_sustain );

    m_release.Copy( &_copyMe->m_release );
    UpdateParameter( m_release );

    m_loopDelay.Copy( &_copyMe->m_loopDelay );
    UpdateParameter( m_loopDelay );
        
    bool dspEnabled = g_preferences->GetInt( "SoundDSP", 1 );

    if( dspEnabled )
    {
        for( int i = 0; i < _copyMe->m_dspFX.Size(); ++i )
        {
            DspHandle *effect = _copyMe->m_dspFX[i];
            DspHandle *copy = new DspHandle();
            copy->m_parent = this;
            copy->Copy( effect );
            m_dspFX.PutData( copy );
        }
    }

    m_parent = _copyMe;
}


void SoundInstance::PropagateBlueprints( bool _forceRestart )
{    
    //if( m_loopType && m_parent )
    {
        //
        // Do we need to restart the sound from scratch?

        bool restartRequired = _forceRestart;

        if( m_parent->m_dspFX.Size() != m_dspFX.Size() )
        {
            restartRequired = true;
        }
        else
        {
            for( int i = 0; i < m_parent->m_dspFX.Size(); ++i )
            {
                DspHandle *newEffect = m_parent->m_dspFX[i];
                DspHandle *ourEffect = m_dspFX[i];
                if( newEffect->m_type != ourEffect->m_type )
                {
                    restartRequired = true;
                    break;
                }
            }
        }
        
        if( strcmp( m_parent->m_soundName, m_soundName ) != 0 )
        {
            strcpy( m_soundName, m_parent->m_soundName );
            restartRequired = true;
        }

        if( restartRequired )
        {
            StopPlaying();
            delete m_soundSampleHandle;	
            m_soundSampleHandle = NULL;			
        }


        //
        // Update all parameters

        if( m_minDistance != m_parent->m_minDistance )
        {
            m_minDistance = m_parent->m_minDistance;
            if( IsPlaying() && m_positionType != TypeMusic ) 
            {
                g_soundLibrary3d->SetChannelMinDistance( m_channelIndex, m_minDistance );
            }
        }

        m_loopType = m_parent->m_loopType;                    
        m_sourceType = m_parent->m_sourceType;
        m_instanceType = m_parent->m_instanceType;

        m_volume.Copy( &m_parent->m_volume );
        UpdateParameter( m_volume );

        m_freq.Copy( &m_parent->m_freq );
        UpdateParameter( m_freq );

        m_attack.Copy( &m_parent->m_attack );
        UpdateParameter( m_attack );

        m_sustain.Copy( &m_parent->m_sustain );
        UpdateParameter( m_sustain );

        m_release.Copy( &m_parent->m_release );
        UpdateParameter( m_release );

        m_loopDelay.Copy( &m_parent->m_loopDelay );
        UpdateParameter( m_loopDelay );


        //
        // Make sure we have the same number of effects

        while( m_parent->m_dspFX.Size() > m_dspFX.Size() )
        {
            DspHandle *newEffect = new DspHandle();
            newEffect->m_parent = this;
            m_dspFX.PutData( newEffect );
        }
        while( m_dspFX.Size() > m_parent->m_dspFX.Size() )
        {
            DspHandle *effect = m_dspFX.GetData( m_dspFX.Size()-1 );
            m_dspFX.RemoveData( m_dspFX.Size()-1 );
            delete effect;
        }


        //
        // Update Filters

        for( int i = 0; i < m_parent->m_dspFX.Size(); ++i )
        {
            AppDebugAssert( m_dspFX.ValidIndex(i) );
            DspHandle *effect = m_parent->m_dspFX[i];
            DspHandle *copy = m_dspFX[i];
            copy->Copy( effect );            
        }        
    }
}


bool SoundInstance::UpdateChannelVolume()
{
    UpdateParameter( m_attack );
    UpdateParameter( m_sustain );
    UpdateParameter( m_release );
    UpdateParameter( m_volume );

    float timeSince = GetHighResTime() - m_adsrTimer;
    float volume = 0.0f;
    bool finished = false;
    
    switch( m_adsrState )
    {
        case StateAttack:
        {
            float fractionIntoAttack = 1.0f;
            if( m_attack.GetOutput() > 0.0f )       fractionIntoAttack = timeSince / m_attack.GetOutput();            
            if( fractionIntoAttack > 1.0f )         fractionIntoAttack = 1.0f;
            volume = m_volume.GetOutput() * fractionIntoAttack;
            if( NearlyEquals(fractionIntoAttack, 1.0f) )
            {
                m_adsrTimer = GetHighResTime();
                m_adsrState = StateSustain;
            }
            break;
        }

        case StateSustain:
        {
            volume = m_volume.GetOutput();
            float sustain = m_sustain.GetOutput();

            if( sustain > 0.0f && 
                GetHighResTime() >= m_adsrTimer + sustain )
            {
                BeginRelease(false);
            }

            if( sustain < 0.0f )
            {
                // Sustain < 0 means begin the release phase when this much time remains in the sample
                // eg sustain = -30 : begin Release when 30 seconds remain of the current sample

                float timeLeft = GetTimeRemaining();
                if( timeLeft <= fabs(sustain) )
                {
                    BeginRelease(false);
                }
            }

            break;
        }

        case StateRelease:
        {
            float fractionIntoRelease = 1.0f;
            if( m_release.GetOutput() > 0.0f )      fractionIntoRelease = timeSince / m_release.GetOutput();
            if( fractionIntoRelease > 1.0f )        fractionIntoRelease = 1.0f;

            volume = m_volume.GetOutput() * ( 1.0f - fractionIntoRelease );
            if( NearlyEquals(fractionIntoRelease, 1.0f) &&
                NearlyEquals(m_loopDelayTimer, 0.0f) )      
            {
                finished = true; 
            }

            break;
        }
    }

    m_channelVolume = volume;
    m_channelVolume  = max( m_channelVolume , 0.0f );
    m_channelVolume  = min( m_channelVolume , 10.0f );

    g_soundLibrary3d->SetChannelVolume( m_channelIndex, m_channelVolume );

    return finished;
}


void SoundInstance::BeginRelease( bool _final )
{   
    if( m_adsrState != StateRelease )
    {
        m_adsrState = StateRelease;
        m_adsrTimer = GetHighResTime();

        if( m_loopType == LoopedADSR && !_final )
        {
            if( NearlyEquals(m_loopDelay.GetOutput(), 0.0f) )
            {
                g_soundSystem->TriggerDuplicateSound( this );                
            }
            else
            {
                m_loopDelayTimer = GetHighResTime();
            }
        }
    }

    if( m_loopType == LoopedADSR && _final )
    {
        m_loopDelayTimer = 0.0f;
    }
}


void SoundInstance::OpenStream( bool _keepCurrentStream )
{
    m_restartOccured = true;
    
    if( m_soundSampleHandle &&
        (_keepCurrentStream || m_sourceType == Sample) )
    {
		m_soundSampleHandle->Restart();
        return;
    }

	delete m_soundSampleHandle; 	
    m_soundSampleHandle = NULL;

	char *sampleName = m_soundName;
    if (m_sourceType == SampleGroupRandom)
    {
		SampleGroup *group = g_soundSystem->m_blueprints.GetSampleGroup( m_soundName );
		AppReleaseAssert( group, "Failed to find Sample Group %s", m_soundName );
		int numSamples = group->m_samples.Size();
        
        int memoryUsage = g_preferences->GetInt( "SoundMemoryUsage", 1 );
        if( memoryUsage == 2 ) numSamples *= 0.5f;
        if( memoryUsage == 3 ) numSamples *= 0.25f;
        numSamples = max( numSamples, 1 );

        int sampleIndex = AppRandom() % numSamples;
		sampleName = group->m_samples[ sampleIndex ];
    }
	
    strcpy( m_sampleName, sampleName );
	m_soundSampleHandle = g_soundSampleBank->GetSample(m_sampleName);
}


bool SoundInstance::IsPlayable()
{
    if( m_sourceType == SampleGroupRandom )
    {
		SampleGroup *group = g_soundSystem->m_blueprints.GetSampleGroup( m_soundName );
		if( !group )
		{
			return false;
		}
	}

	return true;
}


bool SoundInstance::AdvanceLoop()
{
    UpdateParameter( m_loopDelay );

    if( m_loopType == LoopedADSR )
    {
        if( m_loopDelayTimer > 0.0f )
        {
            float loopFinish = m_loopDelayTimer + m_loopDelay.GetOutput();
            if( GetHighResTime() >= loopFinish )
            {
                g_soundSystem->TriggerDuplicateSound( this );
                m_loopDelayTimer = 0.0f;
            }
        }        

        OpenStream( true );
        return true;    
    }
    else if( m_loopType == Looped )
    {
        if( NearlyEquals(m_loopDelayTimer, 0.0f) )
        {
            // We have just come to the end of our sample
            // and are beginning a new loop phase
            if( NearlyEquals(m_loopDelay.GetOutput(), 0.0f) )
            {
                OpenStream( false );
                return true;
            }
            else
            {
                m_loopDelayTimer = GetHighResTime();
                return false;
            }    
        }
        else
        {
            float loopFinish = m_loopDelayTimer + m_loopDelay.GetOutput();
            if( GetHighResTime() >= loopFinish )
            {
                OpenStream( false );
                m_loopDelayTimer = 0.0f;
                return true;
            }

            return false;
        }
    }
    else
    {
        AppReleaseAssert( false, "AdvanceLoop called on SinglePlay sound?" );
        return false;
    }
}


bool SoundInstance::StartPlaying( int _channelIndex )
{
    if( strstr( m_soundName, "???" ) )
    {
        // This is a new sound that hasn't yet had its properties set
        return false;
    }

	START_PROFILE("StartPlaying");
		
    //
    // If we don't have our stream yet, load it now

	START_PROFILE("OpenStream");
    OpenStream( false );
	END_PROFILE("OpenStream");


    //
    // Set up our parameters

	START_PROFILE("Set Parameters");
    m_channelIndex = _channelIndex;

    
    switch( m_positionType )
    {
        case Type2D:
        case TypeInEditor:
        case Type3DRandom:
                            g_soundLibrary3d->SetChannel3DMode( m_channelIndex, 1 );       
                            break;

        case TypeMusic:
            break;

        default:            
                            g_soundLibrary3d->SetChannel3DMode( m_channelIndex, 0 );       
                            break;
    }


    if( m_positionType != TypeMusic )
    {
        g_soundLibrary3d->SetChannelMinDistance( m_channelIndex, m_minDistance );

        Update3DPosition();
        g_soundLibrary3d->SetChannelPosition( m_channelIndex, m_pos, m_vel );
    }

    
    UpdateParameter( m_freq );
    g_soundLibrary3d->SetChannelFrequency( m_channelIndex, 
                                           (float)m_soundSampleHandle->m_soundSample->m_freq * m_freq.GetOutput() );

    bool done = UpdateChannelVolume();

    END_PROFILE("Set Parameters");


    //
    // Start our DSP effects

	START_PROFILE("Setup DSP Effects");
    int filterTypes[16];
    int numActiveFilters = 0;
        
    for( int i = 0; i < m_dspFX.Size(); ++i )
    {
        DspHandle *effect = m_dspFX[i];
        effect->Initialise( this );            
        filterTypes[numActiveFilters] = effect->m_type;
        ++numActiveFilters;
    }                 
    
    if( numActiveFilters > 0 )
    {
        g_soundLibrary3d->EnableDspFX( m_channelIndex, numActiveFilters, filterTypes );
    }
	END_PROFILE("Setup DSP Effects");

	END_PROFILE("StartPlaying");
    return true;
}


bool SoundInstance::Advance()
{   
    //
    // Update parameters

    bool amIDone = UpdateChannelVolume();
    if( amIDone ) return true;

    UpdateParameter( m_freq );
    g_soundLibrary3d->SetChannelFrequency( m_channelIndex, 
                                           (float)m_soundSampleHandle->m_soundSample->m_freq * m_freq.GetOutput() );


    //if( m_loopDelayTimer > 0.0f ) AdvanceLoop();
    // This line was causing severe stuttering on sounds with ADSR and loopDelay > 0
    
    //
    // Update Filters

    for( int i = 0; i < m_dspFX.Size(); ++i )
    {
        DspHandle *effect = m_dspFX[i];
        effect->Advance();
    }

    
	//
    // Update 3d position
    
	bool rv = Update3DPosition();
    m_restartOccured = false;

	return rv;
}


bool SoundInstance::Update3DPosition()
{
    bool updateRequired = false;
    
    //
    // Work out our new position
    // And determine if we need to be updated
    
    switch( m_positionType )
    {
        case Type2D:
        case TypeMusic:
        {
            m_pos.Zero();
            m_vel.Zero();
            return false;
        }

        case Type3DStationary:
        {
            return false;
        }

        case Type3DAttachedToObject:
        {
            SoundObjectId id = GetAttachedObject();
            if( !id.IsValid() )
            {
                m_positionType = Type3DStationary;
                m_vel = Vector3<float>::ZeroVector();
			    g_soundLibrary3d->SetChannelPosition( m_channelIndex, m_pos, m_vel );
                BeginRelease(true);
                return false;
            }

            g_soundSystem->m_interface->GetObjectPosition( id, m_pos, m_vel );            
            g_soundLibrary3d->SetChannelPosition( m_channelIndex, m_pos, m_vel );
            return false;
        }

        case TypeInEditor:
        {
            Vector3<float> relativePos( g_soundSystem->m_editorPos.x,
										0.0f,
										g_soundSystem->m_editorPos.z );
            m_pos = relativePos;
            m_vel.Zero();

		    g_soundLibrary3d->SetChannelPosition( m_channelIndex, m_pos, m_vel );
            return false;
        }

        case Type3DRandom:
        {
            if( m_restartOccured )
            {
                float range = 500.0f;
                m_pos.Set( sfrand(range), 
                           sfrand(range),
                           sfrand(range) );
                m_vel.Zero();

		        g_soundLibrary3d->SetChannelPosition( m_channelIndex, m_pos, m_vel );
            }
            return false;
        }
    }

    return false;
}

 
void SoundInstance::ForceParameter( SoundParameter &_param, float value )
{
    
/*
    switch( _param.m_link )
    {
        case SoundParameter::LinkedToHeightAboveGround:
        {
//            float landHeight = 0.0f;
//            if( g_app->m_location ) landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
//            m_pos.y = landHeight + value;
            break;
        }       

        case SoundParameter::LinkedToXpos:
            m_pos.x = value;
            break;

        case SoundParameter::LinkedToYpos:
            m_pos.y = value;
            break;

        case SoundParameter::LinkedToZpos:
            m_pos.z = value;
            break;
            
        case SoundParameter::LinkedToVelocity:
            m_vel.Set( 0.0f, 0.0f, value );
            break;
    
        case SoundParameter::LinkedToCameraDistance:
        {
//            if( g_app->m_camera )
//            {
//                m_pos = g_app->m_camera->GetPos() + 
//                        g_app->m_camera->GetFront() * value;
//            }
            break;
        }
    }*/

}


bool SoundInstance::UpdateParameter ( SoundParameter &_param )
{
    if( _param.m_updateType == SoundParameter::UpdateOnceEveryRestart &&
        !m_restartOccured )
    {
        return false;
    }

    if( _param.m_type == SoundParameter::TypeLinked )
    {
        char *linkName = SoundParameter::GetLinkName(_param.m_link);
        float value = g_soundSystem->m_interface->GetPropertyValue( linkName, m_objId );              
        _param.Recalculate( value );            
    }
    else
    {
        _param.Recalculate();
    }

    return true;
}


void SoundInstance::StopPlaying()
{
    if( IsPlaying() )
    {
        if( m_dspFX.Size() > 0 )
        {            
            g_soundLibrary3d->DisableDspFX( m_channelIndex );
        }
       
        m_channelIndex = -1;
    }
    
    //
    // Make sure there aren't any channels that think they are 
    // playing our sound

    for( int i = 0; i < g_soundSystem->m_numChannels; ++i )
    {
        SoundInstanceId soundId = g_soundSystem->m_channels[i];
        if( soundId == m_id )
        {
            g_soundSystem->m_channels[i].SetInvalid();
        }
    }
}


bool SoundInstance::IsPlaying()
{
    return ( GetChannelIndex() != -1 );
}


int SoundInstance::GetChannelIndex()
{
    if( m_channelIndex == -1 )
    {
        return -1;
    }
        
    if( m_channelIndex >= 0 && 
        m_channelIndex < g_soundSystem->m_numChannels &&
        g_soundSystem->m_channels[ m_channelIndex ] == m_id )
    {
        return m_channelIndex;
    }
    
    return -1;
}


void SoundInstance::CalculatePerceivedVolume()
{
    if( m_positionType == TypeInEditor ||
        m_positionType == Type2D )
    {
        m_perceivedVolume = 10.0f;
    }
    else if( m_positionType == TypeMusic ||
             m_positionType == Type3DRandom )
    {
        if( IsPlaying() )   m_perceivedVolume = m_channelVolume;
        else                m_perceivedVolume = m_volume.GetOutput();
    }
    else
    {        
        Vector3<float> cameraPos;
        Vector3<float> unused;
        g_soundSystem->m_interface->GetCameraPosition( cameraPos, unused, unused, unused );

        float distance = ( m_pos - cameraPos ).Mag();
        
        float distanceFactor = 1.0f;
        if( distance > m_minDistance )
        {
            distanceFactor = m_minDistance / distance;    
        }        

        // NOTE : We could base this on m_channelVolume instead of m_volume,
        // in order to take into account ADSR automatically. However m_channelVolume
        // is only maintained if teh sound is actually playing, and we need to
        // calculate perceived volume for sounds that often aren't playing at all.

        // NOTE 2 : I've started using m_channelVolume for sounds that are playing.
        // This is because Alistair tends to set the Release of a sound to 10 seconds
        // if he has any effects on it, eg Echo, to allow the full Echo to continue
        // even after the sound has finished.  This results in our sound channels
        // being filled up with high priority sounds that have mostly faded out.
        
        if( IsPlaying() )   m_perceivedVolume = m_channelVolume;
        else                m_perceivedVolume = m_volume.GetOutput();
        m_perceivedVolume *= distanceFactor;
    }
}



SoundObjectId SoundInstance::GetAttachedObject()
{
    if( m_positionType != Type3DAttachedToObject ) return SoundObjectId();
        
    //
    // We are linked to one or more world objects

    bool recalculateAttachedObject = false;

    if( !g_soundSystem->m_interface->DoesObjectExist( m_objId ) ) recalculateAttachedObject = true;
    if( m_instanceType == MonophonicNearest ) recalculateAttachedObject = true;
    if( m_instanceType == MonophonicRandom && m_restartOccured ) recalculateAttachedObject = true;


    if( recalculateAttachedObject )
    {
        //
        // Prune our objIds list of invalid EntityIds

        for( int i = 0; i < m_objIds.Size(); ++i )
        {
            SoundObjectId id = *m_objIds.GetPointer(i);
            if( !g_soundSystem->m_interface->DoesObjectExist(id) )
            {
                m_objIds.RemoveData(i);
                --i;
            }
        }


        //
        // Now select an object from our list depending on our instance type

        if( m_objIds.Size() > 0 )
        {
            switch( m_instanceType )
            {
                case Polyphonic:
                {
                    m_objId = *m_objIds.GetPointer(0);
                    break;
                }

                case MonophonicRandom:
                {
                    int index = AppRandom() % m_objIds.Size();
                    m_objId = *m_objIds.GetPointer(index);
                    break;
                }

                case MonophonicNearest:
                {
                    float nearest = 99999.9f;                            
                    for( int i = 0; i < m_objIds.Size(); ++i )
                    {
                        SoundObjectId id = *m_objIds.GetPointer(i);
                        Vector3<float> objPos, objVel;
                        g_soundSystem->m_interface->GetObjectPosition( id, objPos, objVel );

                        Vector3<float> camPos, camVel, camUp, camFront;
                        g_soundSystem->m_interface->GetCameraPosition( camPos, camFront, camUp, camVel );

                        float distance = ( camPos - objPos ).MagSquared();
                        if( distance < nearest )
                        {
                            nearest = distance;
                            m_objId = id;
                        }
                    }
                    break;
                }
            }
        }
    }
        
    return m_objId;
}


float SoundInstance::GetTimeRemaining()
{
    if( !m_soundSampleHandle || !m_soundSampleHandle->m_soundSample )
    {
        return -1.0f;
    }

    int numSamplesRemaining = m_soundSampleHandle->NumSamplesRemaining();
    int numChannels = m_soundSampleHandle->m_soundSample->m_numChannels;
    int frequency = m_soundSampleHandle->m_soundSample->m_freq;

    float timeRemaining = (float)numSamplesRemaining / (float) frequency;
    timeRemaining /= (float) numChannels;
    
    return timeRemaining;
}


char *SoundInstance::GetDescriptor()
{
    static char descriptor[256];

    char *looping = GetLoopTypeName( m_loopType );
    char *positionType = GetPositionTypeName( m_positionType );       //m_positionType == TypeInEditor ? " editor" : "       ";
    
    if( strcmp( positionType, "3DAttachedToObject" ) == 0 )
    {
        positionType = "3DAttached";
    }

    char priority[32];
    sprintf( priority, "%2.2f", m_calculatedPriority );

    char volume[32];
    sprintf( volume, "%2.2f", g_soundLibrary3d->GetChannelVolume(m_channelIndex) );
    
    char fx[32];
    if( m_dspFX.Size() )
    {
        sprintf( fx, "fx%d", m_dspFX.Size() );
    }
    else
    {
        sprintf( fx, "   " );
    }

    float timeRemaining = GetTimeRemaining();

    char soundName[256];
    char sampleName[256];
    strcpy( soundName, m_soundName );
    strcpy( sampleName, m_sampleName );

    if( strlen(soundName) > 15 )
    {
        soundName[15] = '\x0';
        soundName[14] = '.';
        soundName[13] = '.';
    }

    if( strlen(sampleName) > 15 )
    {
        sampleName[15] = '\x0';
        sampleName[14] = '.';
        sampleName[13] = '.';
    }

    sprintf( descriptor, "%-15s %-15s %4s   %s  %-12s  %-14s %2.1fs", 
             soundName, sampleName, volume, fx, looping, positionType, timeRemaining );

    return descriptor;
}
