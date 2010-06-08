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
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "global_world.h"
#include "location.h"
#include "team.h"
#include "camera.h"
#include "main.h"

#include "sound/soundsystem.h"



GodDish::GodDish()
:   Building(),
    m_timer(0.0),
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
        m_timer *= ( 1.0 - SERVER_ADVANCE_PERIOD * 2.0 );
    }

    return Building::Advance();
}


bool GodDish::IsInView()
{
    if( m_activated ) return true;

    return Building::IsInView();
}


void GodDish::Render( double _predictionTime )
{    
    Building::Render( _predictionTime );
}


void GodDish::RenderAlphas( double _predictionTime )
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

    double timeIndex = g_gameTime * 2;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int maxBlobs = 50;
    if( buildingDetail == 2 ) maxBlobs = 25;
    if( buildingDetail == 3 ) maxBlobs = 10;
    

    //
    // Calculate alpha value

    double alpha = m_timer * 0.1;
    alpha = min( alpha, 1.0 );
    
    //
    // Central glow effect

    for( int i = 0; i < maxBlobs; ++i )
    {        
        Vector3 pos = m_centrePos;
        pos.x += iv_sin(timeIndex+i) * i * 1.7;
        pos.y += iv_abs(iv_cos(timeIndex+i) * iv_cos(i*20) * 64);
        pos.z += iv_cos(timeIndex+i) * i * 1.7;

        double size = 20.0 * iv_sin(timeIndex+i*13);
        size = max( size, 5.0 );
        
        glColor4f( 0.6, 0.2, 0.1, alpha);        
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
        
        glColor4f( 1.0, 0.4, 0.2, alpha );

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }

    glEnable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
}


void GodDish::Activate()
{
    m_activated = true;
    m_timer = 0.0;

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    g_app->m_sepulveda->HighlightBuilding( m_id.GetUniqueId(), "GodDish" );
#endif

    //
    // Make all green darwinians watch us

    Team *team = g_app->m_location->m_teams[0];
    
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

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    g_app->m_sepulveda->ClearHighlights( "GodDish" );
#endif

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
    spam->m_pos += Vector3(0,1500*0.75,900*0.75);
    spam->m_vel = ( m_pos - spam->m_pos );
    spam->m_vel.SetLength( 80.0 );
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

