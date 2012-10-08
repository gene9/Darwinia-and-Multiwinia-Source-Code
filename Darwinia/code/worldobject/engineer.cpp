#include "lib/universal_include.h"

#include <math.h>

#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/matrix34.h"
#include "lib/shape.h"
#include "lib/debug_utils.h"
#include "lib/text_renderer.h"
#include "lib/debug_render.h"
#include "lib/language_table.h"

#include "lib/input/input.h"
#include "lib/input/input_types.h"

#include "app.h"
#include "explosion.h"
#include "globals.h"
#include "global_world.h"
#include "level_file.h"
#include "location.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"
#include "user_input.h"
#include "obstruction_grid.h"
#include "camera.h"
#include "main.h"

#include "sound/soundsystem.h"

#include "worldobject/incubator.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/controltower.h"
#include "worldobject/engineer.h"
#include "worldobject/bridge.h"
#include "worldobject/researchitem.h"



Engineer::Engineer()
:   Entity(),
    m_hoverHeight(15.0f),
    m_state(StateIdle),
    m_retargetTimer(0.0f),
    m_spiritId(-1),
    m_positionId(-1),
    m_bridgeId(-1)
{
    m_idleRotateRate = syncsfrand(0.5f);

    m_shape = g_app->m_resource->GetShape( "engineer.shp" );

    Matrix34 mat( Vector3(0,0,1), g_upVector, g_zeroVector );
    m_centrePos = m_shape->CalculateCentre(mat);
    m_radius = m_shape->CalculateRadius( mat, m_centrePos );
}


void Engineer::Begin()
{
    Entity::Begin();
    m_wayPoint = m_pos;
    m_targetPos = m_wayPoint;
}


void Engineer::SetWaypoint( Vector3 const &_wayPoint )
{
    m_retargetTimer = 0.0f;
    m_wayPoint = _wayPoint;
    m_state = StateToWaypoint;
}


int Engineer::GetNumSpirits()
{
    return m_spirits.Size();
}


int Engineer::GetMaxSpirits()
{
    int engineerLevel = g_app->m_globalWorld->m_research->CurrentLevel( m_id.GetTeamId(), GlobalResearch::TypeEngineer );
    switch( engineerLevel )
    {
        case 0 :        return 0;
        case 1 :        return 10;
        case 2 :        return 15;
        case 3 :        return 25;
        case 4 :        return 30;
    }

    return 0;
}


bool Engineer::SearchForSpirits()
{
    if( m_spirits.Size() < GetMaxSpirits() )
    {
        Spirit *found = NULL;
        int spiritId = -1;
        float closest = 999999.9f;

        for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
        {
            if( g_app->m_location->m_spirits.ValidIndex(i) )
            {
                Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
                float theDist = ( s->m_pos - m_pos ).Mag();

                if( theDist <= ENGINEER_SEARCHRANGE &&
                    theDist < closest &&
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
            return true;
        }
    }

    return false;
}


bool Engineer::SearchForControlTowers()
{
    int buildingIndex = -1;
    float closest = 99999.9f;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeControlTower )
            {
                ControlTower *ct = (ControlTower *) building;
                Vector3 pos, front;
                //if( (ct->m_id.GetTeamId() != m_id.GetTeamId() || ct->m_ownership < 100.0f) &&
				if ( (!g_app->m_location->IsFriend(ct->m_id.GetTeamId(), m_id.GetTeamId()) || ct->m_ownership < 100.0f ) &&
                    ct->GetAvailablePosition(pos, front) != -1 )
                {
                    float theDist = (building->m_pos - m_pos).Mag();
                    if( theDist <= ENGINEER_SEARCHRANGE &&
                        theDist < closest )
                    {
                        buildingIndex = i;
                        closest = theDist;
                    }
                }
            }
        }
    }

    if( buildingIndex != -1 )
    {
        Building *building = g_app->m_location->m_buildings[buildingIndex];
        m_buildingId = building->m_id.GetUniqueId();
        m_state = StateToControlTower;
        return true;
    }

    return false;
}


bool Engineer::SearchForBridges()
{
    int buildingIndex = -1;
    float closest = 99999.9f;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeBridge)
            {
                Bridge *bridge = (Bridge *) building;
                Vector3 pos, front;
                float theDist = (building->m_pos - m_pos).Mag();
                if( bridge->GetAvailablePosition( pos, front ) &&
                    bridge->m_status < 100.0f &&
                    theDist <= ENGINEER_SEARCHRANGE &&
                    theDist < closest )
                {
                    buildingIndex = i;
                    closest = theDist;
                }
            }
        }
    }

    if( buildingIndex != -1 )
    {
        Building *building = g_app->m_location->m_buildings[buildingIndex];
        m_buildingId = building->m_id.GetUniqueId();
        m_state = StateToBridge;
        return true;
    }

    return false;
}


