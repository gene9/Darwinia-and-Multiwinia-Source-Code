#include "lib/universal_include.h"

#include "lib/filesys/binary_stream_readers.h"
#include "lib/bitmap.h"
#include "lib/debug_render.h"
#include "lib/debug_utils.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/preferences.h"
#include "lib/math/random_number.h"

#include "app.h"
#include "camera.h"
#include "globals.h"
#include "location.h"
#include "entity_grid.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"
#include "gametimer.h"

#include "sound/soundsystem.h"

#include "worldobject/virii.h"
#include "worldobject/egg.h"


ViriiUnit::ViriiUnit(int teamId, int unitId, int numEntities, Vector3 const &_pos)
:   Unit(Entity::TypeVirii, teamId, unitId, numEntities, _pos),
    m_enemiesFound(false),
    m_cameraClose(false)
{
}


bool ViriiUnit::Advance( int _slice )
{
    double searchRadius = m_radius + VIRII_MAXSEARCHRANGE;

    m_enemiesFound = g_app->m_location->m_entityGrid->AreEnemiesPresent( m_centrePos.x, m_centrePos.z, searchRadius, m_teamId );
    
    return Unit::Advance( _slice );
}


void ViriiUnit::Render( double _predictionTime )
{        
    //
    // Render Red Virii shapes

	double nearPlaneStart = g_app->m_renderer->GetNearPlane();
	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.05,
							 			   g_app->m_renderer->GetFarPlane());

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/viriifull.bmp" ) );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    
	glEnable		( GL_BLEND );
	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glDepthMask     ( false );   
	glDisable		( GL_CULL_FACE );
    glBegin         ( GL_QUADS );

    //
    // Render high detail?

    int entityDetail = g_prefsManager->GetInt( "RenderEntityDetail", 1 );
    int viriiDetail = 1;
    if( entityDetail == 3 )
    {
        viriiDetail = 4;
    }
    else
    {
        double rangeToCam = ( m_centrePos - g_app->m_camera->GetPos() ).Mag();
        if      ( entityDetail == 1 && rangeToCam > 1000.0 )        viriiDetail = 2;
        else if ( entityDetail == 2 && rangeToCam > 1000.0 )        viriiDetail = 3;
        else if ( entityDetail == 2 && rangeToCam > 500.0 )         viriiDetail = 2;
        //else if ( entityDetail == 3 && rangeToCam > 1000.0 )        viriiDetail = 4;
        //else if ( entityDetail == 3 && rangeToCam > 600.0 )         viriiDetail = 3;
        //else if ( entityDetail == 3 && rangeToCam > 300.0 )         viriiDetail = 2;
    }

    //
	// Render all the entities that are up-to-date with server advances

    int lastUpdated = m_entities.GetLastUpdated();
    for (int i = 0; i <= lastUpdated; i++)
	{
        if (m_entities.ValidIndex(i))
        {
            Virii *virii = (Virii *) m_entities[i];
            virii->Render( _predictionTime, m_teamId, viriiDetail );
        }
	}

    //
	// Render all the entities that are one step out-of-date with server advances
    
	int size = m_entities.Size();
	_predictionTime += SERVER_ADVANCE_PERIOD;
	for (int i = lastUpdated + 1; i < size; i++)
	{
        if (m_entities.ValidIndex(i))
        {
            Virii *virii = (Virii *) m_entities[i];
            virii->Render( _predictionTime, m_teamId, viriiDetail );
        }
	}

    glEnd           ();
    glDisable       ( GL_TEXTURE_2D );
	glDisable		( GL_BLEND );
	glEnable		( GL_CULL_FACE );
    glDepthMask     ( true );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
								 		   g_app->m_renderer->GetFarPlane());
}

int Virii::s_viriiPopulation[NUM_TEAMS];

Virii::Virii()
:   Entity(),
    m_retargetTimer(-1),
    m_state(StateIdle),
    m_spiritId(-1),
    m_hoverHeight(0.1),
    m_historyTimer(0.0),
    m_prevPosTimer(0.0)
{
    m_retargetTimer = syncfrand(2.0);
}

void Virii::Begin()
{
    Entity::Begin();
    s_viriiPopulation[m_id.GetTeamId()]++;
}

Virii::~Virii()
{
    m_positionHistory.EmptyAndDelete();
    s_viriiPopulation[m_id.GetTeamId()]--;
}


