#include "lib/universal_include.h"
#include "lib/binary_stream_readers.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/random.h"
#include "lib/preferences.h"

#include "sound/sample_cache.h"
#include "sound/sound_instance.h"
#include "sound/soundsystem.h"
#include "sound/sound_stream_decoder.h"
#include "sound/sound_library_3d.h"

#include "app.h"
#include "camera.h"
#include "location.h"


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
        g_app->m_soundSystem->m_filterBlueprints[ m_type ]->GetParameter( i, NULL, NULL, NULL, &dataType );
        switch( dataType )
        {
            case 0 :        *((float *) &params[i]) = (float) m_params[i].GetOutput();        break;
            case 1 :        *((long *) &params[i])  = (long) m_params[i].GetOutput();         break;
            default:
                DarwiniaReleaseAssert( false, "Unknown datatype" );
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
	m_cachedSampleHandle(NULL),
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
	g_deletingCachedSampleHandle = true;
	delete m_cachedSampleHandle;	m_cachedSampleHandle = NULL;
	g_deletingCachedSampleHandle = false;

	m_dspFX.EmptyAndDelete();

	free(m_eventName);

    m_objIds.EmptyAndDelete();
}


void SoundInstance::SetSoundName( char const *_name )
{
    if( _name ) strcpy( m_soundName, _name );
}


void SoundInstance::SetEventName( char const *_entityName, char const *_eventName )
{
	DarwiniaDebugAssert(m_eventName == NULL);
    DarwiniaDebugAssert(_entityName && _eventName);
	DarwiniaDebugAssert(g_app->m_soundSystem);

	m_eventName = (char*)malloc(strlen(_entityName) + strlen(_eventName) + 2);
	strcpy(m_eventName, _entityName);
	strcat(m_eventName, " ");
	strcat(m_eventName, _eventName);
}


char *SoundInstance::GetPositionTypeName( int _type )
{
    static char *types[] = {    "Type2D",
                                "Type3DStationary",
                                "Type3DAttachedToObject",
                                "TypeInEditor"
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

	DarwiniaDebugAssert(_copyMe->m_eventName);
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

    bool dspEnabled = g_prefsManager->GetInt( "SoundDSP", 1 );

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


void SoundInstance::PropagateBlueprints()
{
    //if( m_loopType && m_parent )
    {
        //
        // Do we need to restart the sound from scratch?

        bool restartRequired = false;

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
			g_deletingCachedSampleHandle = true;
            delete m_cachedSampleHandle;	m_cachedSampleHandle = NULL;
			g_deletingCachedSampleHandle = false;
        }


        //
        // Update all parameters

        if( m_minDistance != m_parent->m_minDistance )
        {
            m_minDistance = m_parent->m_minDistance;
            if( IsPlaying() )
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
            DarwiniaDebugAssert( m_dspFX.ValidIndex(i) );
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

            if( m_sustain.GetOutput() > 0.0f &&
                GetHighResTime() >= m_adsrTimer + m_sustain.GetOutput() )
            {
                BeginRelease(false);
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
                NearlyEquals(m_loopDelayTimer, 0.0f) )       finished = true;
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
                g_app->m_soundSystem->TriggerDuplicateSound( this );
            }
            else
            {
                m_loopDelayTimer = GetHighResTime();
            }
        }
    }

    if( m_loopType == LoopedADSR &&
        m_adsrState == StateRelease &&
        _final )
    {
        m_loopDelayTimer = 0.0f;
    }
}