bool Engineer::SearchForResearchItems()
{
    float closest = 99999.9f;
    int buildingIndex = -1;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeResearchItem)
            {
                ResearchItem *item = (ResearchItem *) building;
                float theDist = (building->m_pos - m_pos).Mag();
                if( item->NeedsReprogram() &&
                    theDist <= ENGINEER_SEARCHRANGE &&
                    theDist < closest )
                {
                    buildingIndex = i;
                    closest = theDist;
                }
            }
        }
    }

    if( buildingIndex != -1 )
    {
        Building *building = g_app->m_location->m_buildings[buildingIndex];
        m_buildingId = building->m_id.GetUniqueId();
        Vector3 usToThem = ( building->m_pos - m_pos ).SetLength( 35.0f );
        m_targetPos = building->m_pos - usToThem;
        m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );
        m_targetPos.y += m_hoverHeight;
        m_state = StateToResearchItem;
        return true;
    }

    return false;
}


void Engineer::ChangeHealth( int amount )
{
    if( !m_dead )
    {
        int healthBandBefore = int( m_stats[StatHealth] / 50.0f );

        if( amount < 0 )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "LoseHealth" );
        }

        if( m_stats[StatHealth] + amount < 0 )
        {
            m_stats[StatHealth] = 0;
            m_dead = true;
            g_app->m_soundSystem->TriggerEntityEvent( this, "Die" );
        }
        else if( m_stats[StatHealth] + amount > 255 )
        {
            m_stats[StatHealth] = 255;
        }
        else
        {
            m_stats[StatHealth] += amount;
            g_app->m_particleSystem->CreateParticle( m_pos, g_zeroVector, Particle::TypeMuzzleFlash );
        }

        int healthBandAfter = int( m_stats[StatHealth] / 50.0f );

        float fractionDead = 1.0f - m_stats[StatHealth] / (float) EntityBlueprint::GetStat( TypeEngineer, StatHealth );
        fractionDead = max( fractionDead, 0.0f );
        fractionDead = min( fractionDead, 1.0f );

        if( fractionDead == 1.0f || healthBandAfter < healthBandBefore )
        {
            Matrix34 transform( m_front, g_upVector, m_pos );
            g_explosionManager.AddExplosion( m_shape, transform, fractionDead );
        }
    }
}


