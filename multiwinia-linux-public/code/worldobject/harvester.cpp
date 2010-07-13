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
#include "lib/math/random_number.h"
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
#include "worldobject/engineer.h"
#include "worldobject/harvester.h"
#include "worldobject/weapons.h"
#include "worldobject/spawnpoint.h"

#define HARVESTER_COLLECTION_RANGE 75.0f

Harvester::Harvester()
:   Entity(),
    m_hoverHeight(50.0f),
    m_state(StateIdle),
    m_retargetTimer(0.0f),
    m_positionId(-1),
    m_vunerable(false),
    m_movedThisAdvance(false),
    m_fieldFound(false)
{

    m_shape = g_app->m_resource->GetShape( "harvester.shp" );

    Matrix34 mat( Vector3(0,0,1), g_upVector, g_zeroVector );
    m_centrePos = m_shape->CalculateCentre(mat);
    m_radius = m_shape->CalculateRadius( mat, m_centrePos );
}


Harvester::~Harvester()
{
    if( m_id.GetTeamId() != 255 )
    {
        Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        team->UnRegisterSpecial( m_id );
    }
}


void Harvester::Begin()
{
    Entity::Begin();
    m_wayPoint = m_pos;
    m_targetPos = m_wayPoint;

    SetShape( "harvester.shp", true );

    if( m_id.GetTeamId() != 255 )
    {
        Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        team->RegisterSpecial( m_id );
    }
}


void Harvester::SetWaypoint( Vector3 const &_wayPoint )
{
    m_retargetTimer = 0.0f;
    m_wayPoint = _wayPoint;
    m_state = StateToWaypoint;
}


int Harvester::GetNumSpirits()
{
    return m_spirits.Size();
}


int Harvester::GetMaxSpirits()
{
    int harvesterLevel = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeHarvester );
    switch( harvesterLevel )
    {
        case 0 :        return 0;
        case 1 :        return 25;
        case 2 :        return 50;
        case 3 :        return 100;
        case 4 :        return 200;
    }

    return 0;
}


bool Harvester::SearchForSpirits()
{   
    if( m_spirits.Size() < GetMaxSpirits() )
    {
        Spirit *found = NULL;
        Vector3 pos;
        double closest = 999999.9;

        for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
        {
            if( g_app->m_location->m_spirits.ValidIndex(i) )
            {
                Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
                double theDist = ( s->m_pos - m_pos ).Mag();

                if( theDist <= ENGINEER_SEARCHRANGE &&
                    theDist < closest &&
                    s->m_state == Spirit::StateFloating &&
                    s->m_broken < 0.0f )
                {
                    found = s;
                    pos = s->m_pos;
                    closest = theDist;
                }
            }
        }            

        if( found )
        {
            m_targetPos = pos;
            m_state = StateToSpirit;
            return true;
        }
    }
        
    return false;
}

bool Harvester::ChangeHealth( int amount, int _damageType )
{
    if( !m_vunerable ) return false;
    if( _damageType != DamageTypeUnresistable ) return false;

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

        int healthBandAfter = int( m_stats[StatHealth] / 50.0 );

        double fractionDead = 1.0 - m_stats[StatHealth] / (double) EntityBlueprint::GetStat( TypeEngineer, StatHealth );
        fractionDead = max( fractionDead, 0.0 );
        fractionDead = min( fractionDead, 1.0 );
        
        if( fractionDead == 1.0 || healthBandAfter < healthBandBefore )
        {
            Matrix34 transform( m_front, g_upVector, m_pos );
            g_explosionManager.AddExplosion( m_shape, transform, fractionDead ); 
        }
    }

    return true;
}


