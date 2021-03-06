#include "lib/universal_include.h"
#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/resource.h"

#include "sound/soundsystem.h"

#include "worldobject/egg.h"
#include "worldobject/virii.h"

#include "app.h"
#include "camera.h"
#include "location.h"
#include "main.h"
#include "team.h"


Egg::Egg()
:   Entity(),
    m_state(StateDormant),
    m_spiritId(-1),
    m_timer(0.0)
{    
}


bool Egg::ChangeHealth( int amount, int _damageType )
{
    if( !m_dead )
    {
        if( m_stats[StatHealth] + amount < 0 )
        {
            m_stats[StatHealth] = 100;
            m_dead = true;
        }
        else if( m_stats[StatHealth] + amount > 255 )
        {
            m_stats[StatHealth] = 255;
        }
        else
        {
            m_stats[StatHealth] += amount;    
        }
    }

    return true;
}

void Egg::Render( double predictionTime )
{    
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/egg.bmp" ) );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
 	glEnable		( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glDepthMask     ( false );    
    glDisable       ( GL_CULL_FACE );
    
    RGBAColour colour;
    if( m_id.GetTeamId() >= 0 ) colour = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour;
    double alpha = m_stats[StatHealth] / EntityBlueprint::GetStat( Entity::TypeEgg, StatHealth );
    if( alpha < 0.1 ) alpha = 0.1;
    glColor4ub   ( 255, 255, 255, 255 * alpha );

    Vector3 pos = m_pos + m_vel * predictionTime;
    pos.y += 3.0;
    Vector3 up = g_app->m_camera->GetUp();
    Vector3 right = g_app->m_camera->GetRight();
    double size = 4.0;
 
    //
    // Render main egg shape
    
    if( !m_dead )
    {

        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );       glVertex3dv( (pos - right * size - up * size).GetData() );
            glTexCoord2i( 0, 1 );       glVertex3dv( (pos - right * size + up * size).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3dv( (pos + right * size + up * size).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3dv( (pos + right * size - up * size).GetData() );    
        glEnd();


        //
        // Render throb

        if( m_state == StateFertilised )
        {
            double predictedTimer = m_timer + predictionTime;
            double throb = iv_abs(iv_sin( predictedTimer )) * 8.0 * predictedTimer;
            throb *= alpha;
            if( throb > 255 ) throb = 255;
            if( throb < 0 ) throb = 0;
            size *= 1.3;
            glColor4ub  ( 255, 255, 255, throb );
    
            glBegin( GL_QUADS );
                glTexCoord2i( 0, 0 );       glVertex3dv( (pos - right * size - up * size).GetData() );
                glTexCoord2i( 0, 1 );       glVertex3dv( (pos - right * size + up * size).GetData() );
                glTexCoord2i( 1, 1 );       glVertex3dv( (pos + right * size + up * size).GetData() );
                glTexCoord2i( 1, 0 );       glVertex3dv( (pos + right * size - up * size).GetData() );    
            glEnd();


            throb = iv_abs(iv_sin( predictedTimer )) * 8 * predictedTimer;
            throb *= alpha;
            if( throb > 255 ) throb = 255;
            if( throb < 0 ) throb = 0;
            size *= 0.5;
            pos.y += 2.0;
            glColor4ub  ( 255, 255, 255, throb );
    
            glBegin( GL_QUADS );
                glTexCoord2i( 0, 0 );       glVertex3dv( (pos - right * size - up * size).GetData() );
                glTexCoord2i( 0, 1 );       glVertex3dv( (pos - right * size + up * size).GetData() );
                glTexCoord2i( 1, 1 );       glVertex3dv( (pos + right * size + up * size).GetData() );
                glTexCoord2i( 1, 0 );       glVertex3dv( (pos + right * size - up * size).GetData() );    
            glEnd();
        }
    }
    else
    {
        double predictedHealth = m_stats[StatHealth];
        if( m_onGround ) predictedHealth -= 40.0 * predictionTime;
        else             predictedHealth -= 20.0 * predictionTime;
        double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );

        size *= 0.5;
        
        glColor4ub  ( 255, 255, 255, predictedHealth * 2.0 );

        for( int i = 0; i < 3; ++i )
        {
            Vector3 fragmentPos = pos;
            if( i == 0 ) fragmentPos.x += 10.0 - predictedHealth / 10.0;
            if( i == 1 ) fragmentPos.z += 10.0 - predictedHealth / 10.0;
            if( i == 2 ) fragmentPos.x -= 10.0 - predictedHealth / 10.0;            
            fragmentPos.y += ( fragmentPos.y - landHeight ) * i * 0.5;            
            
            double tleft = 0.0;
            double tright = 1.0;
            double ttop = 0.0;
            double tbottom = 1.0;

            if( i == 0 )
            {
                tright -= (tright-tleft)/2;
                tbottom -= (tbottom-ttop)/2;
            }
            if( i == 1 )
            {
                tleft += (tright-tleft)/2;
                tbottom -= (tbottom-ttop)/2;
            }
            if( i == 2 )
            {
                ttop += (tbottom-ttop)/2;
                tleft += (tright-tleft)/2;
            }

            glBegin(GL_QUADS);
                glTexCoord2f(tleft, tbottom);     glVertex3dv( (fragmentPos - right * size + up * size).GetData() );
                glTexCoord2f(tright, tbottom);    glVertex3dv( (fragmentPos + right * size + up * size).GetData() );
                glTexCoord2f(tright, ttop);       glVertex3dv( (fragmentPos + right * size - up * size).GetData() );
                glTexCoord2f(tleft, ttop);        glVertex3dv( (fragmentPos - right * size - up * size).GetData() );
            glEnd();
        }
    }


    BeginRenderShadow();
    RenderShadow( pos, size );
    EndRenderShadow();

    
    glEnable        ( GL_CULL_FACE );
	glDisable       ( GL_BLEND );
    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}

bool Egg::Advance( Unit *_unit )
{
    if( g_app->m_location->m_spirits.ValidIndex( m_spiritId ) )
    {
        Spirit *spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
        spirit->m_pos = m_pos+Vector3(0,3,0);
    }
    else
    {
        m_state = StateDormant;
        m_timer = 0.0;
    }

    if( !m_dead )
    {
        if( m_state == StateFertilised )
        {
            m_timer += SERVER_ADVANCE_PERIOD;

            if( m_timer >= 15.0 )
            {
                g_app->m_location->m_spirits.RemoveData( m_spiritId );
                if( Virii::s_viriiPopulation[m_id.GetTeamId()] < VIRII_POPULATIONLIMIT )
                {
                    g_app->m_location->SpawnEntities( m_pos, m_id.GetTeamId(), -1, Entity::TypeVirii, 4, g_zeroVector, 0.0, 200.0 );
                }
                return true;
            }
        }
        else if( m_state == StateDormant )
        {
            m_timer += SERVER_ADVANCE_PERIOD;            
            double maxLife = EntityBlueprint::GetStat( TypeEgg, StatHealth ); 
            maxLife *= (1.0 - ( m_timer / EGG_DORMANTLIFE ));            
            if( m_stats[StatHealth] > maxLife )
            {
                int change = m_stats[StatHealth] - maxLife;
                ChangeHealth( change * -1 );
            }
        }
    }

    if( m_onGround )
    {
        double groundLevel = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
        if( m_pos.y < groundLevel )
        {
            m_pos.y = groundLevel;
        }
        else if( m_pos.y > groundLevel )
        {
            m_onGround = false;
        }
    }

    if( m_dead )
    {
        if( g_app->m_location->m_spirits.ValidIndex( m_spiritId ) )
        {
            Spirit *spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
            spirit->EggDestroyed();
            m_spiritId = -1;
        }
    }

    if( !m_onGround )
    {
        AdvanceInAir(_unit);
    }
    else
    {        
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    }
    
    if( m_pos.y <= 0.0 )
    {
        ChangeHealth(-500);
    }

    return Entity::Advance(_unit);
}

void Egg::Fertilise( int spiritId )
{
    if( g_app->m_location->m_spirits.ValidIndex( spiritId ) )
    {
        m_spiritId = spiritId;
        Spirit *spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
        spirit->InEgg();
        m_state = StateFertilised;
        m_timer = 0.0;
        m_stats[StatHealth] = EntityBlueprint::GetStat( TypeEgg, StatHealth );
    }
}