bool Engineer::Advance( Unit *_unit )
{
    bool amIDead = false;

    switch( m_state )
    {
        case StateIdle:                 amIDead = AdvanceIdle();             break;
        case StateToWaypoint:           amIDead = AdvanceToWaypoint();       break;
        case StateToSpirit:             amIDead = AdvanceToSpirit();         break;
        case StateToIncubator:          amIDead = AdvanceToIncubator();      break;
        case StateToControlTower:       amIDead = AdvanceToControlTower();   break;
        case StateReprogramming:        amIDead = AdvanceReprogramming();    break;
        case StateToBridge:             amIDead = AdvanceToBridge();         break;
        case StateOperatingBridge:      amIDead = AdvanceOperatingBridge();  break;
        case StateToResearchItem:       amIDead = AdvanceToResearchItem();   break;
        case StateResearching:          amIDead = AdvanceResearching();      break;
    };

    //
    // If I am now dead, handle it accordingly

    amIDead = Entity::Advance(_unit);
    if( amIDead )
    {
        // Drop all my spirits
        while( m_spirits.Size() > 0 )
        {
            int spiritId = m_spirits[0];
            if( g_app->m_location->m_spirits.ValidIndex(spiritId) )
            {
                Spirit *s = g_app->m_location->m_spirits.GetPointer(spiritId);
                s->CollectorDrops();
                m_spirits.RemoveData(0);
            }
        }

        // If I was reprogramming something, stop now
        if( m_state == StateReprogramming )
        {
            Building *building = g_app->m_location->GetBuilding( m_buildingId );
            ControlTower *ct = (ControlTower *) building;
            if( ct )
            {
                ct->EndReprogram( m_positionId );
                g_app->m_soundSystem->StopAllSounds( m_id, "Engineer BeginReprogramming" );
                g_app->m_soundSystem->TriggerEntityEvent( this, "EndReprogramming" );
            }
        }

        // If I was researching something, stop now
        if( m_state == StateResearching )
        {
            g_app->m_soundSystem->StopAllSounds( m_id, "Engineer BeginReprogramming" );
            g_app->m_soundSystem->TriggerEntityEvent( this, "EndReprogramming" );
        }

        // If I was operating a bridge, stop now
        if( m_state == StateOperatingBridge )
        {
            Bridge *bridge = (Bridge *) g_app->m_location->GetBuilding( m_buildingId );
            if( bridge && bridge->m_type == Building::TypeBridge )
            {
                bridge->EndOperation();
                if( m_bridgeId == m_buildingId ) EndBridge();
            }
        }
    }

    //
    // Update position history

    m_positionHistory.PutDataAtStart( new Vector3( m_pos ) );
    int maxSpirits = GetMaxSpirits();
    while( m_positionHistory.ValidIndex(maxSpirits+1) )
    {
        Vector3 *position = m_positionHistory[maxSpirits+1];
        m_positionHistory.RemoveData(maxSpirits+1);
        delete position;
    }


    //
    // Update spirits

    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        int spiritId = m_spirits[i];
        if( g_app->m_location->m_spirits.ValidIndex(spiritId) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(spiritId);
            if( s && s->m_state == Spirit::StateAttached )
            {
                if( m_positionHistory.ValidIndex(i+1) )
                {
                    s->m_pos = *m_positionHistory[i+1];
                    s->m_vel = (*m_positionHistory[i] - *m_positionHistory[i+1]) / SERVER_ADVANCE_PERIOD;
                }
            }
        }
    }


    //
    // Look to see if we've been pulled away by the player

    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( building && building->m_type == Building::TypeControlTower &&
        m_positionId != -1 && m_state != StateReprogramming )
    {
        // We've been moved away from reprogramming
        // (eg user specified waypoint)
        ControlTower *ct = (ControlTower *) building;
        ct->EndReprogram( m_positionId );
        m_positionId = -1;
        g_app->m_soundSystem->StopAllSounds( m_id, "Engineer BeginReprogramming" );
        g_app->m_soundSystem->TriggerEntityEvent( this, "EndReprogramming" );
    }

    if( building && building->m_type == Building::TypeBridge
        && m_state != StateOperatingBridge )
    {
        // We've been moved away from building bridges
        Bridge *bridge = (Bridge *) building;
        bridge->EndOperation();
    }

    if( building && building->m_type == Building::TypeResearchItem
        && m_state != StateResearching )
    {
        // We've been moved away from researching a research item
        g_app->m_soundSystem->StopAllSounds( m_id, "Engineer BeginReprogramming" );
        //g_app->m_soundSystem->TriggerEntityEvent( this, "EndReprogramming" );
    }


    //
    // If we own a bridge, check to make sure we are still in range of it

    Bridge *bridge = (Bridge *) g_app->m_location->GetBuilding( m_bridgeId );
    if( bridge && bridge->m_type == Building::TypeBridge )
    {
        float range = ( bridge->m_pos - m_pos ).Mag();
        if( range > 100.0f )
        {
            EndBridge();
        }
    }

    return amIDead;
}


bool Engineer::AdvanceToTargetPos()
{
    Vector3 distance = (m_targetPos - m_pos);
    distance.y = 0.0f;

    if( distance.Mag() > 5.0f )
    {
        Vector3 oldPos = m_pos;

        float amountToTurn = SERVER_ADVANCE_PERIOD * 3.0f;
        Vector3 targetDir = (m_targetPos - m_pos).Normalise();
        Vector3 actualDir = m_front * (1.0f - amountToTurn) + targetDir * amountToTurn;
        actualDir.Normalise();

        float desiredSpeed = distance.Mag();
        desiredSpeed = min( desiredSpeed, m_stats[StatSpeed] );
        if( m_state == StateIdle ) desiredSpeed *= 0.25f;

        Vector3 newPos = m_pos + actualDir * desiredSpeed * SERVER_ADVANCE_PERIOD;
        newPos = PushFromObstructions(newPos);

        Vector3 moved = newPos - oldPos;
        if( moved.Mag() > desiredSpeed * SERVER_ADVANCE_PERIOD ) moved.SetLength( desiredSpeed * SERVER_ADVANCE_PERIOD );
        newPos = m_pos + moved;

        m_pos = newPos;
        float targetHeight = max(g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z),
                                 0.0f ) + m_hoverHeight;
        m_pos.y = targetHeight;
        m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;
        m_front = m_vel;
        m_front.Normalise();

        return false;
    }
    else
    {
        m_vel.Zero();
        //if( m_targetFront != g_zeroVector ) m_front = m_targetFront;
        return true;
    }
}


