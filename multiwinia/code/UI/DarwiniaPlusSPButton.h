#ifndef __DARWINIAPLUSSPBUTTON__
#define __DARWINIAPLUSSPBUTTON__

class DarwiniaPlusSPButton : public GameMenuButton
{
public:
    DarwiniaPlusSPButton( char *_iconName )
    :   GameMenuButton( _iconName )
    {
    }

	void MouseUp()
	{
		GameMenuButton::MouseUp();
		GameMenuWindow *p = (GameMenuWindow *) m_parent;

		p->m_singlePlayer = true;
    }
};

#endif
