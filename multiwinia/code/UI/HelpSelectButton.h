#ifndef __HELPSELECTBUTTON__
#define __HELPSELECTBUTTON__


class HelpSelectButton : public GameMenuButton
{
public:
    int m_helpType;   

    HelpSelectButton()
    :   GameMenuButton(""),
        m_helpType(-1)
    {
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();

        GameMenuWindow *main = (GameMenuWindow *)m_parent;
        main->m_helpPage = m_helpType;
        main->m_newPage = GameMenuWindow::PageHelp;
        main->m_currentHelpImage = 1;
    }
};

#endif
