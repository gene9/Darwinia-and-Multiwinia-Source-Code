#ifndef __TUTORIALBUTTON__
#define __TUTORIALBUTTON__

#ifdef INCLUDE_TUTORIAL

class TutorialPageButton : public GameMenuButton
{
public:
    TutorialPageButton()
    :   GameMenuButton( UnicodeString() )
    {
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();
        GameMenuWindow *p = (GameMenuWindow *)m_parent;
        p->m_newPage = GameMenuWindow::PageTutorials;
    }
};

class TutorialButton : public GameMenuButton
{
public:
    int m_tutorialType;
    TutorialButton()
    :   GameMenuButton(""),
        m_tutorialType(App::MultiwiniaTutorial1)
    {
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        GameMenuButton::Render( realX, realY, highlighted, clicked );
    }

    void MouseUp()
    {
		if( m_inactive )
		{
			return;
		}

        GameMenuWindow *main = (GameMenuWindow *)m_parent;
        if( main->m_quickStart ) 
		{
			return;
		}

        GameMenuButton::MouseUp();	

        //if( !g_app->m_server )
        {
            // TODO Remove me

            main->m_singlePlayer = true;
            main->m_quickStart = true;
            g_app->m_multiwiniaTutorial = true;
            g_app->m_multiwiniaTutorialType = m_tutorialType;
            main->m_lockMenu = true;

            //g_app->m_renderer->StartFadeOut();
        }        
    }
};
#endif

#endif
