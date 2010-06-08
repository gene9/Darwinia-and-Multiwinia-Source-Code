#include "lib/universal_include.h"

#include <math.h>

#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/matrix34.h"
#include "lib/shape.h"
#include "lib/math/random_number.h"

#include "app.h"
#include "explosion.h"
#include "globals.h"
#include "global_world.h"
#include "location.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"
#include "camera.h"
#include "main.h"
#include "entity_grid.h"

#include "sound/soundsystem.h"

#include "worldobject/spaceship.h"
#include "worldobject/darwinian.h"

#define SPACESHIP_HOVERHEIGHT   500.0f
#define SPACESHIP_ABDUCTIONBEAM_RADIUS 45.0f


SpaceShip::SpaceShip()
:   Entity(),
    m_abductees(0),
    m_state(ShipStateIdle),
    m_speed(0.0f),
    m_abductionTimer(0.0f)
{
    m_type = TypeSpaceShip;
    m_shape = g_app->m_resource->GetShape( "spaceship.shp" );
}

void SpaceShip::Begin()
{
    Entity::Begin();
    //m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z ) + SPACESHIP_HOVERHEIGHT;
    m_onGround = false;

    m_abductionTimer = 60.0f + syncfrand(60.0f);
}

void SpaceShip::AdvanceAbducting( bool _atTarget)
{
    bool targetAbducted = false;
    int numFound = 0;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, SPACESHIP_ABDUCTIONBEAM_RADIUS, &numFound );
    for( int i = 0; i < numFound; ++i )
    {
        Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeDarwinian );
        if( d && 
            d->m_id.GetTeamId() != g_app->m_location->GetFuturewinianTeamId() &&
            d->m_state != Darwinian::StateInsideArmour )
        {
            Vector3 myPos = m_pos;
            myPos.y = d->m_pos.y;

            float distance = (myPos - d->m_pos).Mag();

            if( distance <= SPACESHIP_ABDUCTIONBEAM_RADIUS )
            {
                d->Abduct( m_id );
                if( d->m_id == m_target ) 
                {
                    m_target.SetInvalid();
                    m_waypoint = g_zeroVector;
                }
            }
        }
    }


    m_abductionTimer -= SERVER_ADVANCE_PERIOD;

    if( m_abductionTimer <= 0.0f && _atTarget )
    {
        m_state = ShipStateWaiting;
        m_target.SetInvalid();
        m_waypoint = g_zeroVector;
        m_vel.Zero();
        m_abductionTimer = 5.0f;
    }

}

void SpaceShip::AdvanceDropping()
{
    float groundHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
    if( groundHeight < 20 ) 
    {
        // We're over water, so don't drop anyone
        m_abductees = 0;
    }

    if( m_abductees > 100 ) m_abductees = 100;

    if( m_abductees > 0 )
    {
        m_lastDrop = g_app->m_location->SpawnEntities( m_pos, g_app->m_location->GetFuturewinianTeamId(), -1, Entity::TypeDarwinian, 1, g_zeroVector, 0.0f );
        Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( m_lastDrop, Entity::TypeDarwinian );
        if( d ) d->m_onGround = false;
        m_abductees --;
    }

    if( m_abductees <= 0 )
    {
        Entity *e = g_app->m_location->GetEntity( m_lastDrop );
        if( !e || e->m_onGround )
        {
            m_state = ShipStateLeaving;
            g_app->m_soundSystem->StopAllSounds( m_id, "SpaceShip TractorBeam" );
            int worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
            int worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
            Vector3 pos( FRAND(worldSizeX), 2000.0f, FRAND(worldSizeZ) );
            SetWaypoint( pos );
        }
    }
}

