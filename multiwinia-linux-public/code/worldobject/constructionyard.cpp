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
    m_timer(-1.0),
    m_fractionPopulated(0.0),
    m_numTanksProduced(0),
    m_alpha(0.0)
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
        AppReleaseAssert( m_primitives[i], "ConstructionYard: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", name, m_shape->m_name );
    }

    for( int i = 0; i < YARD_NUMRUNGSPIKES; ++i )
    {
        char name[64];
        sprintf( name, "MarkerSpike0%d", i+1 );
        m_rungSpikes[i] = m_rung->m_rootFragment->LookupMarker( name );
        AppReleaseAssert( m_rungSpikes[i], "ConstructionYard: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", name, m_rung->m_name );
    }
}


bool ConstructionYard::Advance()
{
    m_fractionPopulated = (double) GetNumPortsOccupied() / (double) GetNumPorts();

    if( m_fractionPopulated > 0.0 &&
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

    
    if( m_timer < 0.0 )
    {
        // Not currently building anything
        if( m_numPrimitives == YARD_NUMPRIMITIVES &&
            m_numSurges > 50 )
        {
            m_timer = 30.0;
        }
    }
    else
    {
        // Building a tank
        m_timer -= m_fractionPopulated * SERVER_ADVANCE_PERIOD;
        if( m_timer <= 0.0 )
        {
            if( IsPopulationLocked() )
            {
                m_timer = 30.0;
            }
            else
            {
                m_numPrimitives = 0;
                m_numSurges = 1;

                Matrix34 mat( m_front, g_upVector, m_pos );
                Matrix34 prim = m_primitives[5]->GetWorldMatrix( mat );
                WorldObjectId objId = g_app->m_location->SpawnEntities( prim.pos, 2, -1, Entity::TypeArmour, 1, g_zeroVector, 0.0 );
                Entity *entity = g_app->m_location->GetEntity( objId );
                Armour *armour = (Armour *) entity;     
                armour->m_front.Set( 0, 0, 1 );
                armour->m_vel.Zero();
                armour->SetWayPoint( m_pos + Vector3(0,0,500) );
            
                ++m_numTanksProduced;
                m_timer = -1.0;
            }
        }
    }
    
    return Building::Advance();    
}


Matrix34 ConstructionYard::GetRungMatrix1()
{
    if( m_numSurges > 0 )
    {
        double rungHeight = 55.0 + iv_sin(g_gameTime) * 10.0 * m_fractionPopulated;
        Vector3 rungPos = m_pos + Vector3(0,rungHeight,0);
        Vector3 front = m_front;
        front.RotateAroundY( iv_cos(g_gameTime * 0.5) * 0.5 * m_fractionPopulated );

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
        double rungHeight = 110.0 + iv_sin(g_gameTime * 0.8) * 15.0 * m_fractionPopulated;
        Vector3 rungPos = m_pos + Vector3(0,rungHeight,0);
        Vector3 front = m_front;
        front.RotateAroundY( iv_cos(g_gameTime * 0.4) * 0.6 * m_fractionPopulated );
	    Matrix34 mat = Matrix34(front, g_upVector, rungPos);
        return mat;
    }
    else
    {
        Matrix34 mat = Matrix34(m_front, g_upVector, m_pos+Vector3(0,75,0));
        return mat;
    }
}



void ConstructionYard::Render( double _predictionTime )
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
        prim.pos.y += iv_sin(g_gameTime+i) * 5.0;

        m_primitive->Render( _predictionTime, prim );
    }
}


