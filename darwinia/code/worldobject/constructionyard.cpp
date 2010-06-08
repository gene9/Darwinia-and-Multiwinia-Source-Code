#include "lib/universal_include.h"
#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/3d_sprite.h"
#include "lib/preferences.h"

#include "worldobject/constructionyard.h"
#include "worldobject/researchitem.h"
#include "worldobject/armour.h"

#include "app.h"
#include "main.h"
#include "global_world.h"
#include "particle_system.h"
#include "location.h"
#include "camera.h"
#include "renderer.h"
#include "explosion.h"
#include "team.h"


ConstructionYard::ConstructionYard()
:   Building(),
    m_numPrimitives(0),
    m_numSurges(0),
    m_timer(-1.0f),
    m_fractionPopulated(0.0f),
    m_numTanksProduced(0),
    m_alpha(0.0f)
{
    m_type = TypeYard;
    SetShape( g_app->m_resource->GetShape( "constructionyard.shp" ) );

    m_rung = g_app->m_resource->GetShape( "constructionyardrung.shp" );
    m_primitive = g_app->m_resource->GetShape( "mineprimitive1.shp" );

    for( int i = 0; i < YARD_NUMPRIMITIVES; ++i )
    {
        char name[64];
        sprintf( name, "MarkerPrimitive0%d", i+1 );
        m_primitives[i] = m_shape->m_rootFragment->LookupMarker( name );
        DarwiniaDebugAssert( m_primitives[i] );
    }

    for( int i = 0; i < YARD_NUMRUNGSPIKES; ++i )
    {
        char name[64];
        sprintf( name, "MarkerSpike0%d", i+1 );
        m_rungSpikes[i] = m_rung->m_rootFragment->LookupMarker( name );
        DarwiniaDebugAssert( m_rungSpikes[i] );
    }
}


bool ConstructionYard::Advance()
{
    m_fractionPopulated = (float) GetNumPortsOccupied() / (float) GetNumPorts();

    if( m_fractionPopulated > 0.0f &&
        m_numSurges > 0 &&
        m_numPrimitives > 0 )
    {
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );    
        if( gb && !gb->m_online )
        {
            gb->m_online = true;
            g_app->m_globalWorld->EvaluateEvents();
        }                                    
    }

    
    if( m_timer < 0.0f )
    {
        // Not currently building anything
        if( m_numPrimitives == YARD_NUMPRIMITIVES &&
            m_numSurges > 50 )
        {
            m_timer = 30.0f;
        }
    }
    else
    {
        // Building a tank
        m_timer -= m_fractionPopulated * SERVER_ADVANCE_PERIOD;
        if( m_timer <= 0.0f )
        {
            if( IsPopulationLocked() )
            {
                m_timer = 30.0f;
            }
            else
            {
                m_numPrimitives = 0;
                m_numSurges = 1;

                Matrix34 mat( m_front, g_upVector, m_pos );
                Matrix34 prim = m_primitives[5]->GetWorldMatrix( mat );
                WorldObjectId objId = g_app->m_location->SpawnEntities( prim.pos, 2, -1, Entity::TypeArmour, 1, g_zeroVector, 0.0f );
                Entity *entity = g_app->m_location->GetEntity( objId );
                Armour *armour = (Armour *) entity;     
                armour->m_front.Set( 0, 0, 1 );
                armour->m_vel.Zero();
                armour->SetWayPoint( m_pos + Vector3(0,0,500) );
            
                ++m_numTanksProduced;
                m_timer = -1.0f;
            }
        }
    }
    
    return Building::Advance();    
}


Matrix34 ConstructionYard::GetRungMatrix1()
{
    if( m_numSurges > 0 )
    {
        float rungHeight = 55.0f + sinf(g_gameTime) * 10.0f * m_fractionPopulated;
        Vector3 rungPos = m_pos + Vector3(0,rungHeight,0);
        Vector3 front = m_front;
        front.RotateAroundY( cosf(g_gameTime * 0.5f) * 0.5f * m_fractionPopulated );

        Matrix34 mat(front, g_upVector, rungPos);
	    return mat;
    }
    else
    {
	    Matrix34 mat(m_front, g_upVector, m_pos+Vector3(0,45,0));
        return mat;
    }
}


Matrix34 ConstructionYard::GetRungMatrix2()
{
    if( m_numSurges > 0 )
    {
        float rungHeight = 110.0f + sinf(g_gameTime * 0.8) * 15.0f * m_fractionPopulated;
        Vector3 rungPos = m_pos + Vector3(0,rungHeight,0);
        Vector3 front = m_front;
        front.RotateAroundY( cosf(g_gameTime * 0.4f) * 0.6f * m_fractionPopulated );
	    Matrix34 mat = Matrix34(front, g_upVector, rungPos);
        return mat;
    }
    else
    {
        Matrix34 mat = Matrix34(m_front, g_upVector, m_pos+Vector3(0,75,0));
        return mat;
    }
}



