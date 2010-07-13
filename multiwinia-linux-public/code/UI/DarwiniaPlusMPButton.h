#ifndef __DARWINIAPLUSMPBUTTON__
#define __DARWINIAPLUSMPBUTTON__

class DarwiniaPlusMPButton : public GameMenuButton
{
public:
    DarwiniaPlusMPButton( char *_iconName )
    :   GameMenuButton( _iconName )
    {
    }

	void MouseUp()
	{
#if defined(MULTIPLAYER_DISABLED)
        if( m_inactive ) return;
#endif

        GameMenuButton::MouseUp();

        if( m_inactive )
		{
			((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageBuyMeNow;
		}
        else
		{
			((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageNewOrJoin;
		}

		LockController();
    }
};

#endif
