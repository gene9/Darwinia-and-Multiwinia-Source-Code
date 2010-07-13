#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/debug_render.h"
#include "lib/input/input.h"
#include "lib/input/control_bindings.h"
#include "lib/text_renderer.h"

#include "sound/soundsystem.h"

#include "worldobject/carryablebuilding.h"
#include "worldobject/darwinian.h"

#include "app.h"
#include "location.h"
#include "entity_grid.h"
#include "main.h"
#include "team.h"
#include "user_input.h"
#include "multiwinia.h"


CarryableBuilding::CarryableBuilding()
:   Building(),
    m_lifted(false),
    m_scale(1.0),
    m_numLifters(0),
    m_numLiftersThisFrame(0),
    m_minLifters(0),
    m_maxLifters(0),
    m_recountTimer(0.0),
    m_numRequests(0),
    m_speedScale(1.0),
    m_routeId(-1),
    m_routeWayPointId(-1),
    m_requestedRouteId(-1)
{
}


void CarryableBuilding::Initialise( Building *_template )
{
    Building::Initialise( _template );
}


void CarryableBuilding::SetShape( Shape *_shape )
{
    Building::SetShape( _shape );
}


void CarryableBuilding::SetWaypoint( Vector3 const &_waypoint, int routeId )
{    
    m_requestedWaypoint = _waypoint;
    m_requestedRouteId = routeId;
    ++m_numRequests;
}


Vector3 CarryableBuilding::GetCarryPosition( int _uniqueId )
{    
    if( m_numLiftersThisFrame >= m_maxLifters )
    {
        return g_zeroVector;
    }

    Vector3 offset( m_carryRadius, 0, 0 );

    offset.RotateAroundY( _uniqueId * 123.467 );
    
    offset += m_pos;
    offset += m_vel * 0.5;
    
    offset.y = g_app->m_location->m_landscape.m_heightMap->GetValue(offset.x, offset.z);

    return offset;
}


int *CarryableBuilding::CalculateCollisions( Vector3 const &_pos, int &_numCollisions, double &_collisionFactor )
{
    double size = 30;
    _collisionFactor = 0;
    _numCollisions = 0;

    static int collisions[32];

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            CarryableBuilding *carryable = (CarryableBuilding *) g_app->m_location->m_buildings[i];
            if( carryable != this && IsCarryableBuilding(carryable->m_type) )
            {
                double distance = ( carryable->m_pos - _pos ).Mag();
                double totalRadius = m_carryRadius + carryable->m_carryRadius;//size * m_scale + size * carryable->m_scale;                
                if( distance < totalRadius )
                {
                    collisions[_numCollisions] = carryable->m_id.GetUniqueId();
                    ++_numCollisions;
                    _collisionFactor += ( totalRadius - distance );
                }
            }
        }
    }
    
    return collisions;
}


double CarryableBuilding::GetCarryPercentage()
{
    double pushForce = 0.0;

    if( m_numLifters >= m_minLifters )
    {
        pushForce = (m_numLifters) / double(m_maxLifters);                
    }

    return pushForce;
}


void CarryableBuilding::FollowRoute()
{
    if(m_routeId == -1 )
    {
        return;
    }

    Route *route = g_app->m_location->m_routingSystem.GetRoute( m_routeId );
    if(!route)
    {
        m_routeId = -1;
        return;
    }

    bool waypointChange = false;

    if (m_routeWayPointId == -1)
    {
        m_routeWayPointId = 1;
        waypointChange = true;
    }
    else
    {
        double distToWaypoint = ( m_pos - m_waypoint ).Mag();
        if( distToWaypoint < 10 )
        {
            ++m_routeWayPointId;    
            waypointChange = true;
        }
    }


    if( waypointChange )
    {
        if (m_routeWayPointId >= route->m_wayPoints.Size())
        {
            m_routeWayPointId = -1;
            m_routeId = -1;
            return;
        }

        WayPoint *waypoint = route->m_wayPoints.GetData(m_routeWayPointId);
        if( !waypoint )
        {
            m_routeId = -1;
            return;
        }

        m_waypoint = waypoint->GetPos();
        m_waypoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_waypoint.x, m_waypoint.z );            
    }   
}


