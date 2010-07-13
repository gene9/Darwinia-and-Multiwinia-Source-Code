#ifndef __DARWINIAMODEBUTTON__
#define __DARWINIAMODEBUTTON__

class DarwiniaModeButton : public GameMenuButton
{
public:
    DarwiniaModeButton( char *_iconName )
    :   GameMenuButton( _iconName )
    {
    }

	void MouseUp()
	{
		GameMenuButton::MouseUp();
		GameMenuWindow *p = (GameMenuWindow *) m_parent;

		p->m_singlePlayer = true;

    #ifdef TARGET_DEBUG
		// Let us test this , debug only
		p->m_newPage = GameMenuWindow::PageDemoSinglePlayer;        
    #else
        // Pc users go straight to chosing a game mode
        p->m_newPage = GameMenuWindow::PageGameSelect;
    #endif
    }
};

#endif
