#ifndef __SCROLLINGTEXTBUTTON__
#define __SCROLLINGTEXTBUTTON__

class ScrollingTextButton : public DarwiniaButton
{
public:

    int             m_currentButton;
    int             m_scrollSpeed;
    float           m_scrollTimer;
    UnicodeString   m_string;
	bool			m_ignoreButtons;
    LList<UnicodeString *> *m_wrapped;

    ScrollingTextButton()
    :   DarwiniaButton(),
        m_currentButton(-1),
        m_scrollTimer(0.0f),
        m_scrollSpeed(30),
		m_ignoreButtons(false),
        m_wrapped(NULL)
    {
    }

    ~ScrollingTextButton()
    {
        ClearWrapped();
    }

    void ClearWrapped()
    {
        if( m_wrapped )
        {
            m_wrapped->EmptyAndDelete();
		    delete m_wrapped;
            m_wrapped = NULL;
        }
    }

    virtual void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        g_app->m_renderer->Clip( realX, realY, m_w, m_h );
        /*glScissor( realX, g_app->m_renderer->ScreenH() - m_h - realY, m_w, m_h );
        glEnable( GL_SCISSOR_TEST );            */

        DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;
		if( !m_ignoreButtons )
		{
			int currentButton = parent->m_currentButton;
			if( currentButton != m_currentButton )
			{
				m_currentButton = currentButton;
				m_scrollTimer = GetHighResTime();
			}        
		}

        int gap = m_fontSize * 1.2f;
        int space = m_h * 0.05f;
        if( space < m_fontSize ) space = m_fontSize * 1.1f;
        int x = realX + m_w * 0.05f;
        int y = realY + space;

        if( m_string.Length() > 0 )
        {
            if( !m_wrapped )
            {
                int fontWidth = g_editorFont.GetTextWidth(1,m_fontSize);
                m_wrapped = WordWrapText( m_string, m_w * 0.95f, fontWidth );
            }
            
            int height = (m_h * 0.05f) + (m_wrapped->Size() * gap);
            if( height > m_h )
            {
                if( GetHighResTime() - m_scrollTimer >= 1.5f )
                {
                    float time = GetHighResTime() - m_scrollTimer - 1.5f;
                    int trueY = y;
                    y -= time * m_scrollSpeed;

                    if( y + height < trueY )
                    {
                        y = trueY;
                        m_scrollTimer = GetHighResTime();
                    }
                }
            }

            for( int i = 0; i < m_wrapped->Size(); ++i )
            {
                UnicodeString *thisLine = m_wrapped->GetData(i);

				float fontSize = m_fontSize;
				UnicodeString newString = UnicodeString(thisLine->m_unicodestring);
				if( thisLine->m_unicodestring[0] == L'-' )
				{
					newString = UnicodeString(&(thisLine->m_unicodestring[1]));
					fontSize *= 1.25f;
				}
                
                glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                g_editorFont.SetRenderOutline(true);
				g_editorFont.DrawText2DSimple( x, y, fontSize, newString );

                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                g_editorFont.SetRenderOutline(false);
				g_editorFont.DrawText2DSimple( x, y, fontSize, newString );
                

                y+=gap;
            }
        }
        //glDisable( GL_SCISSOR_TEST );
        g_app->m_renderer->EndClip();
    }
};

#endif

