#ifndef __NEWSERVERBUTTON__
#define __NEWSERVERBUTTON__

class NewServerButton : public GameMenuButton 
{
public:
	NewServerButton()
	: GameMenuButton( "New Server")
	{
	}

    void MouseUp()
    {
        GameMenuButton::MouseUp();
		((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageGameSelect;
    }
};

#endif