bool Virii::Advance( Unit *_unit )
{
    m_prevPosTimer -= SERVER_ADVANCE_PERIOD;
    
    if( m_prevPosTimer <= 0.0 )
    {
        m_prevPos = m_pos;
        m_prevPosTimer = 5.0;
    }

    //
    // Take damage if we're in the water  
    // Actually, don't
    // if( m_pos.y < 0.0 ) ChangeHealth( -1 );    

    bool amIDead = Entity::Advance(_unit);
	    
    if( m_dead )
    {
        m_vel.Zero();

        if( m_spiritId != -1 )
        {
            if( g_app->m_location->m_spirits.ValidIndex(m_spiritId) )
            {
                Spirit *spirit = g_app->m_location->m_spirits.GetPointer( m_spiritId );
                if( spirit && spirit->m_state == Spirit::StateAttached )
                {
                    spirit->CollectorDrops();
                }
            }
            m_spiritId = -1;
        }
    }

	if( !m_onGround )
    {
        m_vel.y += -10.0 * SERVER_ADVANCE_PERIOD;
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;
        
        double groundLevel = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) + 2.0;
        if( m_pos.y <= groundLevel )
        {
            m_onGround = true;
            m_vel = g_zeroVector;
        }
    }

    if( !m_dead )
    {
        switch( m_state )
        {
            case StateIdle:             amIDead = AdvanceIdle();                break;
            case StateAttacking:        amIDead = AdvanceAttacking();           break;
            case StateToSpirit:         amIDead = AdvanceToSpirit();            break;
            case StateToEgg:            amIDead = AdvanceToEgg();               break;
        }

        if( g_app->m_location->m_spirits.ValidIndex(m_spiritId) )
        {
            Spirit *spirit = g_app->m_location->m_spirits.GetPointer( m_spiritId );
            if( spirit && spirit->m_state == Spirit::StateAttached )
            {
                spirit->m_pos = m_pos;
				spirit->m_pos.y += 2.0;
                spirit->m_vel = m_vel;
            }
        }            

		// Next 5 lines by Andrew. Purpose - to prevent virii grinding themselves against
		// the side of obstructions.
//        Vector3 newPos(PushFromObstructions( m_pos ));
//		if (newPos.x != m_pos.x || newPos.z != m_pos.z)
//		{
//			m_pos = newPos;
//			if (m_retargetTimer > 0.05) m_retargetTimer = 0.1;
//		}
	}
        
    if( m_positionHistory.Size() > 0 ) 
    {
        bool recorded = false;

        if( m_state != StateIdle )
        {
            m_historyTimer -= SERVER_ADVANCE_PERIOD;
            if( m_historyTimer <= 0.0 )
            {
                m_historyTimer = 0.5;
                RecordHistoryPosition(true);    
                recorded = true;
            }
        }

        if( !recorded )
        {
            double lastRecordedHeight = m_positionHistory[0]->m_pos.y;
            if( iv_abs(m_pos.y - lastRecordedHeight) > 3.0 )
            {
                RecordHistoryPosition(false);
            }
        }
    }
    else
    {
        RecordHistoryPosition(true);
    }
    
    double worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    double worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    if( m_pos.x < 0.0 ) m_pos.x = 0.0;
    if( m_pos.z < 0.0 ) m_pos.z = 0.0;
    if( m_pos.x >= worldSizeX ) m_pos.x = worldSizeX;
    if( m_pos.z >= worldSizeZ ) m_pos.z = worldSizeZ;
           
    return amIDead;
}


