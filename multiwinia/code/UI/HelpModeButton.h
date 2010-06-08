#ifndef __HELPMODEBUTTON__
#define __HELPMODEBUTTON__

class HelpModeButton : public GameMenuButton
{
public:
    HelpModeButton()
    :   GameMenuButton("")
    {
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();

        ((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageHelpMenu;
    }
};

#endif
