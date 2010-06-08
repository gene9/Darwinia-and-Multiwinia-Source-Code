#ifndef __AUTHPAGEBUTTON__
#define __AUTHPAGEBUTTON__

#include "UI/GameMenuWindow.h"

class AuthPageButton : public GameMenuButton
{
public:
    AuthPageButton()
    :   GameMenuButton( UnicodeString() )
    {}

    void MouseUp()
    {
        GameMenuButton::MouseUp();
        GameMenuWindow *gmw = (GameMenuWindow *)EclGetWindow( "multiwinia_mainmenu_title" );
        if( gmw )
        {
            gmw->m_newPage = GameMenuWindow::PageAuthKeyEntry;
        }
        EclRemoveWindow( m_parent->m_name );
    }
};

#endif