bool Engineer::SearchForRandomPosition()
{
    float distance = 30.0f;
    float angle = syncsfrand(2.0f * M_PI);

    m_targetPos = m_pos + Vector3( sinf(angle) * distance,
                                   0.0f,
                                   cosf(angle) * distance );

    return true;
}


bool Engineer::AdvanceIdle()
{
    m_retargetTimer -= SERVER_ADVANCE_PERIOD;
    if( m_retargetTimer <= 0.0f )
    {
        bool foundNewTarget = false;

        if( !foundNewTarget )   foundNewTarget = SearchForResearchItems();
        if( !foundNewTarget )   foundNewTarget = SearchForControlTowers();
        if( !foundNewTarget )   foundNewTarget = SearchForBridges();
		if ( !(g_app->m_location->m_levelFile->m_teamFlags[m_id.GetTeamId()] & TEAM_FLAG_SOULLESS) ) {
			if( !foundNewTarget )   foundNewTarget = SearchForSpirits();
		}
        if( foundNewTarget ) return false;
        m_retargetTimer = ENGINEER_RETARGETTIMER;
    }

    //
    // Nothing to do...do we have some spirits that need dropping off?

    if( m_spirits.Size() > 0 )
    {
        m_buildingId = -1;                  // Force the engineer to re-evaluate which is the nearest incubator
        bool incubatorFound = SearchForIncubator();
        if( incubatorFound )
        {
            m_state = StateToIncubator;
            return false;
        }
    }


    //
    // Just go into a holding pattern

    bool arrived = AdvanceToTargetPos();
    if( arrived )
    {
        SearchForRandomPosition();
    }

    return false;
}


bool Engineer::AdvanceToWaypoint()
{
    m_targetPos = m_wayPoint;
    m_targetFront.Zero();

    bool arrived = AdvanceToTargetPos();
    if( arrived ) m_state = StateIdle;
    return false;
}


bool Engineer::AdvanceToSpirit()
{
    Spirit *s = NULL;
    if( g_app->m_location->m_spirits.ValidIndex(m_spiritId) )
    {
        s = g_app->m_location->m_spirits.GetPointer(m_spiritId);
    }

    if( !s ||
         s->m_state == Spirit::StateDeath ||
         s->m_state == Spirit::StateAttached )
    {
        // Our spirit died while we were going for it, return to waypoint and continue looking
        m_spiritId = -1;
        m_state = StateToWaypoint;
        return false;
    }

    m_targetPos = s->m_pos;
    m_targetFront.Zero();
    bool arrived = AdvanceToTargetPos();
    if( arrived )
    {
        CollectSpirit( m_spiritId );
        m_spiritId = -1;
        m_state = StateIdle;
    }

    return false;
}


void Engineer::CollectSpirit( int _spiritId )
{
    if( g_app->m_location->m_spirits.ValidIndex(_spiritId) )
    {
        Spirit *spirit = g_app->m_location->m_spirits.GetPointer(_spiritId);
		if ( spirit->m_state == Spirit::StateBirth || spirit->m_state == Spirit::StateFloating ) {
			spirit->CollectorArrives();
			m_spirits.PutData( _spiritId );
		}
    }
}



bool Engineer::SearchForIncubator( bool _includeAllies )
{
    //
    // Source building lost, look for another incubator to bind to
    float nearest = 99999.9f;
    bool found = false;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];

			// Stage 1: Its always friendly if its ours
			bool isFriendly = (building->m_id.GetTeamId() == m_id.GetTeamId());
			// Stage 2: If we are the player and its our spawn team and its an ally, its friendly
			if ( !isFriendly && m_id.GetTeamId() == 2 )
			{
				if ( g_app->m_location->m_levelFile->m_teamFlags[building->m_id.GetTeamId()] & TEAM_FLAG_PLAYER_SPAWN_TEAM ) {
					isFriendly = g_app->m_location->IsFriend( building->m_id.GetTeamId(), m_id.GetTeamId() );
				}
			}
			// Stage 3: On the second pass, include all allied structures as friends
			if ( !isFriendly && _includeAllies ) {
				isFriendly = g_app->m_location->IsFriend( building->m_id.GetTeamId(), m_id.GetTeamId() );
			}

            if( building->m_type == Building::TypeIncubator && isFriendly )
            {
                float distance = ( building->m_pos - m_pos ).Mag();
                int population = ((Incubator *)building)->NumSpiritsInside();
                distance += population * 10;

                if( distance < nearest )
                {
                    m_buildingId = building->m_id.GetUniqueId();
                    nearest = distance;
                    found = true;
                }
            }

            if( (building->m_type == Building::TypeSpawnPoint || building->m_type == Building::TypeSpawnPointRandom) &&
				g_app->m_location->m_levelFile->m_teamFlags[m_id.GetTeamId()] & TEAM_FLAG_SPAWNPOINTINCUBATION &&
                isFriendly )
            {
                double distance = ( building->m_pos - m_pos ).Mag();
                if( distance < nearest )
                {
                    m_buildingId = building->m_id.GetUniqueId();                    
                    nearest = distance;
                    found = true;
                }                    
            }
        }
	}

	if ( !found && !_includeAllies ) { return SearchForIncubator(true); }
    return found;
}