void Virii::RecordHistoryPosition( bool _required )
{    
    START_PROFILE( "RecordHistory" );

    Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
    Vector3 prevPos;
    if( m_positionHistory.Size() > 0 ) prevPos = m_positionHistory[0]->m_pos;        

    ViriiHistory *history = new ViriiHistory();
    history->m_pos = m_pos;
    history->m_right = ( m_pos - prevPos ) ^ landNormal;
    history->m_right.Normalise();
    history->m_distance = 0.0;
    history->m_required = _required;
    
    if( m_positionHistory.Size() > 0 )
    {
        history->m_distance = ( m_pos - m_positionHistory[0]->m_pos ).Mag();
        history->m_glowDiff = ( m_pos - m_positionHistory[0]->m_pos );
        history->m_glowDiff.SetLength( 10.0 );
    }


    m_positionHistory.PutDataAtStart( history );
    
    double totalDistance = 0.0;
    int removeFrom = -1;
    int entityDetail = g_prefsManager->GetInt( "RenderEntityDetail", 1 );
    double tailLength = 175.0;          // - (entityDetail-1) * 50.0;
    for( int i = 0; i < m_positionHistory.Size(); ++i )
    {
        ViriiHistory *history = m_positionHistory[i];
        totalDistance += history->m_distance;

        if( totalDistance > tailLength )
        {
            removeFrom = i;
            break;
        }
    }

    if( removeFrom != -1 )
    {
        while( m_positionHistory.ValidIndex( removeFrom ) )
        {
            ViriiHistory *history = m_positionHistory[ removeFrom ];
            m_positionHistory.RemoveData( removeFrom );
            delete history;
        }
    }

    END_PROFILE(  "RecordHistory" );
}


bool Virii::AdvanceToTargetPos(Vector3 const &_pos)
{
    START_PROFILE( "AdvanceToTargetPos" );
    
    Vector3 oldPos = m_pos;

    Vector3 distance = _pos - m_pos;
    m_vel = distance;
    m_vel.Normalise();
    m_front = m_vel;
    m_vel *= m_stats[StatSpeed];
    distance.y = 0.0;

    Vector3 nextPos = m_pos + m_vel * SERVER_ADVANCE_PERIOD;
    nextPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z) + m_hoverHeight;
    double currentHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    double nextHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( nextPos.x, nextPos.z );

    //
    // Slow us down if we're going up hill
    // Speed up if going down hill

    double factor = 1.0 - (currentHeight - nextHeight) / -5.0;
    if( factor < 0.1 ) factor = 0.1;
    if( factor > 2.0 ) factor = 2.0;
    m_vel *= factor;
    nextPos = m_pos + m_vel * SERVER_ADVANCE_PERIOD;
    nextPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z) + m_hoverHeight;
        

    //
    // Are we there?
    
    bool arrived = false;
    if (distance.MagSquared() < m_vel.MagSquared() * SERVER_ADVANCE_PERIOD * SERVER_ADVANCE_PERIOD)
    {                    
        nextPos = _pos;
        arrived = true;    
    }
    
    m_pos = nextPos;    
    m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;
    
    END_PROFILE(  "AdvanceToTargetPos" );

    return arrived;
}


bool Virii::AdvanceIdle()
{
	START_PROFILE( "AdvanceIdle");

    m_retargetTimer -= SERVER_ADVANCE_PERIOD;
    bool foundTarget = false;

    if( m_retargetTimer <= 0.0 )
    {
        m_retargetTimer = 1.0;

        if( g_app->m_location->m_spirits.ValidIndex(m_spiritId) )
        {
            foundTarget = SearchForEggs();
        }
            
        if( !foundTarget ) foundTarget = SearchForEnemies();        
        if( !foundTarget ) foundTarget = SearchForSpirits();
        if( !foundTarget ) foundTarget = SearchForIdleDirection();        
    }

    if( m_state == StateIdle )
    {
        AdvanceToTargetPos( m_wayPoint );
    }

	END_PROFILE( "AdvanceIdle");
    return false;
}


bool Virii::AdvanceAttacking()
{
	START_PROFILE( "AdvanceAttacking");

    WorldObject *enemy = g_app->m_location->GetEntity( m_enemyId );
    Entity *entity = (Entity *) enemy;

    if( !entity || entity->m_dead )
    {
        m_state = StateIdle;
	    END_PROFILE( "AdvanceAttacking");
        return false;
    }

//    int enemySpiritId = g_app->m_location->GetSpirit( m_enemyId );
//    if( enemySpiritId != -1 && entity->m_dead )
//    {
//        // Our enemy has died and dropped a spirit
//        // If we can see an egg, then go for it
//        if( FindNearbyEgg( enemySpiritId, VIRII_MAXSEARCHRANGE ).IsValid() )
//        {
//            m_enemyId.SetInvalid();
//            m_spiritId = enemySpiritId;
//            m_state = StateToSpirit;
//        }
//        return false;
//    }

    bool arrived = AdvanceToTargetPos( entity->m_pos );
    if( arrived )
    {
        entity->ChangeHealth( -20 );
        for( int i = 0; i < 3; ++i )
        {
			Vector3 vel = Vector3(  SFRAND(15.0), 15.0, SFRAND(15.0) );
			vel.y += syncsfrand(15.0);
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeMuzzleFlash );
        }
        g_app->m_soundSystem->TriggerEntityEvent( this, "Attack" );
        SearchForEnemies();
    }

	END_PROFILE( "AdvanceAttacking");
    return false;
}


