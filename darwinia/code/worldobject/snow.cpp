#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/debug_render.h"
#include "lib/3d_sprite.h"
#include "lib/resource.h"

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
    m_positionOffset = syncfrand(10.0f);
    m_xaxisRate = syncfrand(2.0f);
    m_yaxisRate = syncfrand(2.0f);
    m_zaxisRate = syncfrand(2.0f);

    m_timeSync = GetHighResTime();
    m_type = EffectSnow;
}
    

bool Snow::Advance()
{
    m_vel *= 0.9f;

    //
    // Make me float around slowly

    m_positionOffset += SERVER_ADVANCE_PERIOD;
    m_xaxisRate += syncsfrand(1.0f);
    m_yaxisRate += syncsfrand(1.0f);
    m_zaxisRate += syncsfrand(1.0f);
    if( m_xaxisRate > 2.0f ) m_xaxisRate = 2.0f;
    if( m_xaxisRate < 0.0f ) m_xaxisRate = 0.0f;
    if( m_yaxisRate > 2.0f ) m_yaxisRate = 2.0f;
    if( m_yaxisRate < 0.0f ) m_yaxisRate = 0.0f;
    if( m_zaxisRate > 2.0f ) m_zaxisRate = 2.0f;
    if( m_zaxisRate < 0.0f ) m_zaxisRate = 0.0f;
    m_hover.x = sinf( m_positionOffset ) * m_xaxisRate;
    m_hover.y = sinf( m_positionOffset ) * m_yaxisRate;
    m_hover.z = sinf( m_positionOffset ) * m_zaxisRate;            

    float heightAboveGround = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    if( heightAboveGround > -10.0f )
    {
        float fractionAboveGround = heightAboveGround / 100.0f;                    
        fractionAboveGround = min( fractionAboveGround, 1.0f );
        fractionAboveGround = max( fractionAboveGround, 0.2f );
        m_hover.y = (-20.0f - syncfrand(20.0f)) * fractionAboveGround;
    }
    else
    {
        return true;
    }
    
    Vector3 oldPos = m_pos;

    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_pos += m_hover * SERVER_ADVANCE_PERIOD;
    float worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    float worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    if( m_pos.x < 0.0f ) m_pos.x = 0.0f;
    if( m_pos.z < 0.0f ) m_pos.z = 0.0f;
    if( m_pos.x >= worldSizeX ) m_pos.x = worldSizeX;
    if( m_pos.z >= worldSizeZ ) m_pos.z = worldSizeZ;

    return false;
}


float Snow::GetLife()
{
    float timePassed = GetHighResTime() - m_timeSync;
    float life = timePassed / 10.0f;
    life = min( life, 1.0f );
    return life;
}


void Snow::Render( float _predictionTime )
{
    _predictionTime -= SERVER_ADVANCE_PERIOD;
    
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    predictedPos += m_hover * _predictionTime;
    
    float size = 20.0f;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0 );
    Render3DSprite( predictedPos, size, size, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
}