bool CarryableBuilding::Advance()
{
#if defined(INCLUDEGAMEMODE_CTS) || defined(INCLUDEGAMEMODE_ASSAULT) || defined(INCLUDEDGAMEMODE_PROLOGUE)
    CalculateOwnership();

    if( m_id.GetTeamId() == 255 )
    {
        m_waypoint = m_pos;
        m_routeId = -1;
        // AppDebugOut( "Statue waypoint reset because team = 255\n" );
    }

    //
    // Update our waypoint if enough darwinians asked for it

    if( m_numRequests > m_numLiftersThisFrame * 0.66 )
    {
        m_waypoint = m_requestedWaypoint;
        m_routeId = m_requestedRouteId;
        m_routeWayPointId = -1;
        AppDebugOut( "Statue waypoint set based on carriers\n" );
    }
    m_numRequests = 0;


    //
    // Follow our route if we have one

    if( m_routeId != -1 ) 
    {
        FollowRoute();
    }

   
    //
    // Calculate collisions where we are

    int numCollisions;
    double currentCollisionFactor;
    CalculateCollisions( m_pos, numCollisions, currentCollisionFactor );

   
    //
    // How hard are we being pushed?
    
    m_numLifters = m_numLiftersThisFrame;
    m_numLiftersThisFrame = 0;
    bool oldLifted = m_lifted;
    m_lifted = false;

    double pushForce = GetCarryPercentage();
    if( pushForce > 0.0 ) 
    {
        m_lifted = true;
        pushForce *= 16.0 * GetSpeedScale();
        pushForce *= iv_abs( iv_sin(GetNetworkTime()) );
    }    
    

    if( !oldLifted && m_lifted )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "Lift" );
        g_app->m_soundSystem->TriggerBuildingEvent( this, "Carry" );

        // Just picked up, so recalculate our route immediately
        m_waypoint = m_pos;
        m_routeId = -1;
    }

    if( oldLifted && !m_lifted )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "Drop" );
        g_app->m_soundSystem->StopAllSounds( m_id, "Statue Carry" );
    }


    //
    // Work out where we want to be

    Vector3 desiredMovementVector = (m_waypoint - m_pos).Normalise();
    Vector3 desiredVel = desiredMovementVector * pushForce;    
    Vector3 desiredPos = m_pos + desiredVel * SERVER_ADVANCE_PERIOD;
    desiredPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(desiredPos.x, desiredPos.z);   


    //
    // If we are there, don't bother moving any further

    double currentDistance = ( m_pos - m_waypoint ).Mag();
    double desiredDistance = ( desiredPos - m_waypoint ).Mag();
    
    if( desiredDistance >= currentDistance )
    {
        desiredVel.Zero();
        desiredPos = m_pos;
    }


    //
    // Calculate collisions where we want to be

    double desiredCollisionFactor;
    int *collisionIds = CalculateCollisions( desiredPos, numCollisions, desiredCollisionFactor );


    if( currentCollisionFactor == 0.0 || desiredCollisionFactor < currentCollisionFactor )
    {
        m_vel = desiredVel;
        m_pos = desiredPos;
        m_centrePos = m_pos;

        m_up = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
        m_up.y *= 1.5;
        m_up.Normalise();

        Vector3 right = m_front ^ g_upVector;
        right.Normalise();
        m_front = m_up ^ right;
    }
    else
    {
        // We have crashed into another carryable object
        // Damage it if it is an enemy
        m_vel.Zero();
        int numEnemies = 0;
        for( int i = 0; i < numCollisions; ++i )
        {
            CarryableBuilding *carryable = (CarryableBuilding *) g_app->m_location->GetBuilding( collisionIds[i] );
            AppDebugAssert( carryable && IsCarryableBuilding(carryable->m_type) );
            if( !g_app->m_location->IsFriend( carryable->m_id.GetTeamId(), m_id.GetTeamId() ) )
            {
                ++numEnemies;
            }
        }

        double collisionForce = m_scale / (double) numEnemies;
        for( int i = 0; i < numCollisions; ++i )
        {
            CarryableBuilding *carryable = (CarryableBuilding *) g_app->m_location->GetBuilding( collisionIds[i] );
            AppDebugAssert( carryable && IsCarryableBuilding(carryable->m_type) );
            if( !g_app->m_location->IsFriend( carryable->m_id.GetTeamId(), m_id.GetTeamId() ) )
            {
                carryable->HandleCollision( collisionForce );
            }
        }
    }
#endif

    return Building::Advance();
}


void CarryableBuilding::HandleCollision( double _force )
{

}

