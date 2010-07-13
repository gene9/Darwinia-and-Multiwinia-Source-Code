#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"

#include "app.h"
#include "location.h"
#include "team.h"
#include "multiwinia.h"
#include "rocket_status_panel.h"
#include "main.h"

#include "worldobject/rocket.h"



RocketStatusPanel::RocketStatusPanel()
:   m_x(0),
    m_y(0),
    m_w(0),
    m_teamId(-1),
    m_damageTimer(0.0f),
    m_lastDamage(0.0f),
    m_previousFuelLevel(0.0f)
{
}


EscapeRocket *RocketStatusPanel::GetMyRocket()
{
    if( m_teamId >= 0 && m_teamId < NUM_TEAMS )
    {
        LobbyTeam *team = &g_app->m_multiwinia->m_teams[ m_teamId ];
        
        Building *building = g_app->m_location->GetBuilding( team->m_rocketId );

        if( building && 
            building->m_type == Building::TypeEscapeRocket &&
            building->m_id.GetTeamId() == m_teamId )
        {
            return (EscapeRocket *)building;
        }
    }

    return NULL;
}


void RocketStatusPanel::Render()
{
    //
    // Determine our rocket status

    EscapeRocket *rocket = GetMyRocket();
    if( !rocket ) return;

    Team *team = g_app->m_location->m_teams[ m_teamId ];
    if( !team ) return;

    float fuelPercent = rocket->m_fuel / 100.0f;
    int darwiniansInside = rocket->m_passengers;

    if( rocket->m_damage > m_lastDamage )
    {
        m_damageTimer = GetHighResTime();
    }
    m_lastDamage = rocket->m_damage;

    if( fuelPercent > 1.0f ) fuelPercent = 1.0f;

    double refuelRate = rocket->m_fuel - m_previousFuelLevel;
    m_previousFuelLevel = rocket->m_fuel;

    float h = m_w * 1.5f;
    glShadeModel( GL_SMOOTH );


    //
    // Background team colour

    glColor4ub( team->m_colour.r*0.2f, team->m_colour.g*0.2f, team->m_colour.b*0.2f, 200 );

    glBegin( GL_QUADS );
        glVertex2f( m_x, m_y );
        glVertex2f( m_x + m_w, m_y );
        glVertex2f( m_x + m_w, m_y + h );
        glVertex2f( m_x, m_y + h );
    glEnd();


    //
    // Refueling effect

    float fuelBase = m_y + h * 0.97f;
    float fuelFullH = h * 0.95f;
    float fuelH = fuelFullH * fuelPercent;
    int refuelAlpha = 128;

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );


    if( fuelPercent < 1.0f )
    {
        glColor4ub( team->m_colour.r, team->m_colour.g, team->m_colour.b, refuelAlpha );

        float texY = fuelPercent * -100 + 0.5f;
        float texH = 1.0f;

        glBegin( GL_QUADS );
            glTexCoord2f(0,texY);       glVertex2f( m_x, fuelBase );
            glTexCoord2f(1,texY);       glVertex2f( m_x + m_w, fuelBase );
            glTexCoord2f(1,texY+texH);  glVertex2f( m_x + m_w, fuelBase - fuelFullH );
            glTexCoord2f(0,texY+texH);  glVertex2f( m_x, fuelBase - fuelFullH );
        glEnd();

    }


    //
    // Fuel level

    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser-long.bmp" ) );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    int fuelAlpha = 200;

    glBegin( GL_QUADS );
        glColor4ub( 0,0,0, fuelAlpha );
        glTexCoord2f( 0, 0.3f );       glVertex2f( m_x, fuelBase );
        glTexCoord2f( 0, 0.7f );       glVertex2f( m_x + m_w, fuelBase );
        
        glColor4ub( team->m_colour.r, team->m_colour.g, team->m_colour.b, fuelAlpha );
        glTexCoord2f( 1, 0.7f );       glVertex2f( m_x + m_w, fuelBase - fuelH );
        glTexCoord2f( 1, 0.3f );       glVertex2f( m_x, fuelBase - fuelH );
    glEnd();

    glDisable( GL_TEXTURE_2D );


    //
    // Shadow above fuel level

    glBegin( GL_QUADS );
        glColor4ub( 0,0,0, fuelAlpha*0.5f );
        glVertex2f( m_x, fuelBase - fuelH );
        glVertex2f( m_x + m_w, fuelBase - fuelH );
        glColor4ub( 0,0,0, 0 );
        glVertex2f( m_x + m_w, fuelBase - fuelH - h * 0.05f );
        glVertex2f( m_x, fuelBase - fuelH - 10 - h * 0.05f );
    glEnd();


    //
    // Rocket bitmap overlay

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/rocketstatuspanel.bmp" ) );
    
    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex2f( m_x, m_y );
        glTexCoord2i(1,1);      glVertex2f( m_x + m_w, m_y );
        glTexCoord2i(1,0);      glVertex2f( m_x + m_w, m_y + h );
        glTexCoord2i(0,0);      glVertex2f( m_x, m_y + h );
    glEnd();
    
    glDisable       ( GL_TEXTURE_2D );



    //
    // Damage effect

    glColor4f( 1.0f, 1.0f, 1.0f, rocket->m_damage/100.0f );

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/rocketcracked.bmp" ) );

    glBegin( GL_QUADS );
    glTexCoord2i(0,1);      glVertex2f( m_x, m_y );
    glTexCoord2i(1,1);      glVertex2f( m_x + m_w, m_y );
    glTexCoord2i(1,0);      glVertex2f( m_x + m_w, m_y + h );
    glTexCoord2i(0,0);      glVertex2f( m_x, m_y + h );
    glEnd();

    glDisable       ( GL_TEXTURE_2D );



    //
    // Darwinians inside
    
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    if( fuelPercent >= 1.0f || darwiniansInside > 0 )
    {
        float dwX = m_x + m_w * 0.25f;
        float dwW = m_w * 0.45f;
        float dwY = m_y + h * 0.225f;
        float dwH = h * 0.55f;
        float s = h * 0.04f;
        int astronautAlpha = 255;

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );

        for( int i = 99; i >= 0; i-- )
        {
            int xIndex = ( i % 10 );
            int yIndex = 9 - int( i / 10 );
            float xPos = dwX + dwW * xIndex/10;
            float yPos = dwY + dwH * yIndex/10;

            if( yIndex % 2 == 0 ) xPos += dwW/20.0f;

            RGBAColour astronautCol = team->m_colour;
            astronautCol.AddWithClamp( RGBAColour(50,50,50,255) );
            astronautCol.a = astronautAlpha;

            if( i <  darwiniansInside )     glColor4ub( team->m_colour.r, team->m_colour.g, team->m_colour.b, astronautAlpha );
            else                            glColor4ub( team->m_colour.r*0.3f, team->m_colour.g*0.3f, team->m_colour.b*0.3f, astronautAlpha*0.2f );

            Vector3 pos( xPos+s/2.0f, yPos+s/2.0f, 0 );
            pos.x += sinf(i + GetHighResTime()) * 1.0f;
            pos.y += cosf(i + i + GetHighResTime()) * 1.0f;

            Vector3 offset( -s/2.0f, -s, 0 );
            
            glBegin( GL_QUADS );
                glTexCoord2f(0,1);      glVertex2dv( (pos+offset).GetData() );      offset.RotateAroundZ(0.5f * M_PI);
                glTexCoord2f(1,1);      glVertex2dv( (pos+offset).GetData() );      offset.RotateAroundZ(0.5f * M_PI);
                glTexCoord2f(1,0);      glVertex2dv( (pos+offset).GetData() );      offset.RotateAroundZ(0.5f * M_PI);
                glTexCoord2f(0,0);      glVertex2dv( (pos+offset).GetData() );      offset.RotateAroundZ(0.5f * M_PI);
            glEnd();
        }

        glDisable( GL_TEXTURE_2D );
    }


    //
    // Engine effect

    if( rocket->m_state == EscapeRocket::StateReady ||
        rocket->m_state == EscapeRocket::StateCountdown ||
        rocket->m_state == EscapeRocket::StateFlight)
    {
        float flameX = m_x + m_w * 0.35f;
        float flameY = m_y + h * 0.8f;
        float flameW = m_w * 0.3f;
        float flameH = m_w * 0.3f;

        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/muzzleflash.bmp" ) );

        if( fmodf( GetHighResTime()*30, 1.0f ) < 0.5f ) glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        else                                             glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex2f( flameX, flameY );
            glTexCoord2i(0,1);      glVertex2f( flameX+flameW, flameY );
            glTexCoord2i(1,1);      glVertex2f( flameX+flameW, flameY+flameH );
            glTexCoord2i(1,0);      glVertex2f( flameX, flameY+flameH );
        glEnd();

        glDisable( GL_TEXTURE_2D );
    }


    //
    // Captions at the bottom
    // Determine our caption

    float mainCaptionY = m_y + h * 0.6f;
    float mainCaptionH = h * 0.05f;
    float mainCaptionG = mainCaptionH * 0.1f;

    bool timeFlashEffect = (fmodf( GetHighResTime() * 2, 1.0f ) > 0.55f);

    UnicodeString caption;
    RGBAColour captionColour(255,255,255,255);

    if( GetHighResTime() - m_damageTimer < 10.0f && 
        rocket->m_state != EscapeRocket::StateExploding )
    {
        char damage[256];
        sprintf( damage, "%d%%", int(rocket->m_damage) );
        caption = LANGUAGEPHRASE("multiwinia_rr_status_c");
        caption.ReplaceStringFlag( L'T', damage );

        captionColour.Set(255,0,0,255);  
        if( timeFlashEffect ) captionColour.a *= 0.5f;
    }
    else if( rocket->m_state == EscapeRocket::StateCountdown )
    {        
        char captionC[256];
        sprintf( captionC, "%d", (int)rocket->m_countdown + 1 );
        caption = captionC;
        mainCaptionH *= 4;
    }
    else if( rocket->m_state == EscapeRocket::StateFlight )
    {
        caption = LANGUAGEPHRASE("multiwinia_rr_status_d" );
        mainCaptionH *= 1.5f;
        if( timeFlashEffect ) captionColour.a *= 0.25f;
    }
    else if( rocket->m_state == EscapeRocket::StateExploding )
    {
        caption = LANGUAGEPHRASE("multiwinia_rr_status_e" );
        if( timeFlashEffect ) captionColour.a *= 0.25f;
    }
    else if( fuelPercent >= 1.0f && darwiniansInside < 5 )
    {
        caption = LANGUAGEPHRASE("multiwinia_rr_status_b");
        if( timeFlashEffect ) captionColour.a *= 0.25f;
    } 
    else if( rocket->m_refuelRate < 0.05f && fuelPercent < 0.01f )
    {
        caption = LANGUAGEPHRASE("multiwinia_rr_status_a");        
        if( timeFlashEffect ) captionColour.a *= 0.25f;
    }
    else if( fuelPercent < 1.0f )
    {
        char captionC[256];
        sprintf( captionC, "%2.1f%%", fuelPercent * 100 );
        
        caption = LANGUAGEPHRASE("multiwinia_rr_status_f");
        caption.ReplaceStringFlag( L'T', captionC );

        captionColour.a *= 0.75f;
    }
       


    //
    // Render our caption

    if( caption.Length() )
    {
        LList<UnicodeString *> *wrapped = WordWrapText( caption, 1000, mainCaptionH, false, false );

        for( int i = 0; i < wrapped->Size(); ++i )
        {
            UnicodeString *thisString = wrapped->GetData(i);

            glColor4ub( captionColour.a, captionColour.a, captionColour.a, 0 );
            g_titleFont.SetRenderOutline(true);
            g_titleFont.DrawText2DCentre( m_x + m_w/2, mainCaptionY, mainCaptionH, *thisString );

            glColor4ubv( captionColour.GetData() );
            g_titleFont.SetRenderOutline(false);
            g_titleFont.DrawText2DCentre( m_x + m_w/2, mainCaptionY, mainCaptionH, *thisString );

            mainCaptionY += mainCaptionH;
            mainCaptionY += mainCaptionG;
        }

        wrapped->EmptyAndDelete();
        delete wrapped;
    }


    //
    // White border

    glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );

    glBegin( GL_LINE_LOOP );
        glVertex2f( m_x, m_y );
        glVertex2f( m_x + m_w, m_y );
        glVertex2f( m_x + m_w, m_y + h );
        glVertex2f( m_x, m_y + h );
    glEnd();

    glShadeModel( GL_FLAT );
}

