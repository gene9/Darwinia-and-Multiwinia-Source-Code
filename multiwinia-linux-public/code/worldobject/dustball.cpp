#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/debug_render.h"
#include "lib/3d_sprite.h"
#include "lib/resource.h"
#include "lib/math/random_number.h"

#include "worldobject/dustball.h"

#include "main.h"
#include "globals.h"
#include "app.h"
#include "location.h"

Vector3 DustBall::s_vortexPos;
double DustBall::s_vortexTimer = 0.0;

#define DUSTBALL_SPEED  500.0

// ****************************************************************************
// Class DustBall
// ****************************************************************************
  
DustBall::DustBall()
:   WorldObject(),
    m_size(20.0)
{    
    m_type = EffectDustBall;

    m_vel = g_app->m_location->m_windDirection;
    m_vel.SetLength( DUSTBALL_SPEED );  
}
    

bool DustBall::Advance()
{
    double speed = DUSTBALL_SPEED;

    if( s_vortexPos != g_zeroVector )
    {
        /*double speed = DUSTBALL_SPEED / 5.0;
        speed /= (m_pos.y / 1000.0);
        speed = max( speed, 100.0 );
        speed = min( speed, 500.0 );
        Vector3 thisVortexPos = s_vortexPos;
        thisVortexPos.y = m_pos.y;
        Vector3 oldPos = m_pos;
        double amountToTurn = SERVER_ADVANCE_PERIOD * (1.0 / (m_pos.y / 1000.0));
        Vector3 targetDir = (thisVortexPos - m_pos).Normalise();
        Vector3 actualDir = m_vel.Normalise() * (1.0 - amountToTurn) + targetDir * amountToTurn;
        actualDir.Normalise();

        Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;

        Vector3 moved = newPos - oldPos;
        if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
        newPos = m_pos + moved;*/

        Vector3 vortexPos = s_vortexPos;

        double timeIndex = GetNetworkTime() + m_id.GetUniqueId() * 20;
        double radius = (300.0 * (m_pos.y / 1000.0) );

        Vector3 targetPos = vortexPos;
        targetPos.x += iv_cos( timeIndex ) * radius;
        targetPos.z += iv_sin( timeIndex ) * radius;
        targetPos.y = m_pos.y;

        Vector3 oldPos = m_pos;
        Vector3 actualDir = (targetPos - m_pos).Normalise();

        Vector3 newPos = m_pos + (actualDir * speed * SERVER_ADVANCE_PERIOD);
        Vector3 moved = newPos - oldPos;
        if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
        
        m_pos = newPos;       
        m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
    }
    else
    {
        Vector3 oldPos = m_pos;
        Vector3 target = m_pos + g_app->m_location->m_windDirection * 2000.0;

        double amountToTurn = SERVER_ADVANCE_PERIOD * 3.0;
        Vector3 targetDir = (target - m_pos).Normalise();
        Vector3 actualDir = m_vel.Normalise() * (1.0 - amountToTurn) + targetDir * amountToTurn;
        actualDir.Normalise();

        Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;

        Vector3 moved = newPos - oldPos;
        if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
        newPos = m_pos + moved;

        m_pos = newPos;       
        m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
    }

    if( m_pos.x > g_app->m_location->m_landscape.GetWorldSizeX() ||
        m_pos.z > g_app->m_location->m_landscape.GetWorldSizeZ() ||
        m_pos.x < 0.0 ||
        m_pos.z < 0.0 )
    {
        return true;
    }

    return false;
}


void DustBall::Render( double _predictionTime )
{
    _predictionTime -= SERVER_ADVANCE_PERIOD;
    
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    
    double size = m_size;

    glColor4f(0.6, 0.46, 0.12, 0.2 );
    Render3DSprite( predictedPos, size, size, g_app->m_resource->GetTexture( "textures/cloudyglow.bmp" ) );
    /*Vector3 pos = s_vortexPos;
    pos.y = 0.0;
    for( int i = 0; i < 10; ++i )
    {
        RenderSphere( pos, (300.0 * (pos.y / 1000.0)) );
        pos.y += 100.0;
    }*/
    //RenderSphere( s_vortexPos, 50.0 );
}