void SoundInstance::OpenStream( bool _keepCurrentStream )
{
    m_restartOccured = true;

    if( m_cachedSampleHandle &&
        (_keepCurrentStream || m_sourceType == Sample) )
    {
		m_cachedSampleHandle->Restart();
        return;
    }

	g_deletingCachedSampleHandle = true;
	delete m_cachedSampleHandle;
    m_cachedSampleHandle = NULL;
	g_deletingCachedSampleHandle = false;

	char *sampleName = m_soundName;
    if (m_sourceType == SampleGroupRandom)
    {
		SampleGroup *group = g_app->m_soundSystem->GetSampleGroup( m_soundName );
		DarwiniaReleaseAssert( group, "Failed to find Sample Group %s", m_soundName );
		int numSamples = group->m_samples.Size();

        int memoryUsage = g_prefsManager->GetInt( "SoundMemoryUsage", 1 );
        if( memoryUsage == 2 ) numSamples *= 0.5f;
        if( memoryUsage == 3 ) numSamples *= 0.25f;
        numSamples = max( numSamples, 1 );

        int sampleIndex = darwiniaRandom() % numSamples;
		sampleName = group->m_samples[ sampleIndex ];
    }

	m_cachedSampleHandle = g_cachedSampleManager.GetSample(sampleName);
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
                g_app->m_soundSystem->TriggerDuplicateSound( this );
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
        DarwiniaReleaseAssert( false, "AdvanceLoop called on SinglePlay sound?" );
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

	START_PROFILE(g_app->m_profiler, "StartPlaying");

    //
    // If we don't have our stream yet, load it now

	START_PROFILE(g_app->m_profiler, "OpenStream");
    OpenStream( false );
	END_PROFILE(g_app->m_profiler, "OpenStream");


    //
    // Set up our parameters

	START_PROFILE(g_app->m_profiler, "Set Parameters");
    m_channelIndex = _channelIndex;


    switch( m_positionType )
    {
        case Type2D:
        case TypeInEditor:
                            g_soundLibrary3d->SetChannel3DMode( m_channelIndex, 1 );       break;

        default:
                            g_soundLibrary3d->SetChannel3DMode( m_channelIndex, 0 );       break;
    }

    g_soundLibrary3d->SetChannelMinDistance( m_channelIndex, m_minDistance );
	END_PROFILE(g_app->m_profiler, "Set Parameters");

	START_PROFILE(g_app->m_profiler, "Set freq/vol/pos");
    UpdateParameter( m_freq );
    g_soundLibrary3d->SetChannelFrequency( m_channelIndex, m_cachedSampleHandle->m_cachedSample->m_freq * m_freq.GetOutput() );

    bool done = UpdateChannelVolume();

    Update3DPosition();
    g_soundLibrary3d->SetChannelPosition( m_channelIndex, m_pos, m_vel );
	END_PROFILE(g_app->m_profiler, "Set freq/vol/pos");


    //
    // Start our DSP effects

	START_PROFILE(g_app->m_profiler, "Setup DSP Effects");
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
	END_PROFILE(g_app->m_profiler, "Setup DSP Effects");

	END_PROFILE(g_app->m_profiler, "StartPlaying");
    return true;
}


bool SoundInstance::Advance()
{
    //
    // Update parameters

    bool amIDone = UpdateChannelVolume();
    if( amIDone ) return true;

    UpdateParameter( m_freq );
    g_soundLibrary3d->SetChannelFrequency( m_channelIndex, m_cachedSampleHandle->m_cachedSample->m_freq * m_freq.GetOutput() );

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
            WorldObject *obj = GetAttachedObject();
            if( !obj )
            {
                m_positionType = Type3DStationary;
                m_vel = g_zeroVector;
			    g_soundLibrary3d->SetChannelPosition( m_channelIndex, m_pos, m_vel );
                BeginRelease(true);
                return false;
            }

            m_pos = obj->m_pos;
            m_vel = obj->m_vel;

            if( obj->m_id.GetUnitId() == UNIT_BUILDINGS )
            {
                m_pos = ((Building *) obj)->m_centrePos;
            }

            g_soundLibrary3d->SetChannelPosition( m_channelIndex, m_pos, m_vel );
            return false;
        }

        case TypeInEditor:
        {
            Vector3 relativePos( g_app->m_soundSystem->m_editorPos.x,
                                 0.0f,
                                 g_app->m_soundSystem->m_editorPos.z );
            m_pos = relativePos;
            m_vel.Zero();

		    g_soundLibrary3d->SetChannelPosition( m_channelIndex, m_pos, m_vel );
            return false;
        }
    }

    return false;
}


void SoundInstance::ForceParameter( SoundParameter &_param, float value )
{
    switch( _param.m_link )
    {
        case SoundParameter::LinkedToHeightAboveGround:
        {
            float landHeight = 0.0f;
            if( g_app->m_location ) landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
            m_pos.y = landHeight + value;
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
            if( g_app->m_camera )
            {
                m_pos = g_app->m_camera->GetPos() +
                        g_app->m_camera->GetFront() * value;
            }
            break;
        }
    }
}


