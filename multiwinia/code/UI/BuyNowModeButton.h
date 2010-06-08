#ifndef __BUYNOWMODEBUTTON__
#define __BUYNOWMODEBUTTON__

class BuyNowModeButton : public GameMenuButton
{
public:
    BuyNowModeButton()
    :   GameMenuButton("")
    {
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();

        ((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageBuyMeNow;
    }
};

#endif

