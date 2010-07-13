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

#include "worldobject/dropship.h"
#include "worldobject/darwinian.h"

#define DROPSHIP_HOVERHEIGHT   250.0
#define DROPSHIP_ABDUCTIONBEAM_RADIUS 20.0

DropShip::DropShip()
:   Entity(),
    m_numPassengers(20),
    m_state(DropShipStateArriving),
    m_scale(0.2),
    m_speed(1000.0)
{
    m_type = TypeDropShip;
    m_shape = g_app->m_resource->GetShape( "spaceship.shp" );
    m_shape->ScaleRadius(m_scale);
}

bool DropShip::Advance(Unit *_unit)
{
    bool arrived = false;
    if( m_state != DropShipStateDropping )
    {
        arrived = AdvanceToTargetPos();
    }

    m_front.RotateAroundY( 0.05 );

    if( arrived )
    {
        if( m_state == DropShipStateArriving ) m_state = DropShipStateDropping;
        else if( m_state == DropShipStateDropping ) m_state = DropShipStateLeaving;
    }

    if( m_state == DropShipStateDropping )
    {
        if( m_numPassengers > 0 )
        {
            m_lastDrop = g_app->m_location->SpawnEntities( m_pos, m_id.GetTeamId(), -1, Entity::TypeDarwinian, 1, g_zeroVector, 0.0 );
            Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( m_lastDrop, Entity::TypeDarwinian );
            if( d ) d->m_onGround = false;
            m_numPassengers--;
        }
        else
        {
            Entity *e = g_app->m_location->GetEntity( m_lastDrop );
            if( e && e->m_type == Entity::TypeDarwinian && 
                (e->m_onGround || e->m_dead) )
            {
                m_state = DropShipStateLeaving;
                Vector3 pos = m_pos;
                pos.y += 1000.0;
                SetWaypoint( pos );
            }
        }
    }
    else if( m_state == DropShipStateLeaving )
    {
        if( arrived || m_pos.y > 1000.0) return true;
    }

    return false;
}

bool DropShip::ChangeHealth( int _amount, int _damageType )
{
    return false;
}

void DropShip::Render(double _predictionTime)
{
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
 
    Vector3 entityUp = g_upVector;
    Vector3 entityFront = m_front;
    entityFront.RotateAroundY( 0.05 * _predictionTime * 10.0);
    
   // entityFront.Normalise();
    Vector3 entityRight = entityFront ^ entityUp;
    entityUp = entityRight ^ entityFront;

    Matrix34 mat(entityFront, entityUp, predictedPos);
    mat.f *= m_scale;
    mat.r *= m_scale;
    mat.u *= m_scale;

    g_app->m_renderer->SetObjectLighting();
    m_shape->Render(_predictionTime, mat);
    g_app->m_renderer->UnsetObjectLighting();

    if( m_state == DropShipStateDropping )
    {
        RenderBeam( _predictionTime );
    }
}

void DropShip::RenderBeam( double _predictionTime )
{
    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );
    glDepthMask( false );

    double groundHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    double totalHeight = DROPSHIP_HOVERHEIGHT + groundHeight;
    double y = 0; 

    RGBAColour col = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour;

    y = totalHeight - g_gameTime * 50;
    while( y < groundHeight )
    {
        y += DROPSHIP_HOVERHEIGHT;
    }

    for( int i = 0; i < 30; ++i )
    {
        double heightScale = (DROPSHIP_HOVERHEIGHT - (y - groundHeight)) / DROPSHIP_HOVERHEIGHT;
        double radius = DROPSHIP_ABDUCTIONBEAM_RADIUS * heightScale;

        double totalAngle = M_PI * 2;
        int numSteps = totalAngle * 3;

        double angle = -g_gameTime;
        double timeFactor = g_gameTime;
        
        glBegin( GL_QUAD_STRIP );
        for( int i = 0; i <= numSteps; ++i )
        {
            double alpha = 0.2 + (iv_abs(iv_sin(angle * 2 + timeFactor)) * 0.8);
            glColor4ub( col.r, col.g, col.b, alpha * 100 );

            double xDiff = radius * iv_sin(angle);
            double zDiff = radius * iv_cos(angle);
            Vector3 pos = m_pos + (m_vel * _predictionTime) + Vector3(xDiff,5,zDiff);
            pos.y = y ;
            glVertex3dv( pos.GetData() );
            pos.y += 5;
            glVertex3dv( pos.GetData() );
            angle += totalAngle / (double) numSteps;
        }
        glEnd();

        y-= (totalHeight - groundHeight) / 30.0;
        if( y < groundHeight ) 
        {
            y += DROPSHIP_HOVERHEIGHT;
        }
    }

    glEnable( GL_CULL_FACE );
    glShadeModel( GL_FLAT );
    glDepthMask( true );
}

void DropShip::SetWaypoint(const Vector3 _waypoint)
{
    m_waypoint = _waypoint;
}


bool DropShip::AdvanceToTargetPos()
{
    if( m_state == DropShipStateDropping ) return true;
    if( m_waypoint != g_zeroVector )
    {
        Vector3 targetPos = m_waypoint;
        if( m_state != DropShipStateLeaving )
        {
            targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z ) + DROPSHIP_HOVERHEIGHT;
        }

        m_speed += 100.0 * SERVER_ADVANCE_PERIOD;
        m_speed = min( m_stats[StatSpeed] * 10.0, m_speed );

        double distance = (m_pos - targetPos).Mag();

        if( distance < 500.0 )
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

        if( (m_pos - targetPos).Mag() < 10.0 )
        {
            m_waypoint = g_zeroVector;
            m_vel.Zero();
           // m_speed = 0.0;
            return true;
        }
    }

    return false;
}