#include "lib/universal_include.h"

#include "lib/debug_render.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/text_renderer.h"
#include "lib/math/random_number.h"

#include "particle_system.h"
#include "app.h"
#include "location.h"
#include "entity_grid.h"
#include "explosion.h"
#include "team.h"
#include "main.h"
#include "level_file.h"

#include "worldobject/anthill.h"
#include "worldobject/armyant.h"
#include "worldobject/darwinian.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/virii.h"

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
    m_renderDamaged(false),
    m_antInsideCheck(false)
{
    m_type = TypeAntHill;

    SetShape( g_app->m_resource->GetShape( "anthill.shp" ) );
}

AntHill::~AntHill()
{
	m_objectives.EmptyAndDelete();
}

void AntHill::Initialise( Building *_template )
{
    Building::Initialise( _template );

    m_numAntsInside = ((AntHill *) _template)->m_numAntsInside;

    m_spawnTimer = GetNetworkTime() + 5.0;
}


void AntHill::Damage ( double _damage )
{
    Building::Damage( _damage );

    if( m_health > 0 )
    {
        int healthBandBefore = int(m_health / 20.0);
        m_health += _damage;
        int healthBandAfter = int(m_health / 20.0);
        
        if( healthBandAfter != healthBandBefore )
        {
            Matrix34 mat( m_front, g_upVector, m_pos );
            g_explosionManager.AddExplosion( m_shape, mat, 1.0 - (double)m_health/100.0 );
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
	            double radius = syncfrand(20.0);
	            double theta = syncfrand(M_PI * 2);
    	        pos.x += radius * iv_sin(theta);
	            pos.z += radius * iv_cos(theta);

                Vector3 vel = ( pos - m_pos );
                vel.SetLength( syncfrand(50.0) );

                g_app->m_location->SpawnSpirit( pos, vel, m_id.GetTeamId(), WorldObjectId() );
            }
        
            g_app->m_soundSystem->TriggerBuildingEvent( this, "Explode" );
            m_health = 0;
        }
    }
}

void AntHill::Destroy( double _intensity )
{
	Building::Destroy( _intensity );
	m_health = 0;
}

bool AntHill::SearchingArea( Vector3 _pos )
{
    for( int i = 0; i < m_objectives.Size(); ++i )
    {
        AntObjective *obj = m_objectives[i];
        double distance = ( obj->m_pos - _pos ).Mag();
        if( distance < 20.0 ) return true;
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
            double theDist = ( s->m_pos - m_pos ).Mag();

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
    g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, m_pos.x, m_pos.z, ANTHILL_SEARCHRANGE, &numFound, m_id.GetTeamId() );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity && entity->m_type == Entity::TypeDarwinian )
        {
            Darwinian *darwinian = (Darwinian *) entity;
            double theDist = ( entity->m_pos - m_pos ).Mag();

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
    g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, m_pos.x, m_pos.z, ANTHILL_SEARCHRANGE, &numFound, m_id.GetTeamId() );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = g_app->m_location->GetEntity( id );

        double theDist = ( entity->m_pos - m_pos ).Mag();

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
	double radius = ANTHILL_SEARCHRANGE/2.0 + syncfrand(ANTHILL_SEARCHRANGE/2.0);
	double theta = syncfrand(M_PI * 2);
    scoutPos.x += radius * iv_sin(theta);
	scoutPos.z += radius * iv_cos(theta);    
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

    if( g_app->Multiplayer() )
    {
        if( g_app->m_location->m_levelFile->m_populationCap != -1 )
        {
            if( m_id.GetTeamId() == 255 ) return false;
            int numTeams = g_app->m_location->m_levelFile->m_numPlayers;
            int maxPopPerTeam = g_app->m_location->m_levelFile->m_populationCap / numTeams;
            Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
            int currentPop = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_others.NumUsed() - Virii::s_viriiPopulation[m_id.GetTeamId()];
            return( currentPop >= maxPopPerTeam );
        }
    }

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
                    double distance = ( building->m_pos - m_pos ).Mag();
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


void AntHill::SpawnAnt( Vector3 &waypoint, WorldObjectId targetId )
{
    Unit *unit = g_app->m_location->GetUnit( WorldObjectId( m_id.GetTeamId(), m_unitId, -1, -1 ) );
    if( !unit )
    {
        unit = g_app->m_location->m_teams[m_id.GetTeamId()]->NewUnit( Entity::TypeArmyAnt, m_numAntsInside, &m_unitId, m_pos );        
    }

    Vector3 spawnPos = m_pos;
    spawnPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( spawnPos.x, spawnPos.z );

    WorldObjectId spawnedId = g_app->m_location->SpawnEntities( spawnPos, m_id.GetTeamId(), m_unitId, Entity::TypeArmyAnt, 1, g_zeroVector, 10.0 );
    ArmyAnt *ant = (ArmyAnt *) g_app->m_location->GetEntity( spawnedId );

    ant->m_buildingId = m_id.GetUniqueId();
    ant->m_front = ( ant->m_pos - m_pos ).Normalise();
    ant->m_orders = ArmyAnt::ScoutArea;
    ant->m_wayPoint = waypoint;   
    ant->m_targetId = targetId;
    double radius = syncfrand(30.0);
    double theta = syncfrand(M_PI * 2);
    ant->m_wayPoint.x += radius * iv_sin(theta);
    ant->m_wayPoint.z += radius * iv_cos(theta);
    ant->m_wayPoint = ant->PushFromObstructions( ant->m_wayPoint );
    ant->m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( ant->m_wayPoint.x, ant->m_wayPoint.z );

    m_numAntsInside--;
}


bool AntHill::Advance()
{
    Building::Advance();

    //
    // Is the world awake yet ?

    if( !g_app->m_location ) return false;
    if( !g_app->m_location->m_teams ) return false;
    if( g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_teamType != TeamTypeCPU ) return false;


    bool popLocked = PopulationLocked();

    //
    // Is it time to look for a new objective?

	int maxObjectives = 3;
	if( g_app->Multiplayer() ) maxObjectives = m_numAntsInside / 10;
    if( maxObjectives < 1 ) maxObjectives = 1;
    if( maxObjectives > 6 ) maxObjectives = 6;

    if( !popLocked && GetNetworkTime() > m_objectiveTimer && m_objectives.Size() < maxObjectives )
    {
        Vector3 targetPos;
        WorldObjectId targetId;
        bool targetFound = false;

		if( g_app->Multiplayer() )
		{
			int searchType = syncrand() % 5;
			switch( searchType )
			{
				case 0 :
				case 1 :
							targetFound = SearchForDarwinians	( targetPos, targetId );		break;

				case 2 :	targetFound = SearchForEnemies      ( targetPos, targetId );		break;
				case 3 :	targetFound = SearchForSpirits      ( targetPos );					break;
				case 4 :    targetFound = SearchForScoutArea    ( targetPos );					break;
			}
		}
		else
		{
			if( !targetFound )      targetFound = SearchForDarwinians   ( targetPos, targetId );
			if( !targetFound )      targetFound = SearchForEnemies      ( targetPos, targetId );
			if( !targetFound )      targetFound = SearchForSpirits      ( targetPos );        
			if( !targetFound )      targetFound = SearchForScoutArea    ( targetPos );
		}

        if( targetFound )
        {
            AntObjective *objective = new AntObjective();
            objective->m_pos = targetPos;
            objective->m_targetId = targetId;
        
            objective->m_numToSend = 5 + 5 * (g_app->m_difficultyLevel / 10.0);
			if( g_app->Multiplayer() )
            {
                objective->m_numToSend = 5 + syncfrand(5);
                objective->m_numToSend += m_numAntsInside / 20.0;
            }

            m_objectives.PutData( objective );
        }

        m_objectiveTimer = GetNetworkTime() + syncrand() % 2;
    }

    
    //
    // Send out ants to our existing objectives

    if( !popLocked &&
        m_objectives.Size() > 0 && 
        GetNetworkTime() > m_spawnTimer &&
        m_numAntsInside > 0 )
    {
        int chosenIndex = syncrand() % m_objectives.Size();
        AntObjective *objective = m_objectives[chosenIndex];
        
        SpawnAnt( objective->m_pos, objective->m_targetId );


        objective->m_numToSend--;
        if( objective->m_numToSend <= 0 )
        {
            delete objective;
            m_objectives.RemoveData( chosenIndex );
        }

        m_spawnTimer = GetNetworkTime() + 0.2 - (0.2 * g_app->m_difficultyLevel / 10.0);

        if( g_app->Multiplayer() )
        {
            m_spawnTimer = GetNetworkTime() + 0.2;
        }
    }


    //
    // Convert spirits into ants

    if( m_numSpiritsInside > 0 &&
        GetNetworkTime() > m_eggConvertTimer )
    {
        m_numSpiritsInside--;
        m_numAntsInside += 3;

        double amountToAdd = 5.0;
        if( g_app->Multiplayer() )
        {
            amountToAdd = 1.0;
            m_numAntsInside += 3;
        }

        m_eggConvertTimer = GetNetworkTime() + amountToAdd;
    }


    if( g_app->Multiplayer()  )
    {
        if( m_numAntsInside <= 0 )
        {
            m_antInsideCheck++;

            if( m_antInsideCheck >= 30 )
            {
                // We've been empty of ants for too long
                // Take damage and reload
                m_antInsideCheck = 0;
                m_numAntsInside = 10;
                Damage( -10 );
            }
        }
        else
        {
            m_antInsideCheck = 0;
        }
    }


    //
    // Flicker if we are damaged

    double healthFraction = (double) m_health / 100.0;
    double timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
    m_renderDamaged = ( frand(0.75) * (1.0 - iv_abs(iv_sin(timeIndex))*1.2) > healthFraction );

    
    return ( m_health <= 0 );
}