bool SoundInstance::UpdateParameter ( SoundParameter &_param )
{
    if( _param.m_updateType == SoundParameter::UpdateOnceEveryRestart &&
        !m_restartOccured )
    {
        return false;
    }

    Vector3 pos = m_pos;
    Vector3 vel = m_vel;
    if( m_positionType == Type2D )
    {
        pos = g_app->m_camera->GetPos();
        vel = g_app->m_camera->GetVel();
    }

    if( _param.m_type == SoundParameter::TypeLinked )
    {
        switch( _param.m_link )
        {
            case SoundParameter::LinkedToHeightAboveGround:
            {
                float landHeight = 0.0f;
                if( g_app->m_location ) landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
                _param.Recalculate( pos.y - landHeight );
                break;
            }

            case SoundParameter::LinkedToXpos:
                _param.Recalculate( pos.x );
                break;

            case SoundParameter::LinkedToYpos:
                _param.Recalculate( pos.y );
                break;

            case SoundParameter::LinkedToZpos:
                _param.Recalculate( pos.z );
                break;

            case SoundParameter::LinkedToVelocity:
                _param.Recalculate( vel.Mag() );
                break;

            case SoundParameter::LinkedToCameraDistance:
            {
                float camDist = 0.0f;
                if( g_app->m_camera )
                {
                    camDist = (g_app->m_camera->GetPos() - pos).Mag();
                }
                _param.Recalculate( camDist );
            }
        }
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

    for( int i = 0; i < g_app->m_soundSystem->m_numChannels; ++i )
    {
        SoundInstanceId soundId = g_app->m_soundSystem->m_channels[i];
        if( soundId == m_id )
        {
            g_app->m_soundSystem->m_channels[i].SetInvalid();
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
        m_channelIndex < g_app->m_soundSystem->m_numChannels &&
        g_app->m_soundSystem->m_channels[ m_channelIndex ] == m_id )
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
    else
    {
        float distance = ( m_pos - g_app->m_camera->GetPos() ).Mag();
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


WorldObject *SoundInstance::GetAttachedObject()
{
    if( m_positionType != Type3DAttachedToObject ) return NULL;

    WorldObject *obj = NULL;

    if( g_app->m_locationId != -1 )
    {
        //
        // We are linked to one or more world objects

        bool recalculateAttachedObject = false;

        if( !g_app->m_location->GetWorldObject( m_objId ) ) recalculateAttachedObject = true;
        if( m_instanceType == MonophonicNearest ) recalculateAttachedObject = true;
        if( m_instanceType == MonophonicRandom && m_restartOccured ) recalculateAttachedObject = true;

        if( recalculateAttachedObject )
        {
            //
            // Prune our objIds list of invalid EntityIds

            for( int i = 0; i < m_objIds.Size(); ++i )
            {
                WorldObjectId *id = m_objIds[i];
                WorldObject *obj = g_app->m_location->GetWorldObject( *id );
                if( !obj )
                {
                    m_objIds.RemoveData(i);
                    delete id;
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
                        m_objId = *m_objIds[0];
                        break;
                    }

                    case MonophonicRandom:
                    {
                        int index = darwiniaRandom() % m_objIds.Size();
                        m_objId = *m_objIds[index];
                        break;
                    }

                    case MonophonicNearest:
                    {
                        float nearest = 99999.9f;
                        for( int i = 0; i < m_objIds.Size(); ++i )
                        {
                            WorldObjectId *id = m_objIds[i];
                            WorldObject *obj = g_app->m_location->GetWorldObject( *id );
                            float distance = ( g_app->m_camera->GetPos() - obj->m_pos ).MagSquared();
                            if( distance < nearest )
                            {
                                nearest = distance;
                                m_objId = *id;
                            }
                        }
                        break;
                    }
                }
            }
        }

        obj = g_app->m_location->GetWorldObject( m_objId );
    }

    return obj;
}


char *SoundInstance::GetDescriptor()
{
    static char descriptor[256];

    char *looping = GetLoopTypeName( m_loopType );
    char const *inEditor = m_positionType == TypeInEditor ? " editor" : "       ";

    char priority[32];
    sprintf( priority, "%2.2f", m_calculatedPriority );

    char volume[32];
    sprintf( volume, "%2.1f", m_channelVolume );

    char fx[32];
    if( m_dspFX.Size() )
    {
        sprintf( fx, "fx%d", m_dspFX.Size() );
    }
    else
    {
        sprintf( fx, "   " );
    }

    sprintf( descriptor, "%-18s  pri%5s  vol%4s   %s  %-8s  %s", m_soundName, priority, volume, fx, looping, inEditor );
    return descriptor;
}