double CarryableBuilding::GetSpeedScale()
{
    double scale = m_speedScale;
    if( m_id.GetTeamId() == 255) return scale;

    int numFound;
    bool include[NUM_TEAMS];
    memset( include, false, sizeof(include) );
    include[m_id.GetTeamId()] = true;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, m_carryRadius * 1.2, &numFound, include);

    int numHotFeet = 0;
    int numValid = 0;
    double totalSpeedRatio = 0.0;
    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Darwinian *entity = (Darwinian *)g_app->m_location->GetEntitySafe( id, Entity::TypeDarwinian );
        if( entity && entity->m_state == Darwinian::StateCarryingBuilding &&
            entity->m_buildingId == m_id.GetUniqueId() )
        {
            if(entity->m_hotFeetTaskId != -1 )
            {
                numHotFeet++;
            }
            totalSpeedRatio += (double)entity->m_stats[Entity::StatSpeed] / EntityBlueprint::GetStat( Entity::TypeDarwinian, Entity::StatSpeed );
            numValid++;
        }
    }

    if( numHotFeet > 0 )
    {
        totalSpeedRatio /= numValid;
        //double hotfeetRatio = 1.0 + ((double)numHotFeet / (double)m_numLifters);        
        scale *= totalSpeedRatio;
    }

    return scale;
}


void CarryableBuilding::CalculateOwnership()
{
    if( GetNetworkTime() > m_recountTimer )
    {
        m_recountTimer = GetNetworkTime() + 1.0f;

        int oldTeamId = m_id.GetTeamId();
        int highestCount = 0;
        int newTeamId = 255;

        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            if( g_app->m_location->m_teams[t]->m_teamType != TeamTypeUnused )
            {
                int numFound;
                bool includes[NUM_TEAMS];
                memset( includes, false, sizeof(bool) * NUM_TEAMS );
                includes[t] = true;
                g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, 50, &numFound, includes );
                int entitiesCounted = 0;

                for( int i = 0; i < numFound; ++i )
                {
                    WorldObjectId id = s_neighbours[i];
                    Entity *entity = g_app->m_location->GetEntity( id );
                    if( entity && 
                        entity->m_type == Entity::TypeDarwinian && 
                        !entity->m_dead )
                    {
                        ++entitiesCounted;
                    }
                }
                
                if( entitiesCounted > highestCount &&
                    entitiesCounted >= m_minLifters )
                {
                    newTeamId = t;
                    highestCount = entitiesCounted;
                }
            }
        }

        if( !g_app->m_location->IsFriend( newTeamId, oldTeamId ) )
        {
            SetTeamId( newTeamId );
        }
        m_recountTimer = GetNetworkTime() + 1.0;

        if( m_id.GetTeamId() != oldTeamId )
        {
            AppDebugOut( "Statue team changed to %d\n", m_id.GetTeamId() );
            m_waypoint = m_pos;
            m_routeId = -1;
            m_routeWayPointId = -1;
        }
    }
}


bool CarryableBuilding::IsInView()
{
    return true;
}


void CarryableBuilding::Render( double _predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    if( m_lifted ) predictedPos.y += 5.0;

    Vector3 predictedUp = g_app->m_location->m_landscape.m_normalMap->GetValue(predictedPos.x, predictedPos.z);
    predictedUp.y *= 1.5;
    predictedUp.Normalise();

    Vector3 predictedRight = m_front ^ g_upVector;
    predictedRight.Normalise();
    Vector3 predictedFront = predictedUp ^ predictedRight;

    if( m_shape )
    {
	    Matrix34 mat(predictedFront, predictedUp, predictedPos);
        mat.f *= m_scale;
        mat.u *= m_scale;
        mat.r *= m_scale;

        glEnable( GL_NORMALIZE );
        glDisable( GL_CULL_FACE );

        m_shape->Render(_predictionTime, mat);

        glEnable( GL_CULL_FACE );
        glDisable( GL_NORMALIZE );
    }

    //RenderSphere( predictedPos, m_carryRadius );

    //RenderArrow( predictedPos, m_waypoint, 1.0 );


    //RenderSphere( m_pos, m_radius );
    
    //g_gameFont.DrawText3DCentre( predictedPos + g_upVector * 100, 10, "%2.2", m_up.y );

    //if( !g_app->m_editing )
    //{        
    //    g_gameFont.DrawText3DCentre( predictedPos + g_upVector * 100, 10, "%d / min%d / max%d", m_numLifters, m_minLifters, m_maxLifters );
    //    g_gameFont.DrawText3DCentre( predictedPos + g_upVector * 110, 10, "Team %d", m_id.GetTeamId() );
    //}
}


void CarryableBuilding::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "Lift" );
    _list->PutData( "Carry" );
    _list->PutData( "Drop" );
}


bool CarryableBuilding::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    return false;
}


bool CarryableBuilding::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}


bool CarryableBuilding::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                          double _rayLen, Vector3 *_pos, Vector3 *_norm)
{
    if( g_app->m_editing )
    {
        return Building::DoesRayHit( _rayStart, _rayDir, _rayLen, _pos, _norm );
    }
    else
    {
        return false;
    }
}


bool CarryableBuilding::IsCarryableBuilding( int _type )
{
    return( _type == TypeChessPiece ||
            _type == TypeStatue ||
            _type == TypeWallBuster );
}

