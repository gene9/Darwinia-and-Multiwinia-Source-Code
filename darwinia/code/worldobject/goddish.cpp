#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/math_utils.h"
#include "lib/preferences.h"

#include "worldobject/goddish.h"
#include "worldobject/spam.h"
#include "worldobject/researchitem.h"
#include "worldobject/darwinian.h"

#include "app.h"
#include "globals.h"
#include "sepulveda.h"
#include "global_world.h"
#include "location.h"
#include "team.h"
#include "camera.h"
#include "main.h"

#include "sound/soundsystem.h"



GodDish::GodDish()
:   Building(),
    m_timer(0.0f),
    m_numSpawned(0),
    m_spawnSpam(false),
    m_activated(false)
{
    m_type = TypeGodDish;
    SetShape( g_app->m_resource->GetShape( "goddish.shp" ) );
}


void GodDish::Initialise( Building *_template )
{
    Building::Initialise( _template );
}


bool GodDish::Advance()
{    
    if( m_activated )
    {
        m_timer += SERVER_ADVANCE_PERIOD;
    }
    else
    {
        m_timer *= ( 1.0f - SERVER_ADVANCE_PERIOD * 2.0f );
    }

    return Building::Advance();
}


bool GodDish::IsInView()
{
    if( m_activated ) return true;

    return Building::IsInView();
}


void GodDish::Render( float _predictionTime )
{    
    Building::Render( _predictionTime );
}


void GodDish::RenderAlphas( float _predictionTime )
{
    Building::RenderAlphas( _predictionTime );   
    
    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();
        
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/cloudyglow.bmp" ) );
    glDisable       ( GL_DEPTH_TEST );

    float timeIndex = g_gameTime * 2;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int maxBlobs = 50;
    if( buildingDetail == 2 ) maxBlobs = 25;
    if( buildingDetail == 3 ) maxBlobs = 10;
    

    //
    // Calculate alpha value

    float alpha = m_timer * 0.1f;
    alpha = min( alpha, 1.0f );
    
    //
    // Central glow effect

    for( int i = 0; i < maxBlobs; ++i )
    {        
        Vector3 pos = m_centrePos;
        pos.x += sinf(timeIndex+i) * i * 1.7f;
        pos.y += fabs(cosf(timeIndex+i) * cosf(i*20) * 64);
        pos.z += cosf(timeIndex+i) * i * 1.7f;

        float size = 20.0f * sinf(timeIndex+i*13);
        size = max( size, 5.0f );
        
        glColor4f( 0.6f, 0.2f, 0.1f, alpha);        
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
        
        glColor4f( 1.0f, 0.4f, 0.2f, alpha );

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }

    glEnable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
}


void GodDish::Activate()
{
    m_activated = true;
    m_timer = 0.0f;

    g_app->m_sepulveda->HighlightBuilding( m_id.GetUniqueId(), "GodDish" );

    //
    // Make all green darwinians watch us

    Team *team = &g_app->m_location->m_teams[0];
    
    for( int i = 0; i < team->m_others.Size(); ++i )
    {
        if( team->m_others.ValidIndex(i) )
        {
            Entity *entity = team->m_others[i];
            if( entity && entity->m_type == Entity::TypeDarwinian )
            {
                Darwinian *darwinian = (Darwinian *) entity;
                darwinian->WatchSpectacle( m_id.GetUniqueId() );
                darwinian->CastShadow( m_id.GetUniqueId() );
            }
        }
    }


    g_app->m_soundSystem->TriggerBuildingEvent( this, "ConnectToGod" );
}


void GodDish::DeActivate()
{
    m_activated = false;

    g_app->m_sepulveda->ClearHighlights( "GodDish" );

    g_app->m_soundSystem->StopAllSounds( m_id, "GodDish ConnectToGod" );
    g_app->m_soundSystem->TriggerBuildingEvent( this, "DisconnectFromGod" );
}


void GodDish::SpawnSpam( bool _isResearch )
{
    Spam spamTemplate;
    int buildingId = g_app->m_globalWorld->GenerateBuildingId();
    spamTemplate.m_id.SetUniqueId( buildingId );
    spamTemplate.m_id.SetUnitId( UNIT_BUILDINGS );

    Spam *spam = (Spam *) CreateBuilding( TypeSpam );
    spam->Initialise( &spamTemplate );
    g_app->m_location->m_buildings.PutData( spam );    

    spam->SendFromHeaven();
    if( _isResearch ) spam->SetAsResearch();
    spam->m_pos = m_pos;                
    spam->m_pos += Vector3(0,1500*0.75f,900*0.75f);
    spam->m_vel = ( m_pos - spam->m_pos );
    spam->m_vel.SetLength( 80.0f );
}


void GodDish::TriggerSpam()
{
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b && b->m_type == TypeSpam )
            {
                Spam *spam = (Spam *) b;
                spam->SpawnInfection();
            }
        }
    }
}


void GodDish::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "ConnectToGod" );
    _list->PutData( "DisconnectFromGod" );
}