bool Engineer::AdvanceToIncubator()
{
    Building *building = (Building *) g_app->m_location->GetBuilding( m_buildingId );

    if( !building )
    {
        bool found = SearchForIncubator();
        building = (Building *) g_app->m_location->GetBuilding( m_buildingId );
        if( !building )
        {
            // We can't find a friendly incubator, so go into holding pattern
            m_state = StateIdle;
            return false;
        }
    }

	if( building && building->m_type == Building::TypeIncubator )
    {
        Incubator *incubator = (Incubator *) building;
        incubator->GetDockPoint( m_targetPos, m_targetFront );

		bool arrived = AdvanceToTargetPos();
		if( arrived )
		{
			m_front = m_targetFront;

			// Arrived at our incubator, drop spirit off here one at a time
			int spiritId = m_spirits[0];
			if( g_app->m_location->m_spirits.ValidIndex(spiritId) )
			{
				Spirit *s = g_app->m_location->m_spirits.GetPointer( spiritId );
				incubator->AddSpirit( s );
				g_app->m_location->m_spirits.MarkNotUsed( spiritId );
				m_spirits.RemoveData(0);
			}

			if( m_spirits.Size() == 0 )
			{
				// Return to spirit field
				m_state = StateToWaypoint;
			}
		}
	}

	if( building && (building->m_type == Building::TypeSpawnPoint || building->m_type == Building::TypeSpawnPointRandom) )
    {
        SpawnPoint *spawnPoint = (SpawnPoint *)building;
        m_targetPos = building->m_pos + building->m_front * 50;
        m_targetPos = PushFromObstructions( m_targetPos, false );
        m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );
        m_targetFront = building->m_front;
        
        bool arrived = AdvanceToTargetPos();
        if( arrived )
        {
            m_front = m_targetFront;

            // Arrived at our incubator, drop spirit off here one at a time
            int spiritId = m_spirits[0];
            if( g_app->m_location->m_spirits.ValidIndex(spiritId) )
            {
                Spirit *s = g_app->m_location->m_spirits.GetPointer( spiritId );
                g_app->m_location->m_spirits.MarkNotUsed( spiritId );
                m_spirits.RemoveData(0);

                spawnPoint->SpawnDarwinian();   

                g_app->m_soundSystem->TriggerEntityEvent( this, "DropSpirit" );
			} else { m_spirits.RemoveData(0); }

            if( m_spirits.Size() == 0 )
            {
                // Return to spirit field        
                m_state = StateToWaypoint;
            }
        }    
    }

    return false;
}


bool Engineer::AdvanceToControlTower ()
{
    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( !building )
    {
        m_state = StateIdle;
        return false;
    }

    DarwiniaDebugAssert( building->m_type == Building::TypeControlTower );

    ControlTower *ct = (ControlTower *) building;
    int positionId = ct->GetAvailablePosition( m_targetPos, m_targetFront );

    if( (ct->m_id.GetTeamId() == m_id.GetTeamId() && ct->m_ownership >= 100.0f) ||
        positionId == -1)
    {
        // Our building is gone or is already captured or is full
        m_buildingId = -1;
        m_positionId = -1;
        m_state = StateIdle;
        return false;
    }

    if( positionId != -1 )
    {
        bool arrived = AdvanceToTargetPos();
        if( arrived )
        {
            m_positionId = positionId;
            ct->BeginReprogram( m_positionId );
            g_app->m_soundSystem->TriggerEntityEvent( this, "BeginReprogramming" );
            m_state = StateReprogramming;
        }
    }
    else
    {
        m_state = StateIdle;
    }

    return false;
}