void SpaceShip::AdvanceEngines()
{
    Matrix34 mat( m_front, g_upVector, m_pos );
    for( int e = 1; e < 5; ++e )
    {
        char markerName[128];
        sprintf( markerName, "MarkerEngine%d", e );
        ShapeMarker *engine = m_shape->m_rootFragment->LookupMarker( markerName );
        AppReleaseAssert( engine, "SpaceShip: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", markerName, m_shape->m_name );

        float size = 80.0f + frand(80);
        Vector3 pos = engine->GetWorldMatrix( mat ).pos;
        Vector3 vel( sfrand(50), -frand(50), sfrand(50) );
        g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeBlueSpark, size, RGBAColour(150,150,255,255) );            

        for( int i = 0; i < 15; ++i )
        {
            Vector3 pos = engine->GetWorldMatrix( mat ).pos;
            pos += Vector3( sfrand(20), 10, sfrand(20) );
            
            Vector3 vel( sfrand(50), -frand(150), sfrand(50) );
            float size = 500.0f;

            if( m_state == ShipStateLeaving ) 
            {
                vel *= 2.5f;
            }
            if( i > 10 )
            {
                vel.x *= 0.75f;
                vel.z *= 0.75f;
                //g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileTrail, size );
            }
            else
            {
                g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileFire, size, RGBAColour( 50, 50, 250 ) );
            }
        }
    }
}

bool SpaceShip::Advance(Unit *_unit)
{
    bool atTarget = AdvanceToTargetPos();
    
    if( m_state != ShipStateLeaving )
    {
        if( m_state != ShipStateDropping && 
            m_state != ShipStateWaiting )
        {
            if( atTarget || m_waypoint == g_zeroVector )
            {
                FindNewTarget();
            }
        }

        if( m_state == ShipStateIdle && atTarget )
        {
            m_state = ShipStateAbducting;
            g_app->m_soundSystem->TriggerEntityEvent( this, "TractorBeam" );
        }
    }
    else
    {
        if( m_pos.y >= 1900 ) return true;
    }

    switch( m_state )
    {
        case ShipStateAbducting:    AdvanceAbducting(atTarget);     break;
        case ShipStateDropping:     AdvanceDropping();              break;
        
        case ShipStateWaiting:
            m_abductionTimer -= SERVER_ADVANCE_PERIOD;
            if( m_abductionTimer <= 0.0f )
            {
                m_state = ShipStateDropping;
            }
            break;
    }

    AdvanceEngines();

    m_front.RotateAroundY( 0.01 );

    return Entity::Advance(_unit);
}

bool SpaceShip::AdvanceToTargetPos()
{
    if( m_state == ShipStateDropping ) return true;

    if( m_waypoint != g_zeroVector )
    {
        Vector3 targetPos = m_waypoint;
        if( m_state != ShipStateLeaving )
        {
            targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z ) + SPACESHIP_HOVERHEIGHT;
        }

        m_speed += 100.0f * SERVER_ADVANCE_PERIOD;
        m_speed = min( m_stats[StatSpeed] * 2.0, m_speed );

        if( m_pos.y > 1000.0f && m_state == ShipStateIdle )
        {
            m_speed = ((m_pos.y - 1000.0) / 5000.0) * 8000.0;
            m_speed = max( m_speed, m_stats[StatSpeed] * 2.0 );
        }

        double distance = (m_pos - targetPos).Mag();

        if( distance < 200.0 )
        {
            m_speed -= 100.0 * SERVER_ADVANCE_PERIOD;
            m_speed = max( m_stats[StatSpeed] * 0.1, m_speed );
        }

        double speed = m_speed;

        Vector3 currentDirection = m_vel;
        currentDirection.Normalise();
        Vector3 targetDirection = (targetPos - m_pos).Normalise();

        Vector3 actualDirection = (currentDirection * 0.9) + (targetDirection * 0.1);

        m_vel = actualDirection;
        m_vel.SetLength( speed * SERVER_ADVANCE_PERIOD );
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;

        if( (m_pos - targetPos).Mag() < 100.0f )
        {
            m_waypoint = g_zeroVector;
            m_vel.Zero();
           // m_speed = 0.0f;
            return true;
        }
    }

    return false;
}

