#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/window_manager.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"

#include "sound/sound_library_2d.h"
#include "sound/sound_library_3d.h"

#include "app.h"
#include "renderer.h"

#include "interface/demoend_window.h"


class BuyOnlineButton : public DarwiniaButton
{
    void MouseUp()
    {
        DemoEndWindow *window = (DemoEndWindow *) m_parent;
        if( window->m_saveGame ) g_app->SaveProfile( true, true );

     //   g_soundLibrary2d->Stop();
	    //delete g_soundLibrary3d; g_soundLibrary3d = NULL;
	    //delete g_soundLibrary2d; g_soundLibrary2d = NULL;

     //   delete g_app->m_resource;
     //   delete g_windowManager;

        WindowManager::OpenWebsite( "http://store.introversion.co.uk" );
 
		g_app->m_requestQuit = true;
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        DemoEndWindow *window = (DemoEndWindow *) m_parent;
        float alpha = window->GetAlpha();

        float buttonAlpha = alpha;
        if( !highlighted && !clicked ) buttonAlpha *= 0.7f;

        //
        // Fill
                
        glShadeModel( GL_SMOOTH );
        glBegin( GL_QUADS );
            glColor4f( 0.2f, 0.2f, 0.5f, buttonAlpha );        glVertex2i( realX, realY );
            glColor4f( 0.5f, 0.5f, 0.8f, buttonAlpha );        glVertex2i( realX+m_w, realY );
            glColor4f( 0.2f, 0.2f, 0.5f, buttonAlpha );        glVertex2i( realX+m_w, realY+m_h );
            glColor4f( 0.5f, 0.5f, 0.8f, buttonAlpha );        glVertex2i( realX, realY+m_h );
        glEnd();
        glShadeModel( GL_FLAT );

        //
        // Outline

        glColor4f( 1.0f, 1.0f, 1.0f, buttonAlpha );
        glBegin( GL_LINE_LOOP );
            glVertex2i( realX, realY );
            glVertex2i( realX+m_w, realY );
            glVertex2i( realX+m_w, realY+m_h );
            glVertex2i( realX, realY+m_h );
        glEnd();


        //
        // Caption

        float textSize = m_h * 0.5f;
        
        for( int i = 0; i < 2; ++i )
        {
            if( i == 0 )
            {
                glColor4f( alpha, alpha, alpha, 0.0f );
                g_gameFont.SetRenderOutline(true);
            }
            if( i == 1 )
            {
                g_gameFont.SetRenderOutline(false);
                glColor4f( 1.0f, 1.0f, 1.0f, alpha );
            }
        
            g_gameFont.DrawText2DCentre( realX+m_w/2+13, realY+m_h/2-25, textSize, m_caption );
			g_gameFont.DrawText2DCentre( realX+m_w/2, realY+m_h/2+30, 11, "http://store.introversion.co.uk" );

        }
    }
};


class ExitDemoButton : public DarwiniaButton
{
    void MouseUp()
    {
        DemoEndWindow *window = (DemoEndWindow *) m_parent;
        if( window->ShowExitButton() )
        {
            if( window->m_saveGame ) g_app->SaveProfile( true, true );

            g_app->m_requestQuit = true;

         //   g_soundLibrary2d->Stop();
	        //delete g_soundLibrary3d; g_soundLibrary3d = NULL;
	        //delete g_soundLibrary2d; g_soundLibrary2d = NULL;

         //   delete g_app->m_resource;
         //   delete g_windowManager;
        }
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        DemoEndWindow *window = (DemoEndWindow *) m_parent;
        float alpha = window->GetAlpha();

        float buttonAlpha = alpha;
        if( !highlighted && !clicked ) buttonAlpha *= 0.7f;

        if( !window->ShowExitButton() ) 
        {
            alpha = min( 0.1f, alpha );
            buttonAlpha = min( 0.1f, buttonAlpha );
        }

        //
        // Fill
            
        glShadeModel( GL_SMOOTH );
        glBegin( GL_QUADS );
            glColor4f( 0.2f, 0.2f, 0.5f, buttonAlpha );        glVertex2i( realX, realY );
            glColor4f( 0.5f, 0.5f, 0.8f, buttonAlpha );        glVertex2i( realX+m_w, realY );
            glColor4f( 0.2f, 0.2f, 0.5f, buttonAlpha );        glVertex2i( realX+m_w, realY+m_h );
            glColor4f( 0.5f, 0.5f, 0.8f, buttonAlpha );        glVertex2i( realX, realY+m_h );
        glEnd();
        glShadeModel( GL_FLAT );

        //
        // Outline

        glColor4f( 1.0f, 1.0f, 1.0f, buttonAlpha );
        glBegin( GL_LINE_LOOP );
            glVertex2i( realX, realY );
            glVertex2i( realX+m_w, realY );
            glVertex2i( realX+m_w, realY+m_h );
            glVertex2i( realX, realY+m_h );
        glEnd();


        //
        // Caption

        float textSize = m_h * 0.5f;
    
        glColor4f( alpha, alpha, alpha, 0.0f );
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2DCentre( realX+m_w/2+8, realY+m_h/2-3, textSize, m_caption );
        
        g_gameFont.SetRenderOutline(false);
        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        g_gameFont.DrawText2DCentre( realX+m_w/2+8, realY+m_h/2-3, textSize, m_caption );        
    }
};


class ReturnToGameButton : public DarwiniaButton
{
    void MouseUp()
    {
        EclRemoveWindow( m_parent->m_name );
    }
};


DemoEndWindow::DemoEndWindow( float _fadeInTime, bool _saveGame )
:   DarwiniaWindow( "DemoEnd" ),
    m_saveGame(_saveGame)
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    SetSize( 800, 600 );
    SetPosition(screenW/2-m_w/2,
                screenH/2-m_h/2);
    
    m_timer = GetHighResTime();
    m_fadeInTime = _fadeInTime;
}


void DemoEndWindow::Create()
{
    ExitDemoButton *exit = new ExitDemoButton();
	int buttonW = 210;
    exit->SetShortProperties( LANGUAGEPHRASE("launchpad_button_exitdemo"), m_w/2-buttonW/2, 520, buttonW, 40 );
    RegisterButton( exit );
    
    BuyOnlineButton *buy = new BuyOnlineButton();
	buttonW = 420;
    buy->SetShortProperties( LANGUAGEPHRASE("launchpad_button_buynow"), m_w/2-buttonW/2, 360, buttonW, 100 );
    RegisterButton( buy );
}


float DemoEndWindow::GetAlpha()
{
    float timeNow = GetHighResTime();

    float alpha = ( timeNow - m_timer ) / m_fadeInTime;
    alpha = max( alpha, 0.0f );
    alpha = min( alpha, 0.9f );

    return alpha;
}


bool DemoEndWindow::ShowExitButton()
{
    float timePassed = GetHighResTime() - m_timer;
    return( timePassed > 4 );
}


void DemoEndWindow::Render( bool hasFocus )
{    
    float alpha = GetAlpha();
    
    //
    // Main body fill

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_red.bmp" ) );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    float texH = 1.0f;
    float texW = texH * 512.0f / 64.0f;

    glColor4f( 1.0f, 1.0f, 1.0f, alpha );
    glBegin( GL_QUADS );
        glTexCoord2f( 0.0f, 0.0f );     glVertex2f( m_x, m_y );
        glTexCoord2f( texW, 0.0f );     glVertex2f( m_x + m_w, m_y );
        glTexCoord2f( texW, texH );     glVertex2f( m_x + m_w, m_y + m_h );
        glTexCoord2f( 0.0f, texH );     glVertex2f( m_x, m_y + m_h );
    glEnd();

    glDisable       ( GL_TEXTURE_2D );

    //
    // Outline border

    glColor4f( 1.0f, 1.0f, 1.0f, alpha );

    glBegin( GL_LINE_LOOP );
        glVertex2i( m_x, m_y );
        glVertex2i( m_x+m_w, m_y );
        glVertex2i( m_x+m_w, m_y+m_h );
        glVertex2i( m_x, m_y+m_h );
    glEnd();

    for( int i = 0; i < 2; ++i )
    {
        if( i == 0 ) 
        {
            glColor4f( alpha, alpha, alpha, 0.0 );
            g_gameFont.SetRenderOutline(true);
        }

        if( i == 1 ) 
        {
            glColor4f( 1.0f, 1.0f, 1.0f, alpha );
            g_gameFont.SetRenderOutline(false);
        }

        g_gameFont.DrawText2DCentre( m_x+m_w/2+20, m_y+50,  70, LANGUAGEPHRASE("launchpad_buyme_1") );
        g_gameFont.DrawText2DCentre( m_x+m_w/2+10, m_y+150, 20, LANGUAGEPHRASE("launchpad_buyme_2") );
        g_gameFont.DrawText2DCentre( m_x+m_w/2+10, m_y+180, 20, LANGUAGEPHRASE("launchpad_buyme_3") );
        g_gameFont.DrawText2DCentre( m_x+m_w/2+10, m_y+250, 20, LANGUAGEPHRASE("launchpad_buyme_4") );
        g_gameFont.DrawText2DCentre( m_x+m_w/2+10, m_y+280, 20, LANGUAGEPHRASE("launchpad_buyme_5") );
    }


    //
    // Render buttons

    EclWindow::Render( hasFocus );
}