bool Virii::AdvanceToSpirit()
{
	START_PROFILE( "AdvanceToSpirit");

    Spirit *s = NULL;
    if( g_app->m_location->m_spirits.ValidIndex(m_spiritId) )
    {
        s = g_app->m_location->m_spirits.GetPointer(m_spiritId);
    }

    if( !s || 
         s->m_state == Spirit::StateDeath ||
         s->m_state == Spirit::StateAttached ||
         s->m_state == Spirit::StateInEgg ) 
    {
        m_spiritId = -1;
        m_state = StateIdle;
		END_PROFILE( "AdvanceToSpirit");
        return false;
    }

    Spirit *spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
    Vector3 targetPos = spirit->m_pos;
    targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z );
    bool arrived = AdvanceToTargetPos( targetPos );
    if( arrived )
    {
        spirit->CollectorArrives();
        m_state = StateToEgg;
    }

	END_PROFILE( "AdvanceToSpirit");
    return false;
}


bool Virii::AdvanceToEgg()
{
	START_PROFILE( "AdvanceToEgg");
    Egg *theEgg = (Egg *) g_app->m_location->GetEntitySafe( m_eggId, Entity::TypeEgg );

    if( !theEgg || theEgg->m_state == Egg::StateFertilised )
    {
        bool found = SearchForEggs();
        if( !found )
        {
            // We can't find any eggs, so go into holding pattern
            if( g_app->m_location->m_spirits.ValidIndex( m_spiritId ) )
            {   
                Spirit *spirit = g_app->m_location->m_spirits.GetPointer( m_spiritId );
                if( spirit->m_state == Spirit::StateAttached )
                {
                    spirit->CollectorDrops();
                }
                m_spiritId = -1;
            }
            m_state = StateIdle;
            END_PROFILE( "AdvanceToEgg");
            return false;
        }
    }
    
    if( !g_app->m_location->m_spirits.ValidIndex( m_spiritId ) )
    {
        m_spiritId = -1;
        m_state = StateIdle;
        END_PROFILE( "AdvanceToEgg");
        return false;
    }
    
    //
    // At this point we MUST have found an egg, otherwise we'd have returned by now

    theEgg = (Egg *) g_app->m_location->GetEntitySafe( m_eggId, Entity::TypeEgg );
    AppDebugAssert( theEgg );

    bool arrived = AdvanceToTargetPos( theEgg->m_pos );
    
    if( arrived )
    {
        theEgg->Fertilise( m_spiritId );
        m_spiritId = -1;
        m_eggId.SetInvalid();
    }

	END_PROFILE( "AdvanceToEgg");
    return false;
}


bool Virii::SearchForEnemies()
{
	START_PROFILE( "SearchForEnemies");

    ViriiUnit *unit = (ViriiUnit *) g_app->m_location->GetUnit( m_id );
    if( unit && !unit->m_enemiesFound ) 
    {
    	END_PROFILE( "SearchForEnemies");
        return false;
    }

    WorldObjectId bestEnemyId = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 
                                                                            VIRII_MINSEARCHRANGE, 
                                                                            VIRII_MAXSEARCHRANGE, 
                                                                            m_id.GetTeamId() );
    Entity *enemy = g_app->m_location->GetEntity( bestEnemyId );

    if( enemy && !enemy->m_dead && enemy->m_onGround )
    {
        m_enemyId = bestEnemyId;
        m_state = StateAttacking;
		END_PROFILE( "SearchForEnemies");
        return true;
    }

	END_PROFILE( "SearchForEnemies");
    return false;
}


bool Virii::SearchForSpirits()
{   
	START_PROFILE( "SearchForSpirits");

    Spirit *found = NULL;
    int spiritId = -1;
    double closest = 999999.9;

    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        if( g_app->m_location->m_spirits.ValidIndex(i) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
            double theDist = ( s->m_pos - m_pos ).Mag();

            if( theDist <= VIRII_MAXSEARCHRANGE &&
                theDist < closest &&
                s->NumNearbyEggs() > 0 &&
                ( s->m_state == Spirit::StateBirth ||
                  s->m_state == Spirit::StateFloating ) )
            {                
                found = s;
                spiritId = i;
                closest = theDist;
            }
        }
    }            

    if( found )
    {
        m_spiritId = spiritId;
        m_state = StateToSpirit;
		END_PROFILE( "SearchForSpirits");
        return true;
    }    

	END_PROFILE( "SearchForSpirits");
    return false;
}


WorldObjectId Virii::FindNearbyEgg( int _spiritId, double _autoAccept )
{   
    if( !g_app->m_location->m_spirits.ValidIndex(_spiritId) )
    {
        return WorldObjectId();
    }

    Spirit *spirit = g_app->m_location->m_spirits.GetPointer( _spiritId );
    if( !spirit ) return WorldObjectId();

    WorldObjectId *m_nearbyEggs = spirit->GetNearbyEggs();
    int numNearbyEggs = spirit->NumNearbyEggs();

    WorldObjectId eggId;
    double closest = 999999.9;

    for( int i = 0; i < numNearbyEggs; ++i )
    {
        WorldObjectId thisEggId = m_nearbyEggs[i];
        Egg *egg = (Egg *) g_app->m_location->GetEntitySafe( thisEggId, Entity::TypeEgg );
   
        if( egg && egg->m_state == Egg::StateDormant )
        {
            double theDist = ( egg->m_pos - spirit->m_pos ).Mag();
            if( theDist <= _autoAccept )
            {
                return thisEggId;
            }
            else if( theDist < closest )
            {
                eggId = thisEggId;
                closest = theDist;
            }
        }
    }

    return eggId;    
}


WorldObjectId Virii::FindNearbyEgg( Vector3 const &_pos )
{
    int numFound;
    g_app->m_location->m_entityGrid->GetFriends( s_neighbours, _pos.x, _pos.z, VIRII_MAXSEARCHRANGE, &numFound, m_id.GetTeamId() );

    //
    // Build a list of candidates within range

    LList<WorldObjectId> eggIds;

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Egg *egg = (Egg *) g_app->m_location->GetEntitySafe( id, Entity::TypeEgg );
        
        if( egg &&
            egg->m_state == Egg::StateDormant &&                
            egg->m_onGround )
        {
            eggIds.PutData( id );
        }
    }


    //
    // Chose one randomly

    if( eggIds.Size() > 0 )
    {
        int chosenIndex = syncfrand( eggIds.Size() );
        WorldObjectId *eggId = eggIds.GetPointer(chosenIndex);        
        return *eggId;
    }
    else
    {
        return WorldObjectId();
    }
}


bool Virii::SearchForEggs()
{
	START_PROFILE( "SearchForEggs");

    WorldObjectId eggId = FindNearbyEgg( m_pos );

    if( eggId.IsValid() )
    {
        m_eggId = eggId;
        m_state = StateToEgg;
		END_PROFILE( "SearchForEggs");
        return true;
    }

	END_PROFILE( "SearchFprEggs");
    return false;
}


bool Virii::SearchForIdleDirection()
{
	START_PROFILE( "SearchForIdleDir");

    double distToSpawnPoint = ( m_pos - m_spawnPoint ).Mag();
    double chanceOfReturn = ( distToSpawnPoint / m_roamRange );
    if( chanceOfReturn < 0.75 ) chanceOfReturn = 0.0;
    if( chanceOfReturn >= 1.0 || syncfrand(1.0) <= chanceOfReturn )
    {
        // We have strayed too far from our spawn point
        // So head back there now
        Vector3 newDirection = ( m_spawnPoint - m_pos );        
        newDirection.y = 0;
        if      ( newDirection.x < -0.5 )  newDirection.x = -1.0;
        else if ( newDirection.x > 0.5 )   newDirection.x = 1.0;
        else                                newDirection.x = 0.0; 
        if      ( newDirection.z < -0.5 )  newDirection.z = -1.0;
        else if ( newDirection.z > 0.5 )   newDirection.z = 1.0;
        else                                newDirection.z = 0.0; 
        newDirection.SetLength( m_stats[StatSpeed] );
        Vector3 nextPos = m_pos + newDirection * m_retargetTimer;
        nextPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( nextPos.x, nextPos.z );
        m_wayPoint = nextPos;
        m_state = StateIdle;
        RecordHistoryPosition(true);
        g_app->m_soundSystem->TriggerEntityEvent( this, "ChangeDirection" );
		END_PROFILE( "SearchForIdleDir");
        return true;
    }
    else
    {
        int attempts = 0;
        while( attempts < 3 )
        {
            ++attempts;

            int x = ( ((int) syncfrand(3.0)) - 1 );
            int z = ( ((int) syncfrand(3.0)) - 1 );
            Vector3 newVel( x, 0.0, z );
            newVel.Normalise();
            newVel *= m_stats[StatSpeed];

            Vector3 nextPos = m_pos + newVel * m_retargetTimer;
            nextPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( nextPos.x, nextPos.z );
            if( nextPos.y > 0.0 ) 
            {           
                m_wayPoint = nextPos;
                m_state = StateIdle;
                RecordHistoryPosition(true);        
                g_app->m_soundSystem->TriggerEntityEvent( this, "ChangeDirection" );
				END_PROFILE( "SearchForIdleDir");
                return true;
            }
        }
    }
    
	END_PROFILE( "SearchForIdleDir");
    return false;
}


Vector3 Virii::AdvanceDeadPositionVector( int _index, Vector3 const &_pos, double _time )
{
    double distance = 10.0 * _time;
    Vector3 result = _pos;

    switch( _index % 8 )
    {
        case 0 :        result += Vector3( -distance, 0, 0 );                  break;
        case 1 :        result += Vector3( -distance, 0, distance );           break;
        case 2 :        result += Vector3( 0, 0, distance );                   break;
        case 3 :        result += Vector3( distance, 0, distance );            break;
        case 4 :        result += Vector3( distance, 0, 0 );                   break;
        case 5 :        result += Vector3( distance, 0, -distance );           break;
        case 6 :        result += Vector3( 0, 0, -distance );                  break;
        case 7 :        result += Vector3( -distance, 0, -distance );          break;
    }

    return result;
}


bool Virii::AdvanceDead()
{
    for( int i = 0; i < m_positionHistory.Size(); ++i )
    {   
        Vector3 *thisPos = &m_positionHistory[i]->m_pos;
        *thisPos = AdvanceDeadPositionVector( i, *thisPos, SERVER_ADVANCE_PERIOD );
    }
    return true;
}


bool Virii::IsInView()
{
    Vector3 centrePos = m_pos;
    double radiusSqd = 0.0;

    for( int i = 0; i < m_positionHistory.Size(); ++i )
    {
        Vector3 pos = m_positionHistory[i]->m_pos;
        double distance = ( pos - centrePos ).MagSquared();
        if( distance > radiusSqd )
        {
            radiusSqd = distance;
        }
    }

    double radius = iv_sqrt( radiusSqd );
    return g_app->m_camera->SphereInViewFrustum( centrePos, radius );
}


void Virii::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );
    _list->PutData( "ChangeDirection" );
}


void Virii::Render( double predictionTime, int teamId, int _detail )
{           
    predictionTime += SERVER_ADVANCE_PERIOD;

    Vector3 predictedPos = m_pos + m_vel * predictionTime;
    if( m_onGround && _detail == 1 ) 
	{
		predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z ) + m_hoverHeight;
	}

    double health = (double) m_stats[StatHealth] / (double) EntityBlueprint::GetStat( m_type, StatHealth );
    health = min( health, 1.0 );
    
    RGBAColour wormColour = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour * health;
    RGBAColour glowColour( 200, 100, 100 );
    if( g_app->Multiplayer() )
    {
        glowColour = wormColour;
    }
    wormColour.a = 200;
    glowColour.a = 150;

    if( teamId == g_app->m_location->GetMonsterTeamId() )
    {
        wormColour.Set( 255, 255, 255, 0 );
        glowColour.Set( 200, 200, 200, 50 );

        if( m_dead )
        {
            glEnd();
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
            glBegin( GL_QUADS );
        }
    }    
    
    if( m_dead )
    {
        wormColour.r = wormColour.g = wormColour.b = 250;
        wormColour.a = 100 * m_stats[StatHealth] / 100.0;

        glowColour.r = glowColour.g = glowColour.b = 250;
        glowColour.a = 250 * m_stats[StatHealth] / 100.0;
    }

    Vector3 landNormal = g_upVector;
    if( _detail == 1 ) landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );

    ViriiHistory prevPos;
    prevPos.m_pos = predictedPos;
    prevPos.m_right = -m_front ^ landNormal;
    Vector3 firstPos;
    if( m_positionHistory.Size() > 0 ) firstPos = m_positionHistory[0]->m_pos;
    prevPos.m_distance = ( predictedPos - firstPos ).Mag();
    prevPos.m_glowDiff = ( predictedPos - firstPos ).SetLength(10.0);

    #define wormWidth       3.0
    #define wormTexW        32.0/(32.0+128.0)
    #define wormTexHeight   (10.0 * 2)/1024.0

    #define glowWidth       12.0
    #define glowTexXpos     32.0/(32.0+128.0)
    #define glowTexH        128.0/512.0

    double wormTexYpos = 0.0;
    double skippedDistance = 0.0;

    int lastIndex = m_positionHistory.Size();
    if( _detail > 1 )
    {
        int amountToChop = _detail-1;
        amountToChop = min( amountToChop, 2 );
        lastIndex *= ( 1.0 - 0.25 * amountToChop );
    }

    for( int i = 0; i < lastIndex; i ++ )
    {        
        ViriiHistory *history = m_positionHistory[i];

        if( !history->m_required )
        {
            // This worm is a long way away, so we can skip this point in his history
            skippedDistance += history->m_distance;
            continue;
        }

        Vector3 &pos = history->m_pos;     
        Vector3 wormRightAngle = prevPos.m_right * wormWidth;
        Vector3 glowRightAngle = prevPos.m_right * glowWidth;
        double distance = prevPos.m_distance + skippedDistance;
        Vector3 const &glowDiff = prevPos.m_glowDiff;
        
        skippedDistance = 0.0;

             
        //
        // Worm shape

        int newCol = wormColour.a - distance;
        if( newCol < 0 ) newCol = 0;
        wormColour.a = newCol;
        glColor4ubv ( wormColour.GetData() );

        glTexCoord2f( 0.0, wormTexYpos );      glVertex3dv( (prevPos.m_pos - wormRightAngle).GetData() );
        glTexCoord2f( wormTexW, wormTexYpos );  glVertex3dv( (prevPos.m_pos + wormRightAngle).GetData() );
        wormTexYpos += (distance*6) / 512.0;        
        glTexCoord2f( wormTexW, wormTexYpos );  glVertex3dv( (pos + wormRightAngle).GetData() );
        glTexCoord2f( 0.0, wormTexYpos );      glVertex3dv( (pos - wormRightAngle).GetData() );
        
        //
        // Glow effect
         
        if( _detail < 4 )
        {
            newCol = glowColour.a - distance;
            if( newCol < 0 ) newCol = 0;
            glowColour.a = newCol;
            glColor4ubv ( glowColour.GetData() );

            glTexCoord2f( glowTexXpos, 0.0 );      glVertex3dv( (prevPos.m_pos - glowRightAngle + glowDiff).GetData() );
            glTexCoord2f( 1.0, 0.0 );             glVertex3dv( (prevPos.m_pos + glowRightAngle + glowDiff).GetData() );
        
            glTexCoord2f( 1.0, glowTexH );         glVertex3dv( (pos + glowRightAngle - glowDiff).GetData() );
            glTexCoord2f( glowTexXpos, glowTexH );  glVertex3dv( (pos - glowRightAngle - glowDiff).GetData() );
        }
        
        prevPos = *history;
    }

    if( teamId == g_app->m_location->GetMonsterTeamId() && m_dead )
    {
        glEnd();
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
        glBegin( GL_QUADS );
    }
}

