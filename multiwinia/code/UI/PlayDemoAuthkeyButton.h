#ifndef __PLAYDEMOAUTHKEYBUTTON__
#define __PLAYDEMOAUTHKEYBUTTON__


#if !(defined(TARGET_PC_HARDWARECOMPAT) || defined(TARGET_BETATEST_GROUP) || defined(TARGET_ASSAULT_STRESSTEST))

class PlayDemoAuthkeyButton : public GameMenuButton
{
public:
    PlayDemoAuthkeyButton()
    :   GameMenuButton( UnicodeString() )
    {
        m_transitionDirection = -1;
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;

        Authentication_GenerateKey( parent->m_authKey, 4, true );
        int authStatus = Authentication_SimpleKeyCheck( parent->m_authKey, METASERVER_GAMETYPE_MULTIWINIA );

        if( authStatus == AuthenticationAccepted )
        {
            char fullFileName[512];
            snprintf( fullFileName, sizeof(fullFileName), "%sauthkey", g_app->GetProfileDirectory() );
            fullFileName[sizeof(fullFileName) - 1] = '\0';

            Authentication_SetKey( parent->m_authKey );
            Authentication_SaveKey( parent->m_authKey, fullFileName );

            parent->m_newPage = GameMenuWindow::PageMain;
        }
        else
        {
            memset( parent->m_authKey, 0, sizeof(parent->m_authKey) );
        }
    }
};

#endif // !(TARGET_PC_HARDWARECOMPAT || TARGET_BETATEST_GROUP || TARGET_ASSAULT_STRESSTEST)

#endif
