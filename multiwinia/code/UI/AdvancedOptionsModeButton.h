#ifndef __ADVANCEDOPTIONSMODEBUTTON__
#define __ADVANCEDOPTIONSMODEBUTTON__


class AdvancedOptionsModeButton : public GameMenuButton
{
public:
    AdvancedOptionsModeButton()
    : GameMenuButton( "AdvancedOptions" )
    {
    }

    void MouseUp()
    {
        char authKey[256];
        Authentication_GetKey( authKey );
        bool demoKey = Authentication_IsDemoKey( authKey );

        if( !demoKey )
        {
            GameMenuButton::MouseUp();

            ((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageAdvancedOptions;
        }
    }
};

#endif