bool Harvester::Advance( Unit *_unit )
{   
    bool amIDead = false;

    m_movedThisAdvance = false;
    
    switch( m_state )
    {
        case StateIdle:                 amIDead = AdvanceIdle();             break;
        case StateToWaypoint:           amIDead = AdvanceToWaypoint();       break;
        case StateToSpirit:             amIDead = AdvanceToSpirit();         break;
        case StateToIncubator:          amIDead = AdvanceToIncubator();      break;
        case StateHarvesting:           amIDead = AdvanceHarvesting();       break;
        case StateToSpawnPoint:         amIDead = AdvanceToSpawnPoint();     break;
    };

    AdvanceSpirits();
    AdvanceGrenades();
    AdvanceEngines();

    if( !m_movedThisAdvance )
    {
        // harvester hasnt moved because its harvesting or at an incubator, so make it float up and down
        Vector3 oldPos = m_pos;
        double targetHeight = max(g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z),
                                 0.0 ) + GetHoverHeight();
        m_pos.y = targetHeight;
        m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;
    }

    return Entity::Advance(_unit);
}


void Harvester::AdvanceEngines()
{
    Matrix34 mat( m_front, g_upVector, m_pos );
    for( int e = 1; e < 5; ++e )
    {
        char markerName[128];
        sprintf( markerName, "MarkerEngine%d", e );
        ShapeMarker *engine = m_shape->m_rootFragment->LookupMarker( markerName );

        if( !engine ) continue;
        Vector3 pos = engine->GetWorldMatrix( mat ).pos;            

        for( int i = 0; i < 5; ++i )
        {
            Vector3 vel( sfrand(10), -10-frand(20), sfrand(10) );
            float size = 75.0f;

            g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileFire, size, RGBAColour( 50, 50, 250 ) );            
        }

        pos -= g_upVector * 5;
        pos += Vector3(sfrand(10), sfrand(10), sfrand(10) );
        float size = 50 + sfrand(20);
        g_app->m_particleSystem->CreateParticle( pos, g_upVector*-4, Particle::TypeMissileTrail, size );
    }
}


void Harvester::AdvanceSpirits()
{
    double baseSpeed = 40.0;
    double groundHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    double timeIndex = GetNetworkTime() + m_id.GetUniqueId() * 20;

    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        int spiritId = m_spirits[i];
        if( g_app->m_location->m_spirits.ValidIndex(spiritId) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(spiritId);
            if( s && s->m_state == Spirit::StateHarvested )
            {
                Vector3 dist = (m_pos - s->m_pos );
                dist.y = 0.0;

                double speedModifier = (1.0 - ( dist.Mag() / (HARVESTER_COLLECTION_RANGE * 2.0)));
                double speed = baseSpeed * speedModifier;
                Vector3 targetPos = m_pos;

                double heightScale = (m_hoverHeight - (s->m_pos.y - groundHeight)) / m_hoverHeight;
                double radius = HARVESTER_COLLECTION_RANGE * heightScale;

                //double radius = (dist.Mag() * (groundHeight / s->m_pos.y ));

                if( m_pos.y - s->m_pos.y < 1.0 )
                {
                    //radius = 0.0;
                    speed = baseSpeed / 2.0;
                }

                targetPos.x += iv_cos( timeIndex + i ) * radius;
                targetPos.z += iv_sin( timeIndex + i ) * radius;
                targetPos.y = m_pos.y;

                Vector3 oldPos = s->m_pos;
                Vector3 actualDir = (targetPos - s->m_pos).Normalise();

                Vector3 newPos = s->m_pos + (actualDir * speed * SERVER_ADVANCE_PERIOD);
                Vector3 moved = newPos - oldPos;
                if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
                //s->m_pos = s->m_pos + moved;
                s->m_vel = ((s->m_pos + moved) - oldPos) / SERVER_ADVANCE_PERIOD;

                bool inRange = (s->m_pos - targetPos).Mag() < 2.0;
                if( !inRange )
                {
                    if( iv_abs(s->m_pos.y - targetPos.y) < 5.0 &&
                        (s->m_pos - targetPos).Mag() < 50.0 )
                    {
                        // we're inside the harvester and might be stuck spinning around it, so just makes ure this can't happen
                        inRange = true;
                    }
                }

                if( inRange )
                {
                    Vector3 offset;
                    offset.x = syncsfrand(7.5);
                    offset.y = syncsfrand(7.5);
                    offset.z = syncsfrand(7.5);

                    s->m_state = Spirit::StateCollected;
                    s->m_pos = GetCollectorPos() + offset;
                    s->m_vel.Zero();
                }
            }
        }
    }
}

