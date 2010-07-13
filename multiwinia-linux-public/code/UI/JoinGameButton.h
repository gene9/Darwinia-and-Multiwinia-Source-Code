#ifndef __JOINGAMEBUTTON__
#define __JOINGAMEBUTTON__

class JoinGameButton : public GameMenuButton 
{
public:
	JoinGameButton()
	: GameMenuButton( "Join Game")
	{
	}

    void MouseUp()
    {
        GameMenuButton::MouseUp();
        ((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageJoinGame;
    }
};

#endif