/*
void Virii::RenderLowDetail( double predictionTime, int teamId )
{
    predictionTime += 0.1;
    
    Vector3 predictedPos = m_pos + m_vel * predictionTime;
    if( m_onGround ) 
	{
		predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z ) + m_hoverHeight;
	}

    double health = (double) m_stats[StatHealth] / (double) EntityBlueprint::GetStat( m_type, StatHealth );
    RGBAColour colour = g_app->m_location->m_teams[ m_teamId ]->m_colour * health;
    colour.a = 200;
    if( m_dead )
    {
        colour.r = colour.g = colour.b = 50;
        colour.a = 100 * m_stats[StatHealth] / 100.0;
    }
    
    glColor4ubv( colour.GetData() );

    Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );

    ViriiHistory prevPos;
    prevPos.m_pos = predictedPos;
    prevPos.m_right = -m_front ^ landNormal;
    Vector3 firstPos;
    if( m_positionHistory.Size() > 0 ) firstPos = m_positionHistory[0]->m_pos;
    prevPos.m_distance = ( predictedPos - firstPos ).Mag();

    double width = 3.0;
    double texYpos = 0.0;
    double texW = 32.0/(32.0+128.0);
    double texHeight = (10.0 * 2)/1024.0;

    glBegin         ( GL_QUADS );

    for( int i = 0; i < m_positionHistory.Size(); i += 1 )
    {        
        ViriiHistory *history = m_positionHistory[i];
        Vector3 pos = history->m_pos;        
        Vector3 rightAngle = prevPos.m_right * width;
        double distance = prevPos.m_distance;
     
        int newCol = colour.a - distance;
        if( newCol < 0 ) newCol = 0;
        colour.a = newCol;
        glColor4ubv( colour.GetData() );
        
        glTexCoord2f( 0.0, texYpos );  glVertex3dv( (prevPos.m_pos - rightAngle).GetData() );
        glTexCoord2f( texW, texYpos );  glVertex3dv( (prevPos.m_pos + rightAngle).GetData() );

        texYpos += (distance*6) / 512.0;
        
        glTexCoord2f( texW, texYpos );  glVertex3dv( (pos + rightAngle).GetData() );
        glTexCoord2f( 0.0, texYpos );  glVertex3dv( (pos - rightAngle).GetData() );
                
        prevPos = *history;
    }
    
    glEnd();

}

void Virii::RenderGlow ( double predictionTime, int teamId )
{
    predictionTime += 0.1;
    
    Vector3 predictedPos = m_pos + m_vel * predictionTime;
    if( m_onGround ) 
	{
		//predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z ) + m_hoverHeight;
	}

    double health = (double) m_stats[StatHealth] / (double) EntityBlueprint::GetStat( m_type, StatHealth );
    //RGBAColour colour = g_app->m_location->m_teams[ m_teamId ]->m_colour * health;
    RGBAColour colour( 200, 100, 100 );
    colour.a = 100;
    if( m_dead )
    {
        colour.r = colour.g = colour.b = 50;
        colour.a = 250 * m_stats[StatHealth] / 100.0;
    }
    
    glColor4ubv( colour.GetData() );

    //Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );
    Vector3 landNormal = g_upVector;
    
    ViriiHistory prevPos;
    prevPos.m_pos = predictedPos;
    prevPos.m_right = -m_front ^ landNormal;
    Vector3 firstPos;
    if( m_positionHistory.Size() > 0 ) firstPos = m_positionHistory[0]->m_pos;
    prevPos.m_distance = ( predictedPos - firstPos ).Mag();

    double width = 12.0;
    double texXpos = 32.0/(32.0+128.0);
    double texH = 128.0/512.0;
    
    glBegin         ( GL_QUADS );

    for( int i = 0; i < m_positionHistory.Size(); i += 1 )
    {        
        ViriiHistory *history = m_positionHistory[i];
        Vector3 pos = history->m_pos;        
        Vector3 rightAngle = prevPos.m_right * width;
        double distance = prevPos.m_distance;
     
        Vector3 diff = (prevPos.m_pos - pos).Normalise();
        
        int newCol = colour.a - distance;
        if( newCol < 0 ) newCol = 0;
        colour.a = newCol;
        glColor4ubv( colour.GetData() );
        
        glTexCoord2f( texXpos, 0.0 );      glVertex3dv( (prevPos.m_pos - rightAngle + diff * 10.0).GetData() );
        glTexCoord2f( 1.0, 0.0 );         glVertex3dv( (prevPos.m_pos + rightAngle + diff * 10.0).GetData() );
        
        glTexCoord2f( 1.0, texH );         glVertex3dv( (pos + rightAngle - diff * 10.0).GetData() );
        glTexCoord2f( texXpos, texH );      glVertex3dv( (pos - rightAngle - diff * 10.0).GetData() );
                
        prevPos = *history;
    }
    
    glEnd();
    
}
*/