void Harvester::AdvanceGrenades()
{
    double baseSpeed = 40.0;
    double groundHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    double timeIndex = GetNetworkTime() + m_id.GetUniqueId() * 20;


    for( int i = 0; i < m_grenades.Size(); ++i )
    {
        if( g_app->m_location->m_effects.ValidIndex(m_grenades[i]) )
        {
            Grenade *g = (Grenade *)g_app->m_location->m_effects[m_grenades[i]];
            if( g->m_type != EffectThrowableGrenade ) continue;

            Vector3 targetPos = m_pos;

            Vector3 dist = (m_pos - g->m_pos );
            dist.y = 0.0;

            double speedModifier = (1.0 - ( dist.Mag() / (HARVESTER_COLLECTION_RANGE * 2.0)));
            double speed = baseSpeed * speedModifier;

            //double radius = (dist.Mag() * (groundHeight / g->m_pos.y ));

            double heightScale = (m_hoverHeight - (g->m_pos.y - groundHeight)) / m_hoverHeight;
            double radius = HARVESTER_COLLECTION_RANGE * heightScale;

            if( m_pos.y - g->m_pos.y < 2.0 )
            {
                //radius = 0.0;
                speed = baseSpeed / 2.0;
            }

            targetPos.x += iv_cos( timeIndex + i * 50.0 ) * radius;
            targetPos.z += iv_sin( timeIndex + i * 50.0 ) * radius;

            Vector3 oldPos = g->m_pos;
            Vector3 actualDir = (targetPos - g->m_pos).Normalise();

            Vector3 newPos = g->m_pos + (actualDir * speed * SERVER_ADVANCE_PERIOD);
            Vector3 moved = newPos - oldPos;
            if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
            g->m_pos = g->m_pos + moved;
            g->m_vel = (g->m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

            if( (g->m_pos - targetPos).Mag() < 2.0f )
            {
                m_vunerable = true;
                g->m_life = 0.0f;
                g->m_collected = false;
                ChangeHealth(-999);
                ReleaseSpirits();
                ReleaseGrenades();
                break;
            }
        }
    }
}

void Harvester::RunAI(AI *_ai)
{
    m_aiTimer -= SERVER_ADVANCE_PERIOD;
    if( m_aiTimer > 0.0f ) return;

    m_aiTimer = 1.0f;

    if( m_state == StateIdle )
    {
        double dist = FLT_MAX;
        Vector3 pos;
        for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
        {
            Spirit *s = g_app->m_location->GetSpirit(i);
            if( s )
            {
                if( (m_pos - s->m_pos).Mag() < dist )
                {
                    dist = (m_pos - s->m_pos).Mag();
                    pos = s->m_pos;
                }
            }
        }

        if( pos != g_zeroVector )
        {
            SetWaypoint( pos );
        }
    }
}

void Harvester::ReleaseSpirits()
{
    // Drop all my spirits
    while( m_spirits.Size() > 0 )
    {
        int spiritId = m_spirits[0];
        if( g_app->m_location->m_spirits.ValidIndex(spiritId) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(spiritId);
            s->Begin();
            m_spirits.RemoveData(0);
            s->m_vel = (s->m_pos - m_pos);
            s->m_vel.SetLength( syncfrand(150.0f) );
        }
    }
}

void Harvester::ReleaseGrenades()
{
    while( m_grenades.Size() > 0 )
    {
        int id = m_grenades[0];
        if( g_app->m_location->m_effects.ValidIndex(id) )
        {
            Grenade *e = (Grenade *)g_app->m_location->m_effects[id];
            e->m_collected = false;
            e->ResetForce();
            m_grenades.RemoveData(0);
        }
        else
        {
            m_grenades.RemoveData(0);
        }
    }
}

bool Harvester::AdvanceToTargetPos()
{
    Vector3 distance = (m_targetPos - m_pos);
    distance.y = 0.0f;

    if( distance.Mag() > 5.0f )
    {
        m_movedThisAdvance = true;

        Vector3 oldPos = m_pos;

        double amountToTurn = SERVER_ADVANCE_PERIOD * 9.5;
        double desiredSpeed = distance.Mag();
        desiredSpeed = min( desiredSpeed, (double)m_stats[StatSpeed] );

        if( m_state == StateIdle ) 
        {
            desiredSpeed = m_stats[StatSpeed] * 0.25;
            amountToTurn = SERVER_ADVANCE_PERIOD * 2.0;
        }

        Vector3 targetDir = (m_targetPos - m_pos).Normalise();
        Vector3 actualDir = m_front * (1.0f - amountToTurn) + targetDir * amountToTurn;
        actualDir.Normalise();


        Vector3 newPos = m_pos + actualDir * desiredSpeed;
        Vector3 targetVel = (newPos - m_pos).SetLength( desiredSpeed );
    
        double factor1 = SERVER_ADVANCE_PERIOD * 0.8;
        double factor2 = 1.0 - factor1;

        Vector3 vel = m_vel * factor2 + targetVel * factor1;
        
        /*newPos = PushFromObstructions(newPos);

        Vector3 moved = newPos - oldPos;
        if( moved.Mag() > desiredSpeed * SERVER_ADVANCE_PERIOD ) moved.SetLength( desiredSpeed * SERVER_ADVANCE_PERIOD );
        newPos = m_pos + moved;

        m_pos = newPos;*/
        double targetHeight = max(g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z),
                                 0.0 ) + GetHoverHeight();
        m_pos += vel * SERVER_ADVANCE_PERIOD;
        m_pos.y = targetHeight;

        m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

        m_front = m_vel;
        m_front.y = 0.0f;
        m_front.Normalise();

        return false;
    }
    else
    {
        if( m_state == StateIdle )
        {
            return SearchForRandomPosition();
        }
            
        m_vel.Zero();
        //if( m_targetFront != g_zeroVector ) m_front = m_targetFront;
        return true;
    }   
}


