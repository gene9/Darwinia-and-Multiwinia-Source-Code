#ifndef _include_credits_button_h
#define _include_credits_button_h

class CreditsButton : public GameMenuButton
{
public:
    CreditsButton()
    :   GameMenuButton(UnicodeString())
    {
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;
        parent->m_newPage = GameMenuWindow::PageCredits;
    }
};

#endif