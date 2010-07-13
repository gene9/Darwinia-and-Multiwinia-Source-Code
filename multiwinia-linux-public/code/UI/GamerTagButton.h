#ifndef __GAMERTAGBUTTON__
#define __GAMERTAGBUTTON__

#include "KickPlayerButton.h"

class GamertagButton : public GameMenuButton
{
public:
	int		m_teamId;
	int		m_teamType;
	double	m_teamChangeTime;

	GamertagButton()
	:	GameMenuButton(UnicodeString()),
		m_teamId(-1),
		m_teamType(-1),
		m_teamChangeTime(0.0)
	{
		m_centered = true;
		m_ir = m_ig = m_ib = 1.0f;
	}

	void MouseUp()
	{
		if( m_teamId == g_app->m_globalWorld->m_myTeamId )
		{
            GameMenuButton::MouseUp();
			GameMenuWindow* gmw = (GameMenuWindow*) m_parent;
			gmw->m_newPage = GameMenuWindow::PagePlayerOptions;
		}
	}

	void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        GameMenuWindow* gmw = (GameMenuWindow*) m_parent;
        if( !m_mouseHighlightMode )
        {
            highlighted = false;
        }

        if( gmw->m_buttonOrder[gmw->m_currentButton] == this )
        {
            highlighted = true;
        }

        UpdateButtonHighlight();
		LobbyTeam *lobbyTeam = &g_app->m_multiwinia->m_teams[m_teamId];
		if( lobbyTeam->m_teamName != UnicodeString() )
		{
			m_caption = lobbyTeam->m_teamName;

            if( lobbyTeam->m_demoPlayer )
            {
                UnicodeString demo = LANGUAGEPHRASE("teamtype_demo");
                demo.Trim();
                m_caption = demo + UnicodeString(" ") + m_caption;            
            }

            if( lobbyTeam->m_ready )
            {
                m_caption = lobbyTeam->m_teamName + UnicodeString(" (") + LANGUAGEPHRASE("multiwinia_ready") + UnicodeString(")");
            }
		}
		else if( lobbyTeam->m_teamType == TeamTypeLocalPlayer )
		{
			m_caption = LANGUAGEPHRASE("teamtype_0");
		}
		else if( lobbyTeam->m_teamType == TeamTypeRemotePlayer )
		{
			m_caption = LANGUAGEPHRASE("teamtype_1");
		}
		else if( lobbyTeam->m_teamType == TeamTypeCPU )
		{
			m_caption = LANGUAGEPHRASE("teamtype_2");
		}
		else if( lobbyTeam->m_teamType == TeamTypeSpectator )
		{
			m_caption = LANGUAGEPHRASE("Spectator");
		}

		else
		{
			if( gmw->m_singlePlayer )
				m_caption = LANGUAGEPHRASE("teamtype_2");
			else
				m_caption = LANGUAGEPHRASE("multiwinia_open_or_cpu");
		}

        if( gmw->m_currentButton != -1 && lobbyTeam->m_teamType == TeamTypeRemotePlayer )
        {
            EclButton *b = gmw->m_buttonOrder[ gmw->m_currentButton ];
            if( strstr( b->m_name, "kick" ) && EclMouseInButton( m_parent, b ) )
            {
                KickPlayerButton *kick = (KickPlayerButton *)b;
                if( kick->m_teamId == m_teamId )
                {
                    m_caption = LANGUAGEPHRASE("multiwinia_clicktokick");
                }
            }
        }

		if( lobbyTeam->m_teamType == TeamTypeCPU || lobbyTeam->m_teamType == TeamTypeUnused ) 
        {
            m_inactive = true;
            highlighted = false;
        }
		else m_inactive = false;

        RGBAColour white( 255,255,255,255);
        RGBAColour col = lobbyTeam->m_colour;
        if( /*(g_app->m_multiwinia->m_coopMode || gmw->m_gameType == Multiwinia::GameTypeAssault ) &&*/
            lobbyTeam->m_colourId == -1)
        {
            glColor4f( 0.7f, 0.7f, 0.7f, 1.0f );
        }
        else
        {
            if( lobbyTeam->m_teamType == TeamTypeCPU || 
                lobbyTeam->m_teamType == TeamTypeUnused )
            {
                col *= 0.25f;
            }

            if( !(m_teamType == TeamTypeUnused &&
                lobbyTeam->m_teamType == TeamTypeCPU ) )
            {

                if( lobbyTeam->m_teamType != m_teamType )
                {
                    m_teamChangeTime = GetHighResTime();
					m_teamType = lobbyTeam->m_teamType;
                }

                if( m_teamChangeTime > 0.0f )
                {
                    float timeDiff = GetHighResTime() - m_teamChangeTime;
                    if( timeDiff < 1.0f )
                    {
                        col = (white * (1.0f - timeDiff)) + ( col * timeDiff );
                    }
                    else
                    {
                        m_teamChangeTime = 0.0f;
                    }
                }
            }

            glColor4ubv( col.GetData() );
        }


        glShadeModel( GL_SMOOTH);
        glBegin( GL_QUADS);
            glVertex2f( realX, realY );
            glVertex2f( realX, realY + m_h );
            if( highlighted )
            {
                col = col * 0.5f + white * 0.5f;
                glColor4ubv( col.GetData() );
            }
            else
            {
                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            }
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX + m_w, realY );
        glEnd();
        glShadeModel( GL_FLAT );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        if( lobbyTeam->m_teamType == TeamTypeCPU || 
            lobbyTeam->m_teamType == TeamTypeUnused )
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.3f );
        }
        glBegin( GL_LINE_LOOP );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();


		int textY = realY + m_h/2.0f;
		int textX = realX + m_w * 0.05f;
		glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2D( textX, textY, m_fontSize, m_caption );

		if( m_inactive )
		{
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		}
		else if( highlighted ) //gmw && gmw->m_currentButton == gmw->m_buttonOrder.FindData(this) )
		{
            glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
		}
		else
		{
			glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );
		}
        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2D( textX, textY, m_fontSize, m_caption );
	}
};

#endif
