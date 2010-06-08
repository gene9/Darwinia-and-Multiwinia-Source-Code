#ifndef __QUICKMATCHBUTTON__
#define __QUICKMATCHBUTTON__

class QuickMatchButton : public GameMenuButton
{
public:
    QuickMatchButton()
    :   GameMenuButton(UnicodeString())
    {
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();

		GameMenuWindow *parent = (GameMenuWindow *)m_parent;
		parent->m_newPage = GameMenuWindow::PageQuickMatchGame;
    }
};

#endif
