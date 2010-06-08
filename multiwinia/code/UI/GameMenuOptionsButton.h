#ifndef __GAMEMENUOPTIONSBUTTON__
#define __GAMEMENUOPTIONSBUTTON__

class GameMenuOptionsButton : public GameMenuButton
{
public:
    GameMenuOptionsButton()
        :   GameMenuButton("")
    {
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();

        if (!EclGetWindow("dialog_options"))
        {
            EclRegisterWindow( new OptionsMenuWindow(), m_parent );
        }
    }
};

#endif
