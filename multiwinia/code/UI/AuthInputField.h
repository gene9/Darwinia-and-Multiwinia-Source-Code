#ifndef __AUTHINPUTFIELD__
#define __AUTHINPUTFIELD__

#include "interface/input_field.h"
#include "lib/input/keydefs.h"

class AuthInputField : public InputField
{
public:
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        //
        // Background

        RGBAColour background( 10,10,10,255 );
        RGBAColour borderA( 200,200,200,255);
        RGBAColour borderB(255,255,255,255);

        glLineWidth(1.0f);

        glBegin( GL_QUADS);
            glColor4ubv( background.GetData() );
            glVertex2f( m_x, m_y );
            glVertex2f( m_x + m_w, m_y );
            glVertex2f( m_x + m_w, m_y + m_h );
            glVertex2f( m_x, m_y + m_h );
        glEnd();
        
        glBegin( GL_LINE_LOOP );
            glColor4ubv( borderA.GetData() );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();

        //
        // Caption

        m_value->ToUnicodeString(m_buf);
		m_buf.Truncate( AUTHENTICATION_KEYLEN+1 );
		m_buf.StrUpr();

        float fontSize = m_fontSize;
        //float textWidth = g_renderer->TextWidth(m_buf, fontSize);
        //float fieldX = realX + m_w/2 - textWidth/2;
        //float yPos = realY + m_h/2 - fontSize/2;

        //g_renderer->TextSimple( fieldX, yPos, White, fontSize, m_buf );

        float xPos = realX;
        float yPos = realY + m_fontSize / 2.0f;
        bool authKeyFound = m_buf.StrCmp( UnicodeString("authkey not found") ) != 0;

        for( int i = 0; i < AUTHENTICATION_KEYLEN-1; ++i )
        {
            if( authKeyFound )
            {
                char thisChar = ' ';
                if( i < m_buf.Length() )
                {
					thisChar = m_buf.m_charstring[i];
                }

                glColor4f(255,255,255,255);
                g_titleFont.DrawText2D( xPos+fontSize / 3, yPos, fontSize, "%c", thisChar );
            }

            RGBAColour lineCol(255,255,255,100);
            if( i % 7 >= 5 ) lineCol.a = 255;
            glColor4ubv( lineCol.GetData() );
            glBegin( GL_LINES );
                glVertex2i( xPos+fontSize, realY+1 );
                glVertex2i( xPos+fontSize, realY+m_h-1 );
            glEnd();

            xPos += fontSize;
        }


        //
        // Cursor

        if( m_buf.Length() < AUTHENTICATION_KEYLEN )
        {
            if (m_parent->m_currentTextEdit && 
                strcmp(m_parent->m_currentTextEdit, m_name) == 0 )
            {
                if (fmodf(GetHighResTime(), 1.0f) < 0.5f  )
                {
                    float cursorX = realX + fontSize * 0.1f + m_buf.WcsLen() * fontSize;
                    float cursorY = realY + m_h * 0.1f;
                    float cursorW = fontSize * 0.8f;
                    float cursorH = m_h * 0.8f;
                    glBegin( GL_QUADS);
                        glColor4f( 255, 255, 255, 255 );
                        glVertex2f( cursorX, cursorY );
                        glVertex2f( cursorX + cursorW, cursorY );
                        glVertex2f( cursorX + cursorW, cursorY + cursorH );
                        glVertex2f( cursorX, cursorY + cursorH );
                    glEnd();
                }
            }
        }

        //
        // Auth result

        char authKey[256];
        Authentication_GetKey( authKey );

        if( strlen(authKey) >= AUTHENTICATION_KEYLEN-1 )
        {
            int authResult = Authentication_SimpleKeyCheck( authKey, METASERVER_GAMETYPE_MULTIWINIA );
            bool demoUser = Authentication_IsDemoKey( authKey );

            UnicodeString authString = LANGUAGEPHRASE(Authentication_GetStatusStringLanguagePhrase(authResult));
            if( demoUser ) authString = LANGUAGEPHRASE("dialog_auth_status_demo_user");
			UnicodeString wholeString = LANGUAGEPHRASE("authentication_result");
			wholeString.ReplaceStringFlag( L'S', authString );
            

			glColor4f( 1.0f,1.0f,1.0f,0.0f);
			g_titleFont.SetRenderOutline(true);
			g_titleFont.DrawText2DCentre( realX + m_w/2, realY + m_h * 6.0f, fontSize*1.5f, wholeString );
            glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );           
			g_titleFont.SetRenderOutline(false);
            g_titleFont.DrawText2DCentre( realX + m_w/2, realY + m_h * 6.0f, fontSize*1.5f, wholeString );
            //g_titleFont.DrawText2DCentre( realX + m_w/2, realY + m_h * 3.0f, fontSize*1.5f, authString );
        }
    }

    void Keypress( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii )
    {
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;

        int len = m_buf.Length();
        if( len < AUTHENTICATION_KEYLEN -1 ||
            keyCode == KEY_BACKSPACE )
        {
            InputField::Keypress( keyCode, shift, ctrl, alt, ascii );

			m_buf.StrUpr();
            m_value->FromUnicodeString( m_buf );

			char path[512];
			strcpy( path, g_app->GetProfileDirectory() );
			char fullFileName[512];
			sprintf( fullFileName, "%sauthkey", path );

            Authentication_SetKey( parent->m_authKey );
            Authentication_SaveKey( parent->m_authKey, fullFileName );

            if( keyCode != KEY_BACKSPACE )
            {
                len = m_buf.Length();
                if( len == 6 ||
                    len == 13 ||
                    len == 20 ||
                    len == 27 )
                {
                    m_buf = m_buf + UnicodeString("-");
                    m_value->FromUnicodeString( m_buf );
                }
            }
        }


        if (keyCode == KEY_ENTER)
        {
			if( parent->CheckNewAuthKeyStatus() )
			{
                parent->m_newPage = GameMenuWindow::PageMain;
			}
        }


#ifdef TARGET_OS_MACOSX
        // CMD-V paste support
        if( keyCode == KEY_V && ctrl )
        {
            parent->PasteFromClipboard();            
        }
#endif
    }

    void MouseUp()
    {     
        if( strcmp( ((LocalString *)m_value)->Get(), "authkey not found" ) == 0 )
        {
            ((LocalString *)m_value)->Set('\x0');
        }

        InputField::MouseUp();
    }
};

#endif