bool Harvester::SearchForRandomPosition()
{
    double distance = 30.0;
    double angle = syncsfrand(2.0 * M_PI);

    m_targetPos = m_pos + Vector3( iv_sin(angle) * distance,
                                   0.0,
                                   iv_cos(angle) * distance );

    return true;
}


bool Harvester::AdvanceIdle()
{
    if( m_grenades.Size() > 0 )
    {
        ReleaseGrenades();
    }

    m_retargetTimer -= SERVER_ADVANCE_PERIOD;
    if( m_retargetTimer <= 0.0f )
    {
        bool foundNewTarget = false;    
        if( !foundNewTarget )   foundNewTarget = SearchForSpirits();      
       
        if( foundNewTarget ) return false;
        m_retargetTimer = ENGINEER_RETARGETTIMER;
    }

    //
    // Nothing to do...do we have some spirits that need dropping off?

    if( m_spirits.Size() > 0 )
    {
        m_buildingId = -1;                  // Force the engineer to re-evaluate which is the nearest incubator
        bool spawnPointFound = SearchForSpawnPoint();
        if( spawnPointFound )
        {
            m_state = StateToSpawnPoint;
            return false;
        }
        else
        {
            bool incubatorFound = SearchForIncubator();
            if( incubatorFound )
            {
                m_state = StateToIncubator;        
                return false;
            }
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


bool Harvester::AdvanceToWaypoint()
{
    m_targetPos = m_wayPoint;
    m_targetFront.Zero();

    bool arrived = AdvanceToTargetPos();
    if( arrived ) m_state = StateIdle;    
    return false;
}


bool Harvester::AdvanceToSpirit()
{
    int numFound = 0;
    Vector3 totalPos;
    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        Spirit *s = g_app->m_location->GetSpirit(i);
        if( s )
        {
            if( s->m_state == Spirit::StateFloating )
            {
                Vector3 distance = (s->m_pos - m_pos);
                distance.y = 0.0f;
                if( distance.Mag() < HARVESTER_COLLECTION_RANGE )
                {
                    numFound++;
                    totalPos += s->m_pos;
                }
            }
        }
    }

    if( numFound > 30 && !m_fieldFound )
    {
        // we've just stumbled over a large number of spirits, stop to grab them
        m_targetPos = (totalPos) / numFound; 
        m_fieldFound = true;
    }

    bool arrived = AdvanceToTargetPos();
    if( arrived )
    {
        m_fieldFound = false;
        if( numFound == 0  )
        {
            // our spirits are gone
            m_state = StateIdle;
            m_targetPos = m_pos;
            return false;
        }
            
        m_state = StateHarvesting;
    }
    
    return false;
}

bool Harvester::AdvanceHarvesting()
{
    bool spiritFound = false;
    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        Spirit *spirit = g_app->m_location->GetSpirit(i);
        if( spirit )
        {
            if( spirit->m_state == Spirit::StateFloating )
            {
                Vector3 distance = (spirit->m_pos - m_pos);
                distance.y = 0.0f;
                
                if( distance.Mag() < HARVESTER_COLLECTION_RANGE )
                {
                    if( GetNumSpirits() < GetMaxSpirits() )
                    {
                        CollectSpirit( i );
                        spiritFound = true;
                    }
                }
            }
            else if( spirit->m_state == Spirit::StateHarvested )
            {
                spiritFound = true;
                // remain still until all spirits have been fully collected
            }
        }
     }

    // see if there are any grenades in range to be sucked up
    for( int i = 0; i < g_app->m_location->m_effects.Size(); ++i )
    {
        if( g_app->m_location->m_effects.ValidIndex( i ) )
        {
            Grenade *e = (Grenade *)g_app->m_location->m_effects[i];
            if( e && e->m_type == EffectThrowableGrenade )
            {
                Vector3 distance = (e->m_pos - m_pos);
                distance.y = 0.0;
                double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( e->m_pos.x, e->m_pos.z );

                if( spiritFound )
                {
                    if( e->m_pos.y < landHeight + 2.0f )
                    {
                        if( distance.Mag() < HARVESTER_COLLECTION_RANGE / 3.0f ) // grenades should be almost directly under before being sucked up
                        {
                            ((Grenade *)e)->m_collected = true;
                            ((Grenade *)e)->RemoveForce();
                            m_grenades.PutData(i);
                        }
                    }
                }
            }
        }
    }

    if( !spiritFound )
    {
        m_state = StateIdle;
    }

    return false;
}

void Harvester::CollectSpirit( int _spiritId )
{
    if( m_spirits.Size() < GetMaxSpirits() )
    {
        m_spirits.PutData(_spiritId);
        Spirit *spirit = g_app->m_location->GetSpirit( _spiritId );
        spirit->Harvest(m_id);
    }
}

Vector3 Harvester::GetCollectorPos()
{
    Vector3 pos = m_pos;
    pos.y += 15.0f;
    return pos;
}

bool Harvester::SearchForIncubator()
{
    //
    // Source building lost, look for another incubator to bind to
    double nearest = 99999.9;
    bool found = false;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeIncubator &&
                g_app->m_location->IsFriend( building->m_id.GetTeamId(), m_id.GetTeamId() ) )
            {
                double distance = ( building->m_pos - m_pos ).Mag();
                int population = ((Incubator *)building)->NumSpiritsInside();
                distance += population * 10;

                if( distance < nearest )
                {
                    m_buildingId = building->m_id.GetUniqueId();                    
                    nearest = distance;
                    found = true;
                }                    
            }
        }
    }

    return found;
}

