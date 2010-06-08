#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/debug_render.h"
#include "lib/3d_sprite.h"
#include "lib/resource.h"

#undef TRACK_SYNC_RAND
#include "lib/math/random_number.h"

#include "worldobject/snow.h"

#include "main.h"
#include "globals.h"
#include "app.h"
#include "location.h"


// ****************************************************************************
// Class Snow
// ****************************************************************************
  
Snow::Snow()
:   WorldObject()
{    
    m_positionOffset = syncfrand(10.0);
    m_xaxisRate = syncfrand(2.0);
    m_yaxisRate = syncfrand(2.0);
    m_zaxisRate = syncfrand(2.0);

    m_timeSync = GetNetworkTime();
    m_type = EffectSnow;
}
    

bool Snow::Advance()
{
    m_vel *= 0.9;

    //
    // Make me double around slowly

    m_positionOffset += SERVER_ADVANCE_PERIOD;
    m_xaxisRate += syncsfrand(1.0);
    m_yaxisRate += syncsfrand(1.0);
    m_zaxisRate += syncsfrand(1.0);
    if( m_xaxisRate > 2.0 ) m_xaxisRate = 2.0;
    if( m_xaxisRate < 0.0 ) m_xaxisRate = 0.0;
    if( m_yaxisRate > 2.0 ) m_yaxisRate = 2.0;
    if( m_yaxisRate < 0.0 ) m_yaxisRate = 0.0;
    if( m_zaxisRate > 2.0 ) m_zaxisRate = 2.0;
    if( m_zaxisRate < 0.0 ) m_zaxisRate = 0.0;
    m_hover.x = iv_sin( m_positionOffset ) * m_xaxisRate;
    m_hover.y = iv_sin( m_positionOffset ) * m_yaxisRate;
    m_hover.z = iv_sin( m_positionOffset ) * m_zaxisRate;            

    double heightAboveGround = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    if( heightAboveGround > -10.0 )
    {
        double fractionAboveGround = heightAboveGround / 100.0;                    
        fractionAboveGround = min( fractionAboveGround, 1.0 );
        fractionAboveGround = max( fractionAboveGround, 0.2 );
        m_hover.y = (-20.0 - syncfrand(20.0)) * fractionAboveGround;
    }
    else
    {
        return true;
    }
    
    Vector3 oldPos = m_pos;

    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_pos += m_hover * SERVER_ADVANCE_PERIOD;
    double worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    double worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    if( m_pos.x < 0.0 ) m_pos.x = 0.0;
    if( m_pos.z < 0.0 ) m_pos.z = 0.0;
    if( m_pos.x >= worldSizeX ) m_pos.x = worldSizeX;
    if( m_pos.z >= worldSizeZ ) m_pos.z = worldSizeZ;

    return false;
}


double Snow::GetLife()
{
    double timePassed = GetNetworkTime() - m_timeSync;
    double life = timePassed / 10.0;
    life = min( life, 1.0 );
    return life;
}


void Snow::Render( double _predictionTime )
{
    _predictionTime -= SERVER_ADVANCE_PERIOD;
    
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    predictedPos += m_hover * _predictionTime;
    
    double size = 20.0;

    glColor4f( 1.0, 1.0, 1.0, 1.0 );
    Render3DSprite( predictedPos, size, size, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
}

