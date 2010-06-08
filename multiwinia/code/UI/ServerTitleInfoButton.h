#ifndef __SERVERTITLEINFOBUTTON__
#define __SERVERTITLEINFOBUTTON__

class ServerTitleInfoButton : public DarwiniaButton
{
public:
	ServerTitleInfoButton()
	:	DarwiniaButton()
	{
	}

	void Render( int realX, int realY, bool highlighted, bool clicked )
	{
		glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );
        glBegin( GL_QUADS );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();

		int x = realX;
		int y = realY + (m_h/2.0f);// - (m_fontSize / 4.0f);
		x += m_w * 0.05f;
		g_gameFont.SetRenderOutline(true);
		glColor4f(1.0f,1.0f,1.0f,0.0f);
		g_gameFont.DrawText2D( x, y, m_fontSize, LANGUAGEPHRASE("multiwinia_join_host") );
		g_gameFont.SetRenderOutline(false);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		g_gameFont.DrawText2D( x, y, m_fontSize, LANGUAGEPHRASE("multiwinia_join_host") );

		x += m_w * 0.25f;
		g_gameFont.SetRenderOutline(true);
		glColor4f(1.0f,1.0f,1.0f,0.0f);
		g_gameFont.DrawText2D( x, y, m_fontSize, LANGUAGEPHRASE("multiwinia_join_type") );
		g_gameFont.SetRenderOutline(false);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		g_gameFont.DrawText2D( x, y, m_fontSize, LANGUAGEPHRASE("multiwinia_join_type") );

		x += m_w * 0.1f;
		g_gameFont.SetRenderOutline(true);
		glColor4f(1.0f,1.0f,1.0f,0.0f);
		g_gameFont.DrawText2D( x, y, m_fontSize, LANGUAGEPHRASE("multiwinia_join_map") );
		g_gameFont.SetRenderOutline(false);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		g_gameFont.DrawText2D( x, y, m_fontSize, LANGUAGEPHRASE("multiwinia_join_map") );

		x = realX + m_w * 0.98f;
		g_gameFont.SetRenderOutline(true);
		glColor4f(1.0f,1.0f,1.0f,0.0f);
		g_gameFont.DrawText2DRight( x, y, m_fontSize, LANGUAGEPHRASE("multiwinia_join_players") );
		g_gameFont.SetRenderOutline(false);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		g_gameFont.DrawText2DRight( x, y, m_fontSize, LANGUAGEPHRASE("multiwinia_join_players") );
	}
};


#endif