bool Harvester::SearchForSpawnPoint()
{
    double nearest = 99999.9;
    bool found = false;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeSpawnPoint &&
                g_app->m_location->IsFriend( building->m_id.GetTeamId(), m_id.GetTeamId() ) )
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

    return found;
}


bool Harvester::AdvanceToIncubator()
{
    Incubator *incubator = (Incubator *) g_app->m_location->GetBuilding( m_buildingId );

    if( !incubator )
    {
        bool found = SearchForIncubator();    
        incubator = (Incubator *) g_app->m_location->GetBuilding( m_buildingId );
        if( !incubator )
        {
            // We can't find a friendly incubator, so go into holding pattern
            m_state = StateIdle;
            return false;
        }
    }

        
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
            g_app->m_location->m_spirits.RemoveData( spiritId );
            m_spirits.RemoveData(0);
        }

        if( m_spirits.Size() == 0 )
        {
            // Return to spirit field        
            m_state = StateToWaypoint;
        }
    }

    return false;
}

bool Harvester::AdvanceToSpawnPoint()
{
    SpawnPoint *sp = (SpawnPoint *) g_app->m_location->GetBuilding( m_buildingId );

    if( !sp || sp->m_type != Building::TypeSpawnPoint )
    {
        bool found = SearchForSpawnPoint();
        sp = (SpawnPoint *) g_app->m_location->GetBuilding( m_buildingId );
        if( !sp || sp->m_id.GetTeamId() != m_id.GetTeamId() )
        {
            // We can't find a friendly incubator, so go into holding pattern
            m_state = StateIdle;
            return false;
        }
    }

        
    m_targetPos = sp->m_pos;
    m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z ) + m_hoverHeight;
    m_targetFront = sp->m_front;

    bool arrived = AdvanceToTargetPos();
    if( arrived )
    {
        m_front = m_targetFront;
        
        int spiritId = m_spirits[0];
        if( g_app->m_location->m_spirits.ValidIndex(spiritId) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer( spiritId );
            g_app->m_location->m_spirits.RemoveData( spiritId );
            m_spirits.RemoveData(0);
            sp->SpawnDarwinian();

            g_app->m_soundSystem->TriggerEntityEvent( this, "DropSpirit" );
        }

        if( m_spirits.Size() == 0 )
        {
            // Return to spirit field        
            m_state = StateToWaypoint;
        }
    }

    return false;
}

void Harvester::RenderShape( double predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * predictionTime;
    if( m_onGround )
    {
        predictedPos.y = max(g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z ),
                             0.0 /*sea level*/) + GetHoverHeight();
    }
 
    Vector3 entityUp = g_upVector;
    Vector3 entityFront = m_front;
    //entityFront.y *= 0.5f;
    entityFront.Normalise();
    Vector3 entityRight = entityFront ^ entityUp;
    entityUp = entityRight ^ entityFront;

	g_app->m_renderer->SetObjectLighting();

    glEnable        (GL_CULL_FACE);
    glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_COLOR_MATERIAL);
    glDisable       (GL_BLEND);

	//if (entityFront.y > 0.5f)
	{
		//entityFront.Set(1,0,0);
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


void Harvester::Render( double predictionTime )
{
    //
    // Work out our predicted position and orientation

    Vector3 predictedPos = m_pos + m_vel * predictionTime;
    if( m_onGround )
    {
        predictedPos.y = max(g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z ),
                             0.0 /*sea level*/) + GetHoverHeight();
    }
        

    if( !m_dead )
    {        
		RenderShape( predictionTime );
        RenderBubble( predictionTime );
        RenderBeam( predictionTime );

        BeginRenderShadow();
        RenderShadow( predictedPos, 10.0f );
        EndRenderShadow();
    }
}


