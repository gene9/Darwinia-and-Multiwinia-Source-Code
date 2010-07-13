#include "lib/universal_include.h"

#include "lib/text_renderer.h"
#include "lib/input/keydefs.h"

#include "interface/chatinput_window.h"

#include "UI/GameMenuWindow.h"

#include "network/clienttoserver.h"

#include "app.h"
#include "global_world.h"
#include "location_input.h"
#include "main.h"
#include "renderer.h"

ChatInputWindow::ChatInputWindow()
:   DarwiniaWindow("location_chat")
{
    if( !g_app->m_atMainMenu )
    {
        float fontSize = g_app->m_renderer->ScreenH() * 0.015f;

        float scale = (float)g_app->m_renderer->ScreenW() / 1280.0f;
        if( g_app->m_renderer->ScreenW() <= 800 )
        {
            scale = (float)g_app->m_renderer->ScreenW() / 1024.0f;
            if( g_app->m_renderer->ScreenW() <= 640 )
            {
                scale = 0.75;
            }
        }

        float x = 10 * scale;
        float y = g_app->m_renderer->ScreenH() - (10 * scale) - (fontSize * 1.5f);

        strcpy( m_currentChatMessage, "none" );
        SetPosition( x, y );
        SetSize( g_app->m_renderer->ScreenW() * 0.25f, fontSize * 1.5f );
    }
}

void ChatInputWindow::CloseChat()
{
    EclRemoveWindow( "location_chat" );
}

bool ChatInputWindow::IsChatVisible()
{
    return EclGetWindow( "location_chat" );
}

void ChatInputWindow::Render(bool _hasFocus)
{
    EclWindow::Render( _hasFocus );
}

void ChatInputWindow::Create()
{
    ChatInputField *chat = new ChatInputField();
    chat->SetShortProperties("chat_input", 0, 0, m_w, m_h, UnicodeString() );
    chat->RegisterString( m_currentChatMessage, 255 );
    RegisterButton(chat);

    if( !g_app->m_atLobby )
    {
        BeginTextEdit("chat_input");
    }
}

void ChatInputField::Render(int realX, int realY, bool highlighted, bool clicked)
{
    glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_BLEND );
    glBegin( GL_QUADS );
        glVertex2i( realX, realY );
        glVertex2i( realX + m_w, realY );
        glVertex2i( realX + m_w, realY + m_h );
        glVertex2i( realX, realY + m_h );
    glEnd();

    glLineWidth(2.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin( GL_LINE_LOOP );
        glVertex2f( realX, realY );
        glVertex2f( realX + m_w, realY );
        glVertex2f( realX + m_w, realY + m_h );
        glVertex2f( realX, realY + m_h );
    glEnd();

    if (!highlighted && fmodf(GetHighResTime(), 1.0f) < 0.5f)
	{
		if( m_value )
			m_value->ToUnicodeString( m_buf );

		//m_inputBoxWidth = strlen(m_buf) * PIXELS_PER_CHAR + 7;
    }

    float fontSize = m_h * 0.75f;

    if( !EclIsTextEditing() )
    {
        m_buf = LANGUAGEPHRASE("multiwinia_presstotalk");
        glColor3f( 0.4f, 0.4f, 0.4f );
        //fontSize *= 0.75f;
    }

	int x = realX + (fontSize / 2.0f);
    int y = (realY + m_h ) - (fontSize / 2.0f ) - ((m_h - fontSize) / 2.0f);

    g_app->m_renderer->Clip( realX, realY, m_w, m_h );
    if( g_editorFont.GetTextWidth(m_buf.WcsLen(), fontSize) > (m_w - fontSize))
    {
        x = realX + m_w - 5;
        g_editorFont.DrawText2DRight( x, y, fontSize , m_buf);
    }
    else
    {   
	    g_editorFont.DrawText2D( x, y, fontSize , m_buf);
        x += g_editorFont.GetTextWidthReal( UnicodeString(m_buf), fontSize );
    }

    glLineWidth( 1.0f );
    if( EclIsTextEditing() && fmodf(GetHighResTime(), 1.0f) < 0.5f )
    {
        glBegin( GL_LINES );
            glVertex2f( x, realY + m_h * 0.1f );
            glVertex2f( x, realY + m_h * 0.9f );
        glEnd();
    }

    g_app->m_renderer->EndClip();
}

void ChatInputWindow::Update()
{
    if( !g_app->m_atLobby )
    {
        if( strcmp( EclGetCurrentFocus(), m_name ) != 0 ) 
        {
            EclRemoveWindow( m_name );
        }
    }
}

void ChatInputField::Keypress(int _keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii)
{
    InputField::Keypress( _keyCode, shift, ctrl, alt, ascii );

    bool doIt = false;
    GameMenuWindow *gmw = (GameMenuWindow *)EclGetWindow("multiwinia_mainmenu_title");
    if( !g_app->m_atMainMenu ) 
    {
        if( g_app->m_locationInput )
        {
            doIt = !g_app->m_locationInput->m_chatToggledThisUpdate;
        }
    }
    else
    {
        if( gmw )
        {
            doIt = !gmw->m_chatToggledThisUpdate;
        }
    }
    if (_keyCode == KEY_ENTER && doIt )
    {
        UnicodeString msg;
        m_value->ToUnicodeString(msg);
        if( msg.Length() > 0 )
        {
            bool rename = false;
            UnicodeString name;
			if( !msg.StrStr( UnicodeString( "//" ) ) )
			{
				if( msg.StrStr( UnicodeString( "/rename " ) ) )
				{
					name = UnicodeString( 16, &msg.m_unicodestring[8], 15 );
					rename = true;
				}
				else if( msg.StrStr( UnicodeString( "/name " ) ) )
				{
					name = UnicodeString( 16, &msg.m_unicodestring[6], 15 );
					rename = true;
				}
			}

            if( rename )
            {
                if( name.WcsLen() > 0 )
                {
                    if( g_app->m_atLobby )
                    {
                        GameMenuWindow *gmw = (GameMenuWindow *)EclGetWindow("multiwinia_mainmenu_title");
                        if( gmw )
                        {
                            gmw->m_teamName.Set(name);
                            gmw->SetTeamNameAndColour();
                        }
                    }
                    else
                    {
                        g_app->m_clientToServer->ReliableSetTeamName( name );
                    }
                }
            }
            else
            {
                //ChatInputWindow *parent = (ChatInputWindow*)m_parent;
                msg.m_unicodestring[msg.Length()] = L'\0';
                g_app->m_clientToServer->SendChatMessage(msg, g_app->m_clientToServer->m_clientId );
            }

			m_value->FromUnicodeString(UnicodeString());
            m_buf = UnicodeString();

			if( g_app->m_atLobby )
			{
				m_parent->BeginTextEdit( m_name );
			}
            
        }
        if( !g_app->m_atLobby )
        {
            EclRemoveWindow( m_parent->m_name );
        }

        if( gmw )
        {
            gmw->m_chatToggledThisUpdate = true;
        }
        else if( g_app->m_locationInput )
        {
            g_app->m_locationInput->m_chatToggledThisUpdate = true;
        }
    }
}

void ChatInputField::MouseUp()
{
    m_buf = UnicodeString();
    InputField::MouseUp();
}