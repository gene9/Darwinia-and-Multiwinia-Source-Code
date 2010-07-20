#include "lib/universal_include.h"

#include "lib/debug_render.h"
#include "lib/file_writer.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/text_stream_readers.h"
#include "lib/text_renderer.h"

#include "app.h"
#include "location.h"
#include "entity_grid.h"
#include "explosion.h"
#include "team.h"
#include "main.h"

#include "worldobject/anthill.h"
#include "worldobject/armyant.h"
#include "worldobject/darwinian.h"
#include "worldobject/spawnpoint.h"

#include "sound/soundsystem.h"


AntHill::AntHill()
:   Building(),
    m_objectiveTimer(0),
    m_spawnTimer(0),
    m_numAntsInside(0),
    m_numSpiritsInside(0),
    m_eggConvertTimer(0),
    m_health(100),
    m_unitId(-1),
    m_populationLock(-1),
    m_renderDamaged(false)
{
    m_type = TypeAntHill;

    SetShape( g_app->m_resource->GetShape( "anthill.shp" ) );
}


void AntHill::Initialise( Building *_template )
{
    Building::Initialise( _template );

    m_numAntsInside = ((AntHill *) _template)->m_numAntsInside;

    m_spawnTimer = GetHighResTime() + 5.0f;
}


void AntHill::Damage ( float _damage )
{
    Building::Damage( _damage );

    if( m_health > 0 )
    {
        int healthBandBefore = int(m_health / 20.0f);
        m_health += _damage;
        int healthBandAfter = int(m_health / 20.0f);

        if( healthBandAfter != healthBandBefore )
        {
            Matrix34 mat( m_front, g_upVector, m_pos );
            g_explosionManager.AddExplosion( m_shape, mat, 1.0f - (float)m_health/100.0f );
            g_app->m_soundSystem->TriggerBuildingEvent( this, "Damage" );
        }

        if( m_health <= 0 )
        {
            Matrix34 mat( m_front, g_upVector, m_pos );
            g_explosionManager.AddExplosion( m_shape, mat );

            int numSpirits = m_numAntsInside + m_numSpiritsInside;
            for( int i = 0; i < numSpirits; ++i )
            {
                Vector3 pos = m_pos;
	            float radius = syncfrand(20.0f);
	            float theta = syncfrand(M_PI * 2);
    	        pos.x += radius * sinf(theta);
	            pos.z += radius * cosf(theta);

                Vector3 vel = ( pos - m_pos );
                vel.SetLength( syncfrand(50.0f) );

                g_app->m_location->SpawnSpirit( pos, vel, m_id.GetTeamId(), WorldObjectId() );
            }

            g_app->m_soundSystem->TriggerBuildingEvent( this, "Explode" );
            m_health = 0;
        }
    }
}

void AntHill::Destroy( float _intensity )
{
	Building::Destroy( _intensity );
	m_health = 0;
}

bool AntHill::SearchingArea( Vector3 _pos )
{
    for( int i = 0; i < m_objectives.Size(); ++i )
    {
        AntObjective *obj = m_objectives[i];
        float distance = ( obj->m_pos - _pos ).Mag();
        if( distance < 20.0f ) return true;
    }

    return false;
}


bool AntHill::TargettedEntity( WorldObjectId _id )
{
    for( int i = 0; i < m_objectives.Size(); ++i )
    {
        AntObjective *obj = m_objectives[i];
        if( obj->m_targetId == _id ) return true;
    }

    return false;
}


bool AntHill::SearchForSpirits( Vector3 &_pos )
{
    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        if( g_app->m_location->m_spirits.ValidIndex(i) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
            float theDist = ( s->m_pos - m_pos ).Mag();

            if( theDist <= ANTHILL_SEARCHRANGE &&
                !SearchingArea( s->m_pos ) &&
                ( s->m_state == Spirit::StateBirth ||
                  s->m_state == Spirit::StateFloating ) )
            {
                _pos = s->m_pos;
                return true;
            }
        }
    }

    return false;
}


