#ifndef __MULTIWINIAMODEBUTTON__
#define __MULTIWINIAMODEBUTTON__

class MultiwiniaModeButton : public GameMenuButton
{
public:
    MultiwiniaModeButton( char *_iconName )
    :   GameMenuButton( _iconName )
    {
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();

        ((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageGameSelect;
		g_app->StartMultiPlayerServer();	// TODO Remove me
        ((GameMenuWindow *) m_parent)->m_singlePlayer = false;
    }
};

#endif
