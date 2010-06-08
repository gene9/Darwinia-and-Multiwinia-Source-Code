#ifndef _included_kickplayerbutton_h
#define _included_kickplayerbutton_h

class KickPlayerButton : public DarwiniaButton
{
public:
    int m_teamId;

    KickPlayerButton()
    :   DarwiniaButton(),
        m_teamId(-1)
    {
    }

    bool TeamExists()
    {
        return g_app->m_multiwinia->m_teams[m_teamId].m_teamType == TeamTypeRemotePlayer;
    }

    void MouseUp()
    {
        if( !TeamExists() ) return;
        if( !g_app->m_server ) return;
        
        LobbyTeam *team = &g_app->m_multiwinia->m_teams[m_teamId];
        if( team->m_disconnected ) return;

        g_app->m_server->KickClient( team->m_clientId );        
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        if( !TeamExists() ) return;

        GameMenuWindow *main = (GameMenuWindow *) m_parent;
        if( highlighted && g_inputManager->getInputMode() != INPUT_MODE_KEYBOARD )
        {
            // ignore any mouse inputs if we are using the gamepad
            highlighted = false;
        }

        UpdateButtonHighlight();

        if( main->m_buttonOrder.ValidIndex(main->m_currentButton) &&
            main->m_buttonOrder[main->m_currentButton] == this )
        {
            highlighted = true;
        }

        if( highlighted )
        {
            glColor4f( 0.75f, 0.75f, 1.0f, 0.5f );

            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glBegin( GL_QUADS);
                glVertex2f( realX, realY );
                glVertex2f( realX + m_w, realY );
                glVertex2f( realX + m_w, realY + m_h );
                glVertex2f( realX, realY + m_h );
            glEnd();
        }

        if( g_app->m_resource->DoesTextureExist( "icons/mouse_disabled.bmp" ) )
        {        
            int texId = g_app->m_resource->GetTexture( "icons/mouse_disabled.bmp" );

            glEnable        (GL_TEXTURE_2D);
	        glEnable        (GL_BLEND);
            //glDisable       (GL_CULL_FACE);
            glDepthMask     (false);

            if( g_app->m_resource->DoesTextureExist( "shadow_icons/mouse_disabled.bmp" ) )
            {
                glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "shadow_icons/mouse_disabled.bmp" ) );
                glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
                glBegin( GL_QUADS );
                    glTexCoord2i(0,1);      glVertex2f( realX, realY );
                    glTexCoord2i(1,1);      glVertex2f( realX + m_w, realY );
                    glTexCoord2i(1,0);      glVertex2f( realX + m_w, realY + m_h );
                    glTexCoord2i(0,0);      glVertex2f( realX, realY + m_h );
                glEnd();
            }

            glBindTexture( GL_TEXTURE_2D, texId );	
            glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE );

            glBegin( GL_QUADS );
                glTexCoord2i(0,1);      glVertex2f( realX, realY );
                glTexCoord2i(1,1);      glVertex2f( realX + m_w, realY );
                glTexCoord2i(1,0);      glVertex2f( realX + m_w, realY + m_h );
                glTexCoord2i(0,0);      glVertex2f( realX, realY + m_h );
            glEnd();

            glDepthMask     (true);
            glDisable       (GL_BLEND);
//	        glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);							
	        glDisable       (GL_TEXTURE_2D);
            //glEnable        (GL_CULL_FACE);
        }  
    }
};

#endif