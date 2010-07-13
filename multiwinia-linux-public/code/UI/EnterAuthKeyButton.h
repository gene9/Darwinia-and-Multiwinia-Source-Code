#ifndef __ENTERAUTHKEYBUTTON__
#define __ENTERAUTHKEYBUTTON__

class EnterAuthkeyButton : public GameMenuButton
{
public:
    EnterAuthkeyButton()
    :   GameMenuButton( UnicodeString() )
    {
        m_transitionDirection = -1;
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;
        char authKey[256];
        Authentication_GetKey( authKey );
        int authStatus = Authentication_SimpleKeyCheck( authKey, METASERVER_GAMETYPE_MULTIWINIA );

        if( authStatus == AuthenticationAccepted )
        {
            parent->CheckNewAuthKeyStatus();
            parent->m_newPage = GameMenuWindow::PageMain;
        }
    }
};

#endif
