#ifndef __UNLOCKFULLGAMEBUTTON__
#define __UNLOCKFULLGAMEBUTTON__

#include "lib/window_manager.h"
#ifdef TARGET_MSVC
	#include "lib/input/win32_eventhandler.h"
#endif

class UnlockFullGameButton : public GameMenuButton
{
public:
    char m_url[512];

public:
    UnlockFullGameButton()
    :   GameMenuButton("")
    {
        strcpy( m_url, "not set" );
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        glBegin( GL_QUADS );
        glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
        glVertex2f( realX, realY );
        glVertex2f( realX+m_w, realY );
        glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );
        glVertex2f( realX+m_w, realY+m_h );
        glVertex2f( realX, realY+m_h );
        glEnd();

        glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
        glBegin( GL_LINE_LOOP );
        glVertex2f( realX, realY );
        glVertex2f( realX+m_w, realY );
        glVertex2f( realX+m_w, realY+m_h );
        glVertex2f( realX, realY+m_h );
        glEnd();

        GameMenuButton::Render( realX, realY, false, false );


        //
        // Do our own highlight

        if( highlighted || clicked )
        {
            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
            glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );
            glBegin( GL_QUADS );
            glVertex2f( realX, realY );
            glVertex2f( realX+m_w, realY );
            glVertex2f( realX+m_w, realY+m_h );
            glVertex2f( realX, realY+m_h );
            glEnd();
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }

        //
        // Render URL we're about to visit

        float fontH = m_h * 0.13f;
        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_editorFont.SetRenderOutline(true);
        g_editorFont.DrawText2DCentre( realX + m_w/2, realY + m_h - fontH, fontH, m_url );
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_editorFont.SetRenderOutline(false);
        g_editorFont.DrawText2DCentre( realX + m_w/2, realY + m_h - fontH, fontH, m_url );


        //
        // Joypad stuff

        if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
        {
            float iconSize = m_fontSize;
            float iconGap = 10.0f;
            float iconAlpha = 1.0f;
            int xPos = realX + g_gameFont.GetTextWidth( m_caption.Length(), m_fontSize ) + 60;
            int yPos = realY + (m_h / 2.0f);

            if( m_centered )
            {
                xPos += m_w / 2.0f;
            }

            Vector2 iconCentre = Vector2(xPos, yPos);            

            // Render the icon

            glEnable        ( GL_TEXTURE_2D );
            glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_a.bmp" ) );
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            glColor4f   ( 1.0f, 1.0f, 1.0f, iconAlpha );

            float x = iconSize/2;

            glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + x, iconCentre.y + iconSize/2 );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - x, iconCentre.y + iconSize/2 );
            glEnd();

            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glDisable       ( GL_TEXTURE_2D );
        }
    }


    void MouseUp()
    {
        GameMenuButton::MouseUp();

        g_windowManager->OpenWebsite( m_url );


        int windowed = g_prefsManager->GetInt( "ScreenWindowed", 1 );
        if( !windowed )
        {
            // Switch to windowed mode if required
            g_prefsManager->SetInt( "ScreenWindowed", 1 );
            g_prefsManager->SetInt( "ScreenWidth", 1024 );
            g_prefsManager->SetInt( "ScreenHeight", 768 );

            g_windowManager->DestroyWin();
            delete g_app->m_renderer;
            g_app->m_renderer = new Renderer();
            g_app->m_renderer->Initialise();
#ifdef TARGET_MSVC
            getW32EventHandler()->ResetWindowHandle();
#endif
            g_app->m_resource->FlushOpenGlState();
            g_app->m_resource->RegenerateOpenGlState();

            g_prefsManager->Save();        

            EclInitialise( 1024, 768);

            LList<EclWindow *> *windows = EclGetWindows();
            while (windows->Size() > 0) 
            {
                EclWindow *w = windows->GetData(0);
                EclRemoveWindow(w->m_name);
            }

            g_app->m_gameMenu->DestroyMenu();
            g_app->m_gameMenu->CreateMenu();
        }
    }
};

#endif