void AntHill::Render( double _predictionTime )
{
	Matrix34 mat(m_front, m_up, m_pos);

    //
    // If we are damaged, flicked in and out based on our health

    if( m_renderDamaged )
    {
        double timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
        double thefrand = frand();
        if      ( thefrand > 0.7 ) mat.f *= ( 1.0 - iv_sin(timeIndex) * 0.7 );
        else if ( thefrand > 0.4 ) mat.u *= ( 1.0 - iv_sin(timeIndex) * 0.5 );    
        else                        mat.r *= ( 1.0 - iv_sin(timeIndex) * 0.7 );
        glEnable( GL_BLEND );
        glBlendFunc( GL_ONE, GL_ONE );
    }
    
    m_shape->Render(_predictionTime, mat);

    glDisable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );


//#ifdef DEBUG_RENDER_ENABLED
//    g_editorFont.DrawText3DCentre( m_pos + Vector3(0,140,0), 20, "Health %d", m_health );
//    g_editorFont.DrawText3DCentre( m_pos + Vector3(0,120,0), 20, "%d Ants Inside", m_numAntsInside );
//    g_editorFont.DrawText3DCentre( m_pos + Vector3(0,100,0), 20, "%d Spirits Inside", m_numSpiritsInside );
//    for( int i = 0; i < m_objectives.Size(); ++i )
//    {
//        AntObjective *objective = m_objectives[i];
//        RenderSphere( objective->m_pos, 5.0 );
//    }
//#endif
}


void AntHill::Burn()
{
    //
    // Damage us

    if( syncfrand(1.0) < 0.3 ) Damage(-1);


    //
    // Tell ants to get out

    Vector3 randomWaypoint( 200, 0, 0 );
    randomWaypoint.RotateAroundY( syncfrand( 2.0 * M_PI ) );
    randomWaypoint += m_pos;

    SpawnAnt( randomWaypoint, WorldObjectId() );
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
        AppDebugOut( "Error loading Anthill : Bogus population of %d\n", m_numAntsInside );
        m_numAntsInside = 0;
    }
}


void AntHill::Write ( TextWriter *_out )
{
    Building::Write( _out );
    
    _out->printf( "%d", m_numAntsInside );
}