void ConstructionYard::Render( float _predictionTime )
{
    Building::Render( _predictionTime );
        
    // Rung 1
    Matrix34 mat = GetRungMatrix1();
    m_rung->Render(_predictionTime, mat);


    // Rung 2
    mat = GetRungMatrix2();
	m_rung->Render(_predictionTime, mat);        


    //
    // Primitives
    
    for( int i = 0; i < m_numPrimitives; ++i )
    {
        Matrix34 mat( m_front, g_upVector, m_pos );
        Matrix34 prim = m_primitives[i]->GetWorldMatrix( mat );        
        prim.pos.y += sinf(g_gameTime+i) * 5.0f;

        m_primitive->Render( _predictionTime, prim );
    }
}


void ConstructionYard::RenderAlphas( float _predictionTime )
{
    Building::RenderAlphas( _predictionTime );   

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();
        
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/cloudyglow.bmp" ) );
    
    float timeIndex = g_gameTime * 2;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int maxBlobs = 50;
    if( buildingDetail == 2 ) maxBlobs = 25;
    if( buildingDetail == 3 ) maxBlobs = 10;
    

    //
    // Calculate alpha value

    float targetAlpha = 0.0f;
    if( m_timer > 0.0f )
    {
        targetAlpha = 1.0f - ( m_timer / 60.0f );
    }
    else if( m_numPrimitives > 0 )
    {
        targetAlpha = (m_numPrimitives / 9.0f) * 0.5f;
    }
    if( m_numSurges > 0 )
    {
        targetAlpha = max( targetAlpha, 0.2f );
    }
    
    float factor1 = g_advanceTime;
    float factor2 = 1.0f - factor1;
    m_alpha = m_alpha * factor2 + targetAlpha * factor1; 
    

    //
    // Central glow effect

    for( int i = 0; i < maxBlobs; ++i )
    {        
        Vector3 pos = m_centrePos;
        pos.x += sinf(timeIndex+i) * i * 1.7f;
        pos.y += fabs(cosf(timeIndex+i) * cosf(i*20) * 64);
        pos.z += cosf(timeIndex+i) * i * 1.7f;

        float size = 30.0f * sinf(timeIndex+i*13);
        size = max( size, 5.0f );
        
        glColor4f( 0.6f, 0.2f, 0.1f, m_alpha);
        //glColor4f( 0.5f, 0.6f, 0.8f, m_alpha );

        
        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }

    
    //
    // Central starbursts

    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

    int numStars = 10;
    if( buildingDetail == 2 ) numStars = 5;
    if( buildingDetail == 3 ) numStars = 2;
    
    for( int i = 0; i < numStars; ++i )
    {
        Vector3 pos = m_centrePos;
        pos.x += sinf(timeIndex+i) * i * 1.7f;
        pos.y += fabs(cosf(timeIndex+i) * cosf(i*20) * 64);
        pos.z += cosf(timeIndex+i) * i * 1.7f;

        float size = i * 30.0f;
        
        glColor4f( 1.0f, 0.4f, 0.2f, m_alpha );
        //glColor4f( 0.4f, 0.5f, 1.0f, m_alpha );

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }


    //
    // Starbursts on rungs

    if( m_timer > 0.0f )
    {
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

        for( int r = 0; r < 2; ++r )
        {
            Matrix34 rungMat;
            if( r == 0 ) rungMat = GetRungMatrix1();
            else         rungMat = GetRungMatrix2();
    
            for( int i = 0; i < YARD_NUMRUNGSPIKES; ++i )
            {
                Matrix34 spikeMat = m_rungSpikes[i]->GetWorldMatrix(rungMat);
                Vector3 pos = spikeMat.pos;

                for( int j = 0; j < numStars; ++j )
                {
                    float size = sinf(timeIndex+r+i) * j * 5.0f;
                    glColor4f( 0.6f, 0.2f, 0.1f, m_alpha);
                    //glColor4f( 0.4f, 0.5f, 0.9f, m_alpha );

                    glBegin( GL_QUADS );
                        glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
                        glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
                        glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
                        glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
                    glEnd();        
                }
            }
        }    
    }

    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );


//    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//    g_gameFont.DrawText3DCentre( m_pos+Vector3(0,300,0), 20.0f, "Surges : %d", m_numSurges );
//    g_gameFont.DrawText3DCentre( m_pos+Vector3(0,270,0), 20.0f, "Primitives : %d", m_numPrimitives );
//    g_gameFont.DrawText3DCentre( m_pos+Vector3(0,240,0), 20.0f, "Timer : %2.2f", m_timer );
//    g_gameFont.DrawText3DCentre( m_pos+Vector3(0,210,0), 20.0f, "Armour : %d", m_numTanksProduced );
}