bool AntHill::SearchForDarwinians( Vector3 &_pos, WorldObjectId &_id )
{
    int numFound;
    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetEnemies( m_pos.x, m_pos.z, ANTHILL_SEARCHRANGE, &numFound, m_id.GetTeamId() );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity && entity->m_type == Entity::TypeDarwinian )
        {
            Darwinian *darwinian = (Darwinian *) entity;
            float theDist = ( entity->m_pos - m_pos ).Mag();

            if( theDist <= ANTHILL_SEARCHRANGE &&
                darwinian->m_state != Darwinian::StateCapturedByAnt &&
                !TargettedEntity( entity->m_id ) )
            {
                _pos = entity->m_pos;
                _id = entity->m_id;
                return true;
            }
        }
    }

    return false;
}


bool AntHill::SearchForEnemies( Vector3 &_pos, WorldObjectId &_id )
{
    int numFound;
    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetEnemies( m_pos.x, m_pos.z, ANTHILL_SEARCHRANGE, &numFound, m_id.GetTeamId() );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        Entity *entity = g_app->m_location->GetEntity( id );

        float theDist = ( entity->m_pos - m_pos ).Mag();

        if( theDist <= ANTHILL_SEARCHRANGE &&
            !TargettedEntity( entity->m_id ) )
        {
            _pos = entity->m_pos;
            _id = entity->m_id;
            return true;
        }
    }

    return false;
}


bool AntHill::SearchForScoutArea ( Vector3 &_pos )
{
    Vector3 scoutPos = m_pos;
	float radius = ANTHILL_SEARCHRANGE/2.0f + syncfrand(ANTHILL_SEARCHRANGE/2.0f);
	float theta = syncfrand(M_PI * 2);
    scoutPos.x += radius * sinf(theta);
	scoutPos.z += radius * cosf(theta);
    scoutPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( scoutPos.x, scoutPos.z );

    if( scoutPos.y > 0 )
    {
        _pos = scoutPos;
        return true;
    }

    return false;
}


bool AntHill::PopulationLocked()
{
    //
    // If we haven't yet looked up a nearby Pop lock,
    // do so now

    if( m_populationLock == -1 )
    {
        m_populationLock = -2;

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building && building->m_type == TypeSpawnPopulationLock )
                {
                    SpawnPopulationLock *lock = (SpawnPopulationLock *) building;
                    float distance = ( building->m_pos - m_pos ).Mag();
                    if( distance < lock->m_searchRadius )
                    {
                        m_populationLock = lock->m_id.GetUniqueId();
                        break;
                    }
                }
            }
        }
    }


    //
    // If we are inside a Population Lock, query it now

    if( m_populationLock > 0 )
    {
        SpawnPopulationLock *lock = (SpawnPopulationLock *) g_app->m_location->GetBuilding( m_populationLock );
        if( lock && m_id.GetTeamId() != 255 &&
            lock->m_teamCount[ m_id.GetTeamId() ] >= lock->m_maxPopulation )
        {
            return true;
        }
    }


    return false;
}