bool Harvester::RenderPixelEffect( double predictionTime )
{
    RenderShape( predictionTime );

    return true;
}

void Harvester::RenderBubble( double _predictionTime )
{
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/bubble.bmp" ) );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
 	glEnable		( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glDepthMask     ( false );    
    glDisable       ( GL_CULL_FACE );
    
    glColor4ub   ( 255, 255, 255, 150 );

    Vector3 pos = GetCollectorPos() + m_vel * _predictionTime;
    pos.y += 3.0f;

    Vector3 up = g_app->m_camera->GetUp();
    Vector3 right = g_app->m_camera->GetRight();
    float size = 16.0f;

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 0 );       glVertex3dv( (pos - right * size - up * size).GetData() );
        glTexCoord2i( 0, 1 );       glVertex3dv( (pos - right * size + up * size).GetData() );
        glTexCoord2i( 1, 1 );       glVertex3dv( (pos + right * size + up * size).GetData() );
        glTexCoord2i( 1, 0 );       glVertex3dv( (pos + right * size - up * size).GetData() );    
    glEnd();
    
    glEnable        ( GL_CULL_FACE );
	glDisable       ( GL_BLEND );
    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}

void Harvester::RenderBeam( double _predicionTime )
{
    if( m_state != StateHarvesting ) return;

    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );
    glDepthMask( false );

    float groundHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    float totalHeight = m_hoverHeight + groundHeight;
    float y = groundHeight + g_gameTime * 10;

    RGBAColour col = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour;

    while( y > totalHeight )
    {
        y -= m_hoverHeight;
    }

    for( int i = 0; i < 5; ++i )
    {
        float heightScale = (m_hoverHeight - (y - groundHeight)) / m_hoverHeight;
        float radius = HARVESTER_COLLECTION_RANGE * heightScale;

        float totalAngle = M_PI * 2;
        int numSteps = totalAngle * 3;

        float angle = -g_gameTime;
        float timeFactor = g_gameTime;
        
        glBegin( GL_QUAD_STRIP );
        for( int i = 0; i <= numSteps; ++i )
        {
            float alpha = 0.2 + (fabs(sinf(angle * 2 + timeFactor)) * 0.8f);
            glColor4ub( col.r, col.g, col.b, alpha * 100 );

            float xDiff = radius * sinf(angle);
            float zDiff = radius * cosf(angle);
            Vector3 pos = m_pos + Vector3(xDiff,5,zDiff);
            pos.y = y ;
            glVertex3dv( pos.GetData() );
            pos.y += 5;
            glVertex3dv( pos.GetData() );
            angle += totalAngle / (double) numSteps;
        }
        glEnd();

        y+= (totalHeight - groundHeight) / 5.0;
        if( y > totalHeight ) 
        {
            y -= m_hoverHeight;
        }
    }

    glEnable( GL_CULL_FACE );
    glShadeModel( GL_FLAT );
    glDepthMask( true );
}

double Harvester::GetHoverHeight()
{
    double timeIndex = GetNetworkTime() + m_id.GetUniqueId() * 20;
    return m_hoverHeight + iv_sin(timeIndex) * (m_hoverHeight * 0.05);
}

char *Harvester::GetCurrentAction()
{
    switch( m_state )
    {
        case StateIdle:                     return  "engineer_idle" ;
        case StateToWaypoint:               return  "engineer_towaypoint" ;        
        case StateToSpirit:                 return  "engineer_tospirit" ;
        case StateToIncubator:              return  "engineer_toincubator" ;
    }

    return  "engineer_idle" ;
}

bool Harvester::IsSelectable()
{
    return true;
}

void Harvester::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "BeginReprogramming" );
    _list->PutData( "EndReprogramming" );
    _list->PutData( "ReprogrammingComplete" );
}
