#ifndef __GAMETYPEANYBUTTON__
#define __GAMETYPEANYBUTTON__

class GameTypeAnyButton : public GameMenuButton
{
public:

	GameTypeAnyButton( const UnicodeString &_iconName )
    :   GameMenuButton( _iconName )        
    {
    }

	void MouseUp()
	{
		GameMenuButton::MouseUp();
		
		GameMenuWindow *main = (GameMenuWindow *) m_parent;
		main->m_customGameType = -1;
		main->m_newPage = GameMenuWindow::PageJoinGame;
	}
};

#endif