void SpaceShip::FindNewTarget()
{
    //double minRange = 0.0;
    //if( m_abductees > 0 ) minRange = 300.0;

    /*int numFound;
    g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, m_pos.x, m_pos.z, 3000.0, &numFound, m_id.GetTeamId() );

    Vector3 myPos = m_pos;
    myPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( myPos.x, myPos.z );
    double closest = FLT_MAX;
    for( int i = 0; i < numFound; ++i )
    {
        Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeDarwinian );
        if( d && d->m_state != Darwinian::StateBeingAbducted &&
            d->m_state != Darwinian::StateInsideArmour &&            
            d->m_state != Darwinian::StateOperatingPort )
        {
            double distance = ( myPos - d->m_pos ).Mag();
            if( distance > minRange &&
                distance < closest )
            {
                closest = distance;
                m_target = d->m_id;
            }

            
        }
    }*/

    //
    // Instead of doing this, pick a random place to move towards
    

    //Entity *e = g_app->m_location->GetEntity( m_target );
    //if( e )
    //{
    //    SetWaypoint( e->m_pos );
    //}
    //else
    {
        Vector3 pos;
        int worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
        int worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
        while( true )
        {
            pos.Set( SFRAND(worldSizeX), 0.0, SFRAND(worldSizeZ) );
            pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );

            if( pos.y >= 30 ) break;
        }
        SetWaypoint(pos);
    }
}

void SpaceShip::Render(double predictionTime)
{
    Vector3 predictedPos = m_pos + m_vel * predictionTime;
 
    Vector3 entityUp = g_upVector;
    Vector3 entityFront = m_front;
    entityFront.RotateAroundY( 0.01 * predictionTime * 10.0);
    
   // entityFront.Normalise();
    Vector3 entityRight = entityFront ^ entityUp;
    entityUp = entityRight ^ entityFront;


    Matrix34 mat(entityFront, entityUp, predictedPos);
    g_app->m_location->SetupSpecialLights();
    m_shape->Render(predictionTime, mat);
   
    g_app->m_renderer->UnsetObjectLighting();

    if( m_state == ShipStateAbducting ||
        m_state == ShipStateDropping )
    {
        RenderBeam( predictionTime );
    }
}

void SpaceShip::RenderBeam( double _predictionTime )
{
    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );
    glDepthMask( false );

    float groundHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    float totalHeight = SPACESHIP_HOVERHEIGHT + groundHeight;
    float y = 0; 

    RGBAColour col = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour;

    if( m_state == ShipStateAbducting )
    {
        y = groundHeight + g_gameTime * 50;
        while( y > totalHeight )
        {
            y -= SPACESHIP_HOVERHEIGHT;
        }
    }
    else
    {
        y = totalHeight - g_gameTime * 50;
        while( y < groundHeight )
        {
            y += SPACESHIP_HOVERHEIGHT;
        }
    }

    for( int i = 0; i < 30; ++i )
    {
        float heightScale = (SPACESHIP_HOVERHEIGHT - (y - groundHeight)) / SPACESHIP_HOVERHEIGHT;
        float radius = SPACESHIP_ABDUCTIONBEAM_RADIUS * heightScale;

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
            Vector3 pos = m_pos + (m_vel * _predictionTime) + Vector3(xDiff,5,zDiff);
            pos.y = y ;
            glVertex3dv( pos.GetData() );
            pos.y += 5;
            glVertex3dv( pos.GetData() );
            angle += totalAngle / (double) numSteps;
        }
        glEnd();

        if( m_state == ShipStateAbducting )
        {
            y+= (totalHeight - groundHeight) / 30.0f;
            if( y > totalHeight ) 
            {
                y -= SPACESHIP_HOVERHEIGHT;
            }
        }
        else
        {
            y-= (totalHeight - groundHeight) / 30.0f;
            if( y < groundHeight ) 
            {
                y += SPACESHIP_HOVERHEIGHT;
            }
        }
    }

    glEnable( GL_CULL_FACE );
    glShadeModel( GL_FLAT );
    glDepthMask( true );
}

void SpaceShip::SetWaypoint(const Vector3 &_wayPoint)
{
    m_waypoint = _wayPoint;    
}

bool SpaceShip::ChangeHealth( int amount, int _damageType)
{
    return false;
}

void SpaceShip::IncreaseAbductees()
{
    m_abductees++;
}


void SpaceShip::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "TractorBeam" );
}
