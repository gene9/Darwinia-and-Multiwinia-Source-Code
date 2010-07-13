#ifndef __SPECTATORBUTTON__
#define __SPECTATORBUTTON__

class CSpectatorButton : public GameMenuButton
{
public:
	int		m_teamId;
	int		m_teamType;
	double	m_teamChangeTime;

	CSpectatorButton()
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
        GameMenuButton::MouseUp();
		
		if(g_app->m_globalWorld->m_myTeamId != 255)
		{
			g_app->m_clientToServer->RequestToRegisterAsSpectator(g_app->m_globalWorld->m_myTeamId);
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


        RGBAColour white( 255,255,255,255);
        RGBAColour col( 0,0,0,255);

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
