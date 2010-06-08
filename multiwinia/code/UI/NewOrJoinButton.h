#ifndef __NEWORJOINBUTTON__
#define __NEWORJOINBUTTON__

class NewOrJoinButton : public GameMenuButton
{
public:
	NewOrJoinButton( char *_iconName )
    :   GameMenuButton( _iconName )
    {
    }

    void MouseUp()
    {
#if defined(MULTIPLAYER_DISABLED)
        if( m_inactive ) return;
#endif

        GameMenuButton::MouseUp();

        if( m_inactive ) ((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageBuyMeNow;
        else ((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageNewOrJoin;

		LockController();
    }
};

#endif
