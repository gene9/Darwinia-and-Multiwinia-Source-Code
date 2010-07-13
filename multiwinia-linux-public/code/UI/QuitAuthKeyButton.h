#ifndef __QUITAUTHKEYBUTTON__
#define __QUITAUTHKEYBUTTON__


#if defined(TARGET_PC_HARDWARECOMPAT) || defined(TARGET_BETATEST_GROUP) || defined(TARGET_ASSAULT_STRESSTEST)

class QuitAuthkeyButton : public GameMenuButton
{
public:
    QuitAuthkeyButton()
    :   GameMenuButton( UnicodeString() )
    {
        m_transitionDirection = -1;
    }

    void MouseUp()
    {
        g_app->m_requestQuit = true;
    }
};

#endif // TARGET_PC_HARDWARECOMPAT || TARGET_BETATEST_GROUP

#endif
