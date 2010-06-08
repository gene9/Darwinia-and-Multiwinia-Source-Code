#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/matrix34.h"
#include "lib/shape.h"
#include "lib/math_utils.h"
#include "lib/debug_render.h"
#include "lib/profiler.h"
#include "lib/preferences.h"
#include "lib/math/random_number.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "renderer.h"
#include "location.h"
#include "unit.h"
#include "explosion.h"
#include "particle_system.h"
#include "entity_grid.h"
#include "camera.h"
#include "landscape.h"
#include "team.h"
#include "gametimer.h"

#include "worldobject/armyant.h"
#include "worldobject/anthill.h"
#include "worldobject/darwinian.h"


ArmyAnt::ArmyAnt()
:   Entity(),
    m_orders(NoOrders),
    m_spiritId(-1),
    m_targetFound(false)
{   
    m_type = TypeArmyAnt;
}


void ArmyAnt::Begin()
{
    Entity::Begin();

    SetShape();
    
    m_onGround = true;

    m_scale = 1.0 + syncsfrand( 0.9 );

    m_wayPoint = m_pos;
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_wayPoint.x, m_wayPoint.z);

    double speed = m_stats[StatSpeed];
    speed *= ( 1.0 + syncsfrand(0.4) );
    if( speed < 0 ) speed = 0;
    if( speed > 255 ) speed = 255;
    m_stats[StatSpeed] = (unsigned char) speed;
}

void ArmyAnt::SetShape()
{
    for( int i = 0; i < 3; ++i )
    {
        char shapeName[256];
        if( i == 0 ) strcpy( shapeName, "armyant.shp" );
        else sprintf(shapeName, "armyant%d.shp", i+1 );

        char filename[256];
        Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
        int colourId = team->m_lobbyTeam->m_colourId;
        int groupId = team->m_lobbyTeam->m_coopGroupPosition;
        sprintf( filename, "%s_%d_%d", shapeName, colourId, groupId );

        m_shapes[i] = g_app->m_resource->GetShape( filename, false );
        if( !m_shapes[i] )
        {
            m_shapes[i] = g_app->m_resource->GetShapeCopy( shapeName, false, false );
            ConvertShapeColoursToTeam( m_shapes[i], m_id.GetTeamId(), false );
            g_app->m_resource->AddShape( m_shapes[i], filename );
        }
    }
    m_shape = m_shapes[0];

    const char carryMarkerName[] = "MarkerCarry";
    m_carryMarker = m_shape->m_rootFragment->LookupMarker( "MarkerCarry" );
    AppReleaseAssert( m_carryMarker, "ArmyAnt: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", carryMarkerName, m_shape->m_name );
}

bool ArmyAnt::ChangeHealth( int _amount, int _damageType )
{
    bool dead = m_dead;
    
    Entity::ChangeHealth( _amount );
    
    if( m_dead && !dead )
    {
        //
        // We just died

        Matrix34 transform( m_front, g_upVector, m_pos );
        transform.f *= m_scale;
        transform.u *= m_scale;
        transform.r *= m_scale;
        g_explosionManager.AddExplosion( m_shape, transform );   
        

        //
        // Drop any spirits we are carrying

        if( m_spiritId != -1 )
        {
            if( g_app->m_location->m_spirits.ValidIndex(m_spiritId) )
            {
                Spirit *spirit = g_app->m_location->m_spirits.GetPointer( m_spiritId );
                if( spirit && spirit->m_state == Spirit::StateAttached )
                {
                    spirit->CollectorDrops();
                    spirit->m_vel = m_vel;
                }
            }
            m_spiritId = -1;
        }        
    }

    return true;
}


bool ArmyAnt::Advance( Unit *_unit )
{   
    bool amIDead = Entity::Advance( _unit );

    if( !m_onGround ) AdvanceInAir(_unit);

    if( !amIDead && !m_dead && m_onGround )
    {
        switch( m_orders )
        {
            case NoOrders:              OrderReturnToBase();                        break;
            case ScoutArea:             amIDead = AdvanceScoutArea();               break;
            case CollectSpirit:         amIDead = AdvanceCollectSpirit();           break;
            case CollectEntity:         amIDead = AdvanceCollectEntity();           break;
            case AttackEnemy:           amIDead = AdvanceAttackEnemy();             break;
            case ReturnToBase:          amIDead = AdvanceReturnToBase();            break;
            case BaseDestroyed:         amIDead = AdvanceBaseDestroyed();           break;
        }

        if( !m_targetFound ) m_targetFound = SearchForTargets();

        //
        // Keep attached spirits attached to us

        if( g_app->m_location->m_spirits.ValidIndex(m_spiritId) )
        {
            Spirit *spirit = g_app->m_location->m_spirits.GetPointer( m_spiritId );
            if( spirit && spirit->m_state == Spirit::StateAttached )
            {
                Vector3 carryPos, carryVel;
                GetCarryMarker( carryPos, carryVel );
                spirit->m_pos = carryPos;
                spirit->m_vel = carryVel;
            }
        }    
    }
   
    
    //
    // Move the legs on our model

    int currentIndex = -1;
    for( int i = 0; i < 3; ++i )
    {
        if( m_shape == m_shapes[i] ) 
        {
            currentIndex = i;
            break;
        }
    }

    int newIndex = -1;
    while( true )
    {
        newIndex = syncrand() % 3;
        if( newIndex != currentIndex ) break;
    }
    
    m_shape = m_shapes[newIndex];
    
    return amIDead;
}


