#ifndef __QUITBUTTON__
#define __QUITBUTTON__

class QuitButton : public GameMenuButton
{
public:
    QuitButton()
    :   GameMenuButton( "Quit" )
    {
    }

    void MouseUp()
    {
        g_app->m_requestQuit = true;
    }
};

#endif

