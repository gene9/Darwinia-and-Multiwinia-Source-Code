#ifndef __FILTERSPAGEBUTTON__
#define __FILTERSPAGEBUTTON__

class FiltersPageButton : public GameMenuButton
{
public:
    FiltersPageButton()
    :   GameMenuButton(UnicodeString())
    {
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();
        ((GameMenuWindow *)m_parent)->m_newPage = GameMenuWindow::PageSearchFilters;
    }
};

#endif