bool ConstructionYard::IsPopulationLocked()
{
    Team *team = g_app->m_location->GetMyTeam();
    
    int numArmour = 0;
    for( int i = 0; i < team->m_specials.Size(); ++i )
    {
        WorldObjectId id = *team->m_specials.GetPointer(i);
        Entity *entity = g_app->m_location->GetEntity(id);
        if( entity && entity->m_type == Entity::TypeArmour )
        {
            ++numArmour;
        }
    }

    return( numArmour >= 5 );
}


bool ConstructionYard::AddPrimitive()
{
    if( m_numPrimitives < YARD_NUMPRIMITIVES )
    {
        ++m_numPrimitives;
        return true;
    }    

    return false;
}


void ConstructionYard::AddPowerSurge()
{
    ++m_numSurges;
}


// ============================================================================


DisplayScreen::DisplayScreen()
:   Building()
{
    m_type = TypeDisplayScreen;
    SetShape( g_app->m_resource->GetShape( "displayscreen.shp" ) );

    m_armour = g_app->m_resource->GetShape( "armour.shp" );

    for( int i = 0; i < DISPLAYSCREEN_NUMRAYS; ++i )
    {
        char name[64];
        sprintf( name, "MarkerRay0%d", i+1 );
        m_rays[i] = m_shape->m_rootFragment->LookupMarker( name );
    }
}


void DisplayScreen::RenderAlphas( float _predictionTime )
{
    Building::RenderAlphas( _predictionTime );

    Vector3 armourPos = m_centrePos + Vector3(0,75,0);
    Vector3 armourFront( 0, 0, 1 );
    armourFront.RotateAroundY( g_gameTime * -0.75f );

    Matrix34 armourMat( armourFront, g_upVector, armourPos );
    armourMat.f *= 3.0f;
    armourMat.u *= 3.0f;
    armourMat.r *= 3.0f;

    Vector3 targetPos = armourMat.pos + Vector3(0,50,0);

    glEnable( GL_BLEND );    
    glDisable( GL_CULL_FACE );
    glDepthMask( false );

    //
    // Render black blob

    Vector3 camRight = g_app->m_camera->GetRight();
    Vector3 camUp = g_app->m_camera->GetUp();
    float size = 70.0f;
    glColor4f( 0.4f, 0.3f, 0.4f, 0.0f );
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );

    //glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3fv( (targetPos - camRight * size - camUp * size).GetData() );
        glTexCoord2i(1,0);      glVertex3fv( (targetPos + camRight * size - camUp * size).GetData() );
        glTexCoord2i(1,1);      glVertex3fv( (targetPos + camRight * size + camUp * size).GetData() );
        glTexCoord2i(0,1);      glVertex3fv( (targetPos - camRight * size + camUp * size).GetData() );
    glEnd();

    glDisable( GL_TEXTURE_2D );

    
    //
    // Render rays

    glBlendFunc( GL_SRC_ALPHA, GL_ONE );    
    glShadeModel( GL_SMOOTH );

    for( int i = 0; i < DISPLAYSCREEN_NUMRAYS; ++i )
    {
        Matrix34 buildingMat( m_front, m_up, m_pos );
        Matrix34 rayMat = m_rays[i]->GetWorldMatrix(buildingMat);

        Vector3 rayToArmour = (rayMat.pos - targetPos).Normalise();
        Vector3 right = ( g_app->m_camera->GetPos() - rayMat.pos ) ^ rayToArmour;
        right.Normalise();
        
        glBegin( GL_QUADS );
            glColor4f( 0.9f, 0.9f, 0.9f, 0.5f );
            glVertex3fv( (rayMat.pos - right).GetData() );
            glVertex3fv( (rayMat.pos + right).GetData() );
            
            glColor4f( 0.9f, 0.9f, 0.9f, 0.0f );
            glVertex3fv( (targetPos + right * 30).GetData() );
            glVertex3fv( (targetPos - right * 30).GetData() );
        glEnd();
    }

    glShadeModel( GL_FLAT );


    //
    // Render armour

    glEnable( GL_NORMALIZE );
    
    glBlendFunc( GL_ZERO, GL_SRC_COLOR );
    m_armour->Render( _predictionTime, armourMat );    

    //g_app->m_renderer->SetObjectLighting();    
    //glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    //m_armour->Render( _predictionTime, armourMat );    
    
    g_app->m_renderer->UnsetObjectLighting();
    
    glDisable( GL_NORMALIZE );

}


