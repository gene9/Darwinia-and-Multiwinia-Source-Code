#ifndef _included_auth_status_button_h
#define _included_auth_status_button_h

#include "lib/metaserver/metaserver.h"

class AuthStatusButton : public GameMenuButton
{
public:
    AuthStatusButton()
    :   GameMenuButton( UnicodeString() )
    {
        m_transitionDirection = -1;
    }


    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        float fontLarge, fontMed, fontSmall;
        GetFontSizes( fontLarge, fontMed, fontSmall );

		GameMenuWindow *parent = (GameMenuWindow *) m_parent;
		if( !parent->ShowAuthBox() )
			return;

        char authKey[256];
        Authentication_GetKey( authKey );
		bool isDemoKey = Authentication_IsDemoKey( authKey );
        int authResult = Authentication_GetStatus( authKey );

        UnicodeString authString;
        if( isDemoKey &&
            authResult >= AuthenticationUnknown )
        {
            authString = LANGUAGEPHRASE("dialog_auth_status_demo_user");
        }
        else
        {
            authString = LANGUAGEPHRASE(Authentication_GetStatusStringLanguagePhrase( authResult ));
        }
        UnicodeString authTitle = LANGUAGEPHRASE("dialog_auth_status");

        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_titleFont.SetRenderOutline(true);
        g_titleFont.DrawText2DCentre( realX + m_w/2, realY, fontSmall, authTitle );
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_titleFont.SetRenderOutline(false);
        g_titleFont.DrawText2DCentre( realX + m_w/2, realY, fontSmall, authTitle );

        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_titleFont.SetRenderOutline(true);
        g_titleFont.DrawText2DCentre( realX + m_w/2, realY + fontSmall * 1.5f, fontMed, authString );
        if( authResult == AuthenticationUnknown )
            glColor4f( 1.0f, 1.0f, 1.0f, 0.66f );
        else
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_titleFont.SetRenderOutline(false);
        g_titleFont.DrawText2DCentre( realX + m_w/2, realY + fontSmall * 1.5f, fontMed, authString );
    }

    void MouseUp()
    {
    }
};

#endif