bool Engineer::AdvanceResearching()
{
    //
    // Make sure our research item is still available

    ResearchItem *item = (ResearchItem *) g_app->m_location->GetBuilding( m_buildingId );
    if( !item ||
        item->m_type != Building::TypeResearchItem ||
        !item->NeedsReprogram() )
    {
        g_app->m_soundSystem->StopAllSounds( m_id, "Engineer BeginReprogramming" );

        m_buildingId = -1;
        m_state = StateIdle;
        return false;
    }


    //
    // Do some reprogramming

    bool amIDone = item->Reprogram( m_id.GetTeamId() );

    if( amIDone )
    {
        g_app->m_soundSystem->StopAllSounds( m_id, "Engineer BeginReprogramming" );
        g_app->m_soundSystem->TriggerEntityEvent( this, "ReprogrammingComplete" );
        m_buildingId = -1;
        m_state = StateIdle;
        return false;
    }


    //
    // Spark

    Vector3 end1, end2;
    item->GetEndPositions( end1, end2 );
    float time = g_gameTime + m_id.GetIndex();
    Vector3 toPos = end1;
    toPos += ( end2 - end1 )/2.0f;
    toPos += ( end2 - end1 ) * sinf(time) * 0.5f;

    for( int i = 0; i < 2; ++i )
    {
        Vector3 particleVel = m_pos - toPos;
        particleVel += Vector3( sfrand() * 15.0f, frand() * 10.0f, sfrand() * 15.0f );
        g_app->m_particleSystem->CreateParticle( toPos, particleVel, Particle::TypeBlueSpark );
    }


    //
    // Make us float around a bit while we work

    m_targetFront = (toPos - m_pos).Normalise();
    Vector3 targetFront = m_targetFront;
    m_front = (targetFront * SERVER_ADVANCE_PERIOD) +
              (m_front * (1.0f - SERVER_ADVANCE_PERIOD) );

    Vector3 oldPos = m_pos;
    Vector3 targetPos = m_targetPos;
    Vector3 rightAngle = m_targetFront ^ g_upVector;
    float scale = 2.0f;
    targetPos += m_targetFront * sinf(g_gameTime+m_id.GetUniqueId()*10.0f) * scale;
    targetPos += rightAngle * cosf(g_gameTime+m_id.GetUniqueId()*10.0f) * scale * 1.5f;

    m_pos = (targetPos * SERVER_ADVANCE_PERIOD) +
            (m_pos * (1.0f - SERVER_ADVANCE_PERIOD) );
    m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

    return false;
}


bool Engineer::AdvanceReprogramming()
{
    Building *building = g_app->m_location->GetBuilding( m_buildingId );

    if( !building )
    {
        m_state = StateIdle;
        g_app->m_soundSystem->TriggerEntityEvent( this, "EndReprogramming" );
        return false;
    }

    ControlTower *ct = (ControlTower *) building;

    for( int i = 0; i < 10; ++i )
    {
        bool finished = ct->Reprogram( m_id.GetTeamId() );
        if( finished )
        {
            g_app->m_soundSystem->StopAllSounds( m_id, "Engineer BeginReprogramming" );
            g_app->m_soundSystem->TriggerEntityEvent( this, "ReprogrammingComplete" );
            ct->EndReprogram( m_positionId );
            m_buildingId = -1;
            m_positionId = -1;
            m_state = StateIdle;
            return false;
        }
    }


    //
    // Make us float around a bit while we work

    Vector3 consolePos;
    ct->GetConsolePosition( m_positionId, consolePos );

    Vector3 targetFront = (consolePos - m_pos).Normalise();
    m_front = (targetFront * SERVER_ADVANCE_PERIOD) +
              (m_front * (1.0f - SERVER_ADVANCE_PERIOD) );

    Vector3 oldPos = m_pos;
    Vector3 targetPos = m_targetPos;
    Vector3 rightAngle = m_targetFront ^ g_upVector;
    float scale = 2.0f;
    targetPos += m_targetFront * sinf(g_gameTime+m_id.GetUniqueId()*10.0f) * scale;
    targetPos += rightAngle * cosf(g_gameTime+m_id.GetUniqueId()*10.0f) * scale * 1.5f;

    m_pos = (targetPos * SERVER_ADVANCE_PERIOD) +
            (m_pos * (1.0f - SERVER_ADVANCE_PERIOD) );
    m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

    return false;
}


void Engineer::BeginBridge( Vector3 _to )
{
    int engineerLevel = g_app->m_globalWorld->m_research->CurrentLevel( m_id.GetTeamId(), GlobalResearch::TypeEngineer );
    if( engineerLevel < 5 ) return;

    //
    // Shut down any existing bridges

    EndBridge();


    //
    // Calculate the size of our bridge

    Vector3 bridgeSpan( _to - m_wayPoint );
    int numComponents = int(bridgeSpan.Mag() / 80.0f);
    Vector3 componentSpan = bridgeSpan / (float) numComponents;

    //
    // Create bridge towers

    int linkBuildingId = -1;

    for( int i = numComponents; i >= 0; --i )
    {
        Bridge *component = (Bridge *) Building::CreateBuilding( Building::TypeBridge );
        g_app->m_location->m_buildings.PutData(component);
        component->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );
        component->m_nextBridgeId = linkBuildingId;
        component->m_pos = m_wayPoint + componentSpan * (float) i;
        component->m_pos += Vector3( syncsfrand(15.0f),0.0f,syncsfrand(15.0f) );
        component->m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( component->m_pos.x, component->m_pos.z );
        component->m_front = ( _to - m_wayPoint ).Normalise();
        if( i < numComponents && i > 0 )
        {
            component->m_front.RotateAroundY( 0.5f * M_PI );
        }
        else if( i == numComponents )
        {
            component->m_front.RotateAroundY( M_PI );
        }
        component->m_id.SetTeamId( m_id.GetTeamId() );
        linkBuildingId = component->m_id.GetUniqueId();

        Vector3 right = component->m_front ^ g_upVector;
        component->m_front = right ^ g_upVector;

        if( i == numComponents || i == 0 )
        {
            component->SetBridgeType( Bridge::BridgeTypeEnd );
        }
        else
        {
            component->SetBridgeType( Bridge::BridgeTypeTower );
        }

        if( i == 0 ) m_bridgeId = component->m_id.GetUniqueId();
    }

    g_app->m_location->m_obstructionGrid->CalculateAll();
}


void Engineer::EndBridge()
{
    Bridge *bridge = (Bridge *) g_app->m_location->GetBuilding( m_bridgeId );

    while( bridge )
    {
        bridge->m_status = -1.0f;
        int nextBuildingId = bridge->m_nextBridgeId;
        bridge = (Bridge *) g_app->m_location->GetBuilding( nextBuildingId );
    }

    m_bridgeId = -1;
}


bool Engineer::AdvanceToBridge()
{
    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( building && building->m_type == Building::TypeBridge )
    {
        Bridge *bridge = (Bridge *) building;
        bool positionAvailable = bridge->GetAvailablePosition( m_targetPos, m_targetFront );
        if( !positionAvailable )
        {
            m_state = StateIdle;
            m_buildingId = -1;
        }
        else
        {
            bool arrived = AdvanceToTargetPos();
            if( arrived )
            {
                m_state = StateOperatingBridge;
                m_vel.Zero();
                bridge->BeginOperation();
            }
        }
    }
    else
    {
        m_state = StateIdle;
    }

    return false;
}


bool Engineer::AdvanceOperatingBridge ()
{
    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( building && building->m_type == Building::TypeBridge )
    {
        Vector3 front;
        Bridge *bridge = (Bridge *) building;
        // Nothing to do really
    }
    else
    {
        m_buildingId = -1;
        m_state = StateIdle;
    }

    return false;
}


bool Engineer::AdvanceToResearchItem()
{
    //
    // Make sure our research item is still available

    ResearchItem *item = (ResearchItem *) g_app->m_location->GetBuilding( m_buildingId );
    if( !item || !item->NeedsReprogram() )
    {
        m_buildingId = -1;
        m_state = StateIdle;
        return false;
    }


    bool arrived = AdvanceToTargetPos();
    if( arrived )
    {
        g_app->m_soundSystem->TriggerEntityEvent( this, "BeginReprogramming" );
        m_state = StateResearching;
    }

    return false;
}


void Engineer::RenderShape( float predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * predictionTime;
    if( m_onGround )
    {
        predictedPos.y = max(g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z ),
                             0.0f /*sea level*/) + m_hoverHeight;
    }

    Vector3 entityUp = g_upVector;
    Vector3 entityFront = m_front;
    entityFront.y *= 0.5f;
    entityFront.Normalise();
    Vector3 entityRight = entityFront ^ entityUp;
    entityUp = entityRight ^ entityFront;

	g_app->m_renderer->SetObjectLighting();

    glEnable        (GL_CULL_FACE);
    glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_COLOR_MATERIAL);
    glDisable       (GL_BLEND);

	if (entityFront.y > 0.5f)
	{
		entityFront.Set(1,0,0);
	}
    Matrix34 mat(entityFront, entityUp, predictedPos);
	m_shape->Render(predictionTime, mat);

    glEnable        (GL_BLEND);
    glDisable       (GL_COLOR_MATERIAL);
    glEnable        (GL_TEXTURE_2D);
	g_app->m_renderer->UnsetObjectLighting();
    glEnable        (GL_CULL_FACE);

    g_app->m_renderer->MarkUsedCells(m_shape, mat);

}


void Engineer::Render( float predictionTime )
{
    //
    // Work out our predicted position and orientation

    Vector3 predictedPos = m_pos + m_vel * predictionTime;
    if( m_onGround )
    {
        predictedPos.y = max(g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z ),
                             0.0f /*sea level*/) + m_hoverHeight;
    }


    if( !m_dead )
    {
		RenderShape( predictionTime );

        BeginRenderShadow();
        RenderShadow( predictedPos, 10.0f );
        EndRenderShadow();
    }

    //
    // If we are reprogramming, draw some control lasers

    if( m_state == StateReprogramming ||
        m_state == StateOperatingBridge ||
        m_state == StateResearching )
    {
        Vector3 fromPos = m_pos;
        Vector3 toPos;

        Building *building = g_app->m_location->GetBuilding( m_buildingId );
        if( building )
        {
            if( building->m_type == Building::TypeControlTower )
            {
                ControlTower *tower = (ControlTower *)building;
                tower->GetConsolePosition( m_positionId, toPos );
            }
            else if( building->m_type == Building::TypeBridge )
            {
                Vector3 front;
                Bridge *bridge = (Bridge *) building;
                bridge->GetAvailablePosition( toPos, front );
            }
            else if( building->m_type == Building::TypeResearchItem )
            {
                ResearchItem *item = (ResearchItem *) building;
                Vector3 end1, end2;
                item->GetEndPositions( end1, end2 );
                float time = g_gameTime + m_id.GetIndex();
                toPos = end1;
                toPos += ( end2 - end1 )/2.0f;
                toPos += ( end2 - end1 ) * sinf(time) * 0.5f;
            }


            Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0f;
            Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
            Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();

            rightAngle *= 0.5f;

            glColor4f( 0.2f, 0.4f, 1.0f, fabs(sinf(g_gameTime * 3.0f)) );

            glEnable        ( GL_BLEND );
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
            glDepthMask     ( false );
            glEnable        ( GL_TEXTURE_2D );
            glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );

            glBegin( GL_QUADS );
                glTexCoord2i(0,0);      glVertex3fv( (fromPos - rightAngle).GetData() );
                glTexCoord2i(0,1);      glVertex3fv( (fromPos + rightAngle).GetData() );
                glTexCoord2i(1,1);      glVertex3fv( (toPos + rightAngle).GetData() );
                glTexCoord2i(1,0);      glVertex3fv( (toPos - rightAngle).GetData() );
            glEnd();

            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glDisable       ( GL_TEXTURE_2D );
            glDepthMask     ( true );
            glDisable       ( GL_BLEND );
        }
    }
}


bool Engineer::RenderPixelEffect( float predictionTime )
{
    RenderShape( predictionTime );

    return true;
}


char *Engineer::GetCurrentAction()
{
    switch( m_state )
    {
        case StateIdle:                     return LANGUAGEPHRASE( "engineer_idle" );
        case StateToWaypoint:               return LANGUAGEPHRASE( "engineer_towaypoint" );
        case StateToSpirit:                 return LANGUAGEPHRASE( "engineer_tospirit" );
        case StateToIncubator:              return LANGUAGEPHRASE( "engineer_toincubator" );

        case StateToControlTower:
        case StateReprogramming:            return LANGUAGEPHRASE( "engineer_reprogramming" );

        case StateToResearchItem:
        case StateResearching:              return LANGUAGEPHRASE( "engineer_researching" );

        case StateToBridge:
        case StateOperatingBridge:          return LANGUAGEPHRASE( "engineer_bridge" );
    }

    return LANGUAGEPHRASE( "engineer_idle" );
}


void Engineer::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "BeginReprogramming" );
    _list->PutData( "EndReprogramming" );
    _list->PutData( "ReprogrammingComplete" );
}