bool ArmyAnt::AdvanceScoutArea()
{
    bool arrived = AdvanceToTargetPosition();
    if( arrived ) 
    {
        Entity *targetEntity = g_app->m_location->GetEntity( m_targetId );
        if( targetEntity )
        {
            if( targetEntity->m_type == Entity::TypeDarwinian )
            {
                m_orders = CollectEntity;
            }
            else
            {
                m_orders = AttackEnemy;
            }
        }
        else
        {
            OrderReturnToBase();
        }
    }
    return false;
}


bool ArmyAnt::AdvanceCollectSpirit()
{
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
        m_targetFound = false;
        OrderReturnToBase();
        return false;
    }

    m_wayPoint = s->m_pos;
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    bool arrived = AdvanceToTargetPosition();
    if( arrived )
    {
        s->CollectorArrives();
        OrderReturnToBase();
    }

    return false;
}


bool ArmyAnt::AdvanceCollectEntity()
{
    //
    // Is our entity still here

    Entity *targetEntity = g_app->m_location->GetEntity( m_targetId );
    if( !targetEntity || targetEntity->m_dead )
    {
        m_targetId.SetInvalid();
        m_targetFound = false;
        OrderReturnToBase();
        return false;
    }


    //
    // Make sure he is still capturable

    Darwinian *targetDarwinian = (Darwinian *) targetEntity;
    if( targetDarwinian->m_state == Darwinian::StateCapturedByAnt )
    {
        m_targetId.SetInvalid();
        m_targetFound = false;
        OrderReturnToBase();
        return false;
    }
    

    //
    // Go after him

    m_wayPoint = targetEntity->m_pos;
    bool arrived = AdvanceToTargetPosition();
    if( arrived )
    {
        targetDarwinian->AntCapture( m_id );
        OrderReturnToBase();
    }

    return false;
}


bool ArmyAnt::AdvanceAttackEnemy()
{
    //
    // Is our entity still here

    Entity *targetEntity = g_app->m_location->GetEntity( m_targetId );
    if( !targetEntity || targetEntity->m_dead )
    {
        m_targetId.SetInvalid();
        m_targetFound = false;
        OrderReturnToBase();
        return false;
    }


    //
    // Go after him

    m_wayPoint = targetEntity->m_pos;
    
    if( targetEntity->m_type == TypeEngineer )
    {
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    }

    bool arrived = AdvanceToTargetPosition();
    if( arrived )
    {
        targetEntity->ChangeHealth( -1 );
        for( int i = 0; i < 3; ++i )
        {
			Vector3 vel = Vector3(SFRAND(15.0), 15.0, SFRAND(15.0));
			vel.y += syncfrand(15.0);
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeMuzzleFlash );
        }
        g_app->m_soundSystem->TriggerEntityEvent( this, "Attack" );
    }

    return false;    
}


bool ArmyAnt::AdvanceReturnToBase()
{
    bool arrived = AdvanceToTargetPosition();
    if( arrived ) 
    {
        Building *building = g_app->m_location->GetBuilding( m_buildingId );

        if( !building )
        {
            // Our anthill has been destroyed
            bool newBaseFound = SearchForAntHill();
            if( !newBaseFound )
            {
                m_orders = BaseDestroyed;
            }
            return false;
        }
        else if( building && building->m_type == Building::TypeAntHill )
        {
            AntHill *antHill = (AntHill *) building;
            antHill->m_numAntsInside++;

            // Drop off any spirits we are carrying
            if( g_app->m_location->m_spirits.ValidIndex(m_spiritId) )
            {
                Spirit *spirit = g_app->m_location->m_spirits.GetPointer( m_spiritId );
                if( spirit && spirit->m_state == Spirit::StateAttached )
                {
                    antHill->m_numSpiritsInside++;
                    g_app->m_location->m_spirits.RemoveData( m_spiritId );
                }
            }       
            
            // Any Darwinians being carried are now killed
            Entity *entity = g_app->m_location->GetEntity( m_targetId );
            if( entity && entity->m_type == Entity::TypeDarwinian )
            {
                Darwinian *darwinian = (Darwinian *) entity;
                if( darwinian->m_state == Darwinian::StateCapturedByAnt )            
                {
                    darwinian->ChangeHealth( darwinian->m_stats[Entity::StatHealth] * -2 );
                }
            }
        }

        return true;
    }

    return false;
}


bool ArmyAnt::AdvanceBaseDestroyed()
{
    bool arrived = AdvanceToTargetPosition();
    if( arrived )
    {
        SearchForRandomPosition();
    }

    return false;
}


void ArmyAnt::OrderReturnToBase()
{
    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( building ) 
    {
        m_wayPoint = building->m_pos;
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
        m_orders = ReturnToBase;
    }
    else
    {
        bool newBaseFound = SearchForAntHill();
        if( !newBaseFound )
        {
            m_buildingId = -1;
            m_orders = BaseDestroyed;
        }
    }
}


bool ArmyAnt::SearchForTargets()
{
    bool targetFound = false;
    
    if( !targetFound )      targetFound = SearchForSpirits();
    if( !targetFound )      targetFound = SearchForEnemies();

    return targetFound;
}


bool ArmyAnt::SearchForSpirits()
{   
    Spirit *found = NULL;
    int spiritId = -1;
    double closest = 999999.9;

    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        if( g_app->m_location->m_spirits.ValidIndex(i) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
            double theDist = ( s->m_pos - m_pos ).Mag();

            if( theDist <= ARMYANT_SEARCHRANGE &&
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
        m_orders = CollectSpirit;
        return true;
    }    

    return false;
}


bool ArmyAnt::SearchForEnemies()
{   
    WorldObjectId enemyId = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 0.0, ARMYANT_SEARCHRANGE, m_id.GetTeamId() );
    Entity *enemy = g_app->m_location->GetEntity( enemyId );

    if( enemy && !enemy->m_dead && enemy->m_type != Entity::TypeDarwinian && enemy->m_type != Entity::TypeHarvester )
    {
        m_targetId = enemyId;
        m_orders = AttackEnemy;
        return true;
    }

    return false;
}


bool ArmyAnt::SearchForAntHill()
{
    int buildingId = -1;
    double nearest = 500.0;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];

            if( building->m_type == Building::TypeAntHill &&
                g_app->m_location->IsFriend( building->m_id.GetTeamId(), m_id.GetTeamId() ) )
            {
                double distance = ( building->m_pos - m_pos ).Mag();
                if( distance < nearest )
                {
                    buildingId = building->m_id.GetUniqueId();
                    nearest = distance;
                }
            }
        }
    }


    if( buildingId != -1 )
    {
        m_buildingId = buildingId;
        OrderReturnToBase();
        return true;
    }

    return false;
}


bool ArmyAnt::SearchForRandomPosition()
{
    double distToSpawnPoint = ( m_pos - m_spawnPoint ).Mag();
    double chanceOfReturn = ( distToSpawnPoint / 400.0 );
    if( chanceOfReturn >= 1.0 || syncfrand(1.0) <= chanceOfReturn )
    {
        // We have strayed too far from our spawn point
        // So head back there now
        Vector3 returnVector = m_spawnPoint - m_pos;
        returnVector.SetLength( 100.0 );
        m_wayPoint = m_pos + returnVector;
    }
    else
    {
        double distance = 100.0;
        double angle = syncsfrand(2.0 * M_PI);

        m_wayPoint = m_pos + Vector3( iv_sin(angle) * distance,
                                       0.0,
                                       iv_cos(angle) * distance );
        m_wayPoint = PushFromObstructions( m_wayPoint );

    }
    
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    return true;
}


bool ArmyAnt::AdvanceToTargetPosition()
{
    //
    // Work out where we want to be next

    double speed = m_stats[StatSpeed];
    Vector3 oldPos = m_pos;
    
    if( m_orders == CollectEntity || m_orders == AttackEnemy ) speed *= 2.0;
    
    Vector3 actualDir = (m_wayPoint- m_pos).Normalise();
    Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;   
    //newPos = PushFromObstructions( newPos );
    newPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
    Vector3 moved = newPos - oldPos;
    if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
    newPos = m_pos + moved;
    
    m_pos = newPos;       
    m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
    m_front = ( newPos - oldPos ).Normalise();
    m_front.RotateAroundY( syncsfrand(0.2) );        

    double distance = ( m_pos - m_wayPoint ).Mag();
    if (distance < m_vel.Mag() * SERVER_ADVANCE_PERIOD ||
        distance < 1.0 )
    {
        m_vel.Zero();
        return true;
    }
    
    return false;    
}


void ArmyAnt::GetCarryMarker( Vector3 &_pos, Vector3 &_vel )
{
    Vector3 groundUp = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
    Matrix34 mat( m_front, groundUp, m_pos );
    _pos = m_carryMarker->GetWorldMatrix( mat ).pos;
    _vel = m_vel;
}


void ArmyAnt::Render( double _predictionTime )
{
    if( m_dead ) return;
    
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    Vector3 predictedUp = g_upVector;

    g_app->m_renderer->SetObjectLighting();
    glDisable( GL_TEXTURE_2D );

    Matrix34 mat( m_front, predictedUp, predictedPos );
    mat.u *= m_scale;
    mat.f *= m_scale;
    mat.r *= m_scale;

    m_shape->Render( _predictionTime, mat );

    g_app->m_renderer->UnsetObjectLighting();
}


