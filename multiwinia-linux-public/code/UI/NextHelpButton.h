#ifndef __NEXTHELPBUTTON__
#define __NEXTHELPBUTTON__

class NextHelpButton : public GameMenuButton
{
public:

    NextHelpButton()
    :   GameMenuButton("")
    {
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();

        GameMenuWindow *main = (GameMenuWindow *)m_parent;

        char imageName[512];
        sprintf( imageName, "mwhelp/helppage_%d_%d." TEXTURE_EXTENSION, main->m_helpPage, main->m_currentHelpImage + 1 );
        if( g_app->m_resource->DoesTextureExist( imageName ) )
        {
            main->m_currentHelpImage ++;
        }
        else
        {
            main->m_newPage = GameMenuWindow::PageHelpMenu;
            main->m_currentHelpImage = -1;
            main->m_helpPage = 0;
        }
    }
};

#endif