void ConstructionYard::RenderAlphas( double _predictionTime )
{
    Building::RenderAlphas( _predictionTime );   

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();
        
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/cloudyglow.bmp" ) );
    
    double timeIndex = g_gameTime * 2;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int maxBlobs = 50;
    if( buildingDetail == 2 ) maxBlobs = 25;
    if( buildingDetail == 3 ) maxBlobs = 10;
    

    //
    // Calculate alpha value

    double targetAlpha = 0.0;
    if( m_timer > 0.0 )
    {
        targetAlpha = 1.0 - ( m_timer / 60.0 );
    }
    else if( m_numPrimitives > 0 )
    {
        targetAlpha = (m_numPrimitives / 9.0) * 0.5;
    }
    if( m_numSurges > 0 )
    {
        targetAlpha = max( targetAlpha, 0.2 );
    }
    
    double factor1 = g_advanceTime;
    double factor2 = 1.0 - factor1;
    m_alpha = m_alpha * factor2 + targetAlpha * factor1; 
    

    //
    // Central glow effect

    for( int i = 0; i < maxBlobs; ++i )
    {        
        Vector3 pos = m_centrePos;
        pos.x += iv_sin(timeIndex+i) * i * 1.7;
        pos.y += iv_abs(iv_cos(timeIndex+i) * iv_cos(i*20) * 64);
        pos.z += iv_cos(timeIndex+i) * i * 1.7;

        double size = 30.0 * iv_sin(timeIndex+i*13);
        size = max( size, 5.0 );
        
        glColor4f( 0.6, 0.2, 0.1, m_alpha);
        //glColor4f( 0.5, 0.6, 0.8, m_alpha );

        
        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
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
        pos.x += iv_sin(timeIndex+i) * i * 1.7;
        pos.y += iv_abs(iv_cos(timeIndex+i) * iv_cos(i*20) * 64);
        pos.z += iv_cos(timeIndex+i) * i * 1.7;

        double size = i * 30.0;
        
        glColor4f( 1.0, 0.4, 0.2, m_alpha );
        //glColor4f( 0.4, 0.5, 1.0, m_alpha );

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }


    //
    // Starbursts on rungs

    if( m_timer > 0.0 )
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
                    double size = iv_sin(timeIndex+r+i) * j * 5.0;
                    glColor4f( 0.6, 0.2, 0.1, m_alpha);
                    //glColor4f( 0.4, 0.5, 0.9, m_alpha );

                    glBegin( GL_QUADS );
                        glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
                        glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
                        glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
                        glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
                    glEnd();        
                }
            }
        }    
    }

    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );


//    glColor4f( 1.0, 1.0, 1.0, 1.0 );
//    g_gameFont.DrawText3DCentre( m_pos+Vector3(0,300,0), 20.0, "Surges : %d", m_numSurges );
//    g_gameFont.DrawText3DCentre( m_pos+Vector3(0,270,0), 20.0, "Primitives : %d", m_numPrimitives );
//    g_gameFont.DrawText3DCentre( m_pos+Vector3(0,240,0), 20.0, "Timer : %2.2", m_timer );
//    g_gameFont.DrawText3DCentre( m_pos+Vector3(0,210,0), 20.0, "Armour : %d", m_numTanksProduced );
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
        AppReleaseAssert( m_rays[i], "DisplayScreen: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", name, m_shape->m_name );
    }
}


void DisplayScreen::RenderAlphas( double _predictionTime )
{
    Building::RenderAlphas( _predictionTime );

    Vector3 armourPos = m_centrePos + Vector3(0,75,0);
    Vector3 armourFront( 0, 0, 1 );
    armourFront.RotateAroundY( g_gameTime * -0.75 );

    Matrix34 armourMat( armourFront, g_upVector, armourPos );
    armourMat.f *= 3.0;
    armourMat.u *= 3.0;
    armourMat.r *= 3.0;

    Vector3 targetPos = armourMat.pos + Vector3(0,50,0);

    glEnable( GL_BLEND );    
    glDisable( GL_CULL_FACE );
    glDepthMask( false );

    //
    // Render black blob

    Vector3 camRight = g_app->m_camera->GetRight();
    Vector3 camUp = g_app->m_camera->GetUp();
    double size = 70.0;
    glColor4f( 0.4, 0.3, 0.4, 0.0 );
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );

    //glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3dv( (targetPos - camRight * size - camUp * size).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (targetPos + camRight * size - camUp * size).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (targetPos + camRight * size + camUp * size).GetData() );
        glTexCoord2i(0,1);      glVertex3dv( (targetPos - camRight * size + camUp * size).GetData() );
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
            glColor4f( 0.9, 0.9, 0.9, 0.5 );
            glVertex3dv( (rayMat.pos - right).GetData() );
            glVertex3dv( (rayMat.pos + right).GetData() );
            
            glColor4f( 0.9, 0.9, 0.9, 0.0 );
            glVertex3dv( (targetPos + right * 30).GetData() );
            glVertex3dv( (targetPos - right * 30).GetData() );
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