bool AntHill::Advance()
{
    Building::Advance();

    //
    // Is the world awake yet ?

    if( !g_app->m_location ) return false;
    if( !g_app->m_location->m_teams ) return false;
    if( g_app->m_location->m_teams[ m_id.GetTeamId() ].m_teamType != Team::TeamTypeCPU ) return false;


    bool popLocked = PopulationLocked();

    //
    // Is it time to look for a new objective?

    if( !popLocked && GetHighResTime() > m_objectiveTimer && m_objectives.Size() < 3 )
    {
        Vector3 targetPos;
        WorldObjectId targetId;
        bool targetFound = false;

        if( !targetFound )      targetFound = SearchForDarwinians   ( targetPos, targetId );
        if( !targetFound )      targetFound = SearchForEnemies      ( targetPos, targetId );
        if( !targetFound )      targetFound = SearchForSpirits      ( targetPos );
        if( !targetFound )      targetFound = SearchForScoutArea    ( targetPos );

        if( targetFound )
        {
            AntObjective *objective = new AntObjective();
            objective->m_pos = targetPos;
            objective->m_targetId = targetId;

            objective->m_numToSend = 5 + 5 * (g_app->m_difficultyLevel / 10.0);
            m_objectives.PutData( objective );
        }

        m_objectiveTimer = GetHighResTime() + syncrand() % 5;
    }


    //
    // Send out ants to our existing objectives

    if( !popLocked &&
        m_objectives.Size() > 0 &&
        GetHighResTime() > m_spawnTimer &&
        m_numAntsInside > 0 )
    {

        Unit *unit = g_app->m_location->GetUnit( WorldObjectId( m_id.GetTeamId(), m_unitId, -1, -1 ) );
        if( !unit )
        {
            unit = g_app->m_location->m_teams[m_id.GetTeamId()].NewUnit( Entity::TypeArmyAnt, m_numAntsInside, &m_unitId, m_pos );
        }

        int chosenIndex = syncrand() % m_objectives.Size();
        AntObjective *objective = m_objectives[chosenIndex];
        Vector3 spawnPos = m_pos;
        spawnPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( spawnPos.x, spawnPos.z );

        WorldObjectId spawnedId = g_app->m_location->SpawnEntities( spawnPos, m_id.GetTeamId(), m_unitId, Entity::TypeArmyAnt, 1, g_zeroVector, 10.0f );
        ArmyAnt *ant = (ArmyAnt *) g_app->m_location->GetEntity( spawnedId );

        ant->m_buildingId = m_id.GetUniqueId();
        ant->m_front = ( ant->m_pos - m_pos ).Normalise();
        ant->m_orders = ArmyAnt::ScoutArea;
        ant->m_wayPoint = objective->m_pos;
        ant->m_targetId = objective->m_targetId;
	    float radius = syncfrand(30.0f);
	    float theta = syncfrand(M_PI * 2);
    	ant->m_wayPoint.x += radius * sinf(theta);
	    ant->m_wayPoint.z += radius * cosf(theta);
        ant->m_wayPoint = ant->PushFromObstructions( ant->m_wayPoint );
        ant->m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( ant->m_wayPoint.x, ant->m_wayPoint.z );

        m_numAntsInside--;

        objective->m_numToSend--;
        if( objective->m_numToSend <= 0 )
        {
            m_objectives.RemoveData( chosenIndex );
        }

        m_spawnTimer = GetHighResTime() + 0.2f - (0.2 * g_app->m_difficultyLevel / 10.0);
    }


    //
    // Convert spirits into ants

    if( m_numSpiritsInside > 0 &&
        GetHighResTime() > m_eggConvertTimer )
    {
        m_numSpiritsInside--;
        m_numAntsInside += 3;

        m_eggConvertTimer = GetHighResTime() + 5.0f;
    }


    //
    // Flicker if we are damaged

    float healthFraction = (float) m_health / 100.0f;
    float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
    m_renderDamaged = ( frand(0.75f) * (1.0f - fabs(sinf(timeIndex))*1.2f) > healthFraction );


    return ( m_health <= 0 );
}


void AntHill::Render( float _predictionTime )
{
	Matrix34 mat(m_front, m_up, m_pos);

    //
    // If we are damaged, flicked in and out based on our health

    if( m_renderDamaged )
    {
        float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
        float thefrand = frand();
        if      ( thefrand > 0.7f ) mat.f *= ( 1.0f - sinf(timeIndex) * 0.7f );
        else if ( thefrand > 0.4f ) mat.u *= ( 1.0f - sinf(timeIndex) * 0.5f );
        else                        mat.r *= ( 1.0f - sinf(timeIndex) * 0.7f );
        glEnable( GL_BLEND );
        glBlendFunc( GL_ONE, GL_ONE );
    }

    m_shape->Render(_predictionTime, mat);

    glDisable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );


#ifdef DEBUG_RENDER_ENABLED
    //g_editorFont.DrawText3DCentre( m_pos + Vector3(0,140,0), 20, "Health %d", m_health );
    //g_editorFont.DrawText3DCentre( m_pos + Vector3(0,120,0), 20, "%d Ants Inside", m_numAntsInside );
    //g_editorFont.DrawText3DCentre( m_pos + Vector3(0,100,0), 20, "%d Spirits Inside", m_numSpiritsInside );
//    for( int i = 0; i < m_objectives.Size(); ++i )
//    {
//        AntObjective *objective = m_objectives[i];
//        RenderSphere( objective->m_pos, 5.0f );
//    }
#endif
}


void AntHill::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "Explode" );
}


void AntHill::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_numAntsInside = atoi( _in->GetNextToken() );

    if( m_numAntsInside < 0 || m_numAntsInside > 10000 )
    {
        DebugOut( "Error loading Anthill : Bogus population of %d\n", m_numAntsInside );
        m_numAntsInside = 0;
    }
}


void AntHill::Write ( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%d", m_numAntsInside );
}
