#ifndef __RESEARCHMODEBUTTON__
#define __RESEARCHMODEBUTTON__

class ResearchModeButton : public GameMenuButton
{
public:
    ResearchModeButton()
    : GameMenuButton( "Research" )
    {
    }

    void MouseUp()
    {
        ((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageResearch;
    }
};

#endif
