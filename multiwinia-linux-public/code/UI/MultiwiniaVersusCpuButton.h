#ifndef __MULTIWINIAVERSUSCPUBUTTON__
#define __MULTIWINIAVERSUSCPUBUTTON__

class MultiwiniaVersusCpuButton : public GameMenuButton
{
public:
	MultiwiniaVersusCpuButton() : GameMenuButton("")
	{

	}

    void MouseUp()
    {
        GameMenuWindow *parent = (GameMenuWindow *) m_parent;
        parent->m_newPage = GameMenuWindow::PageGameSelect;
        g_app->m_renderer->InitialiseMenuTransition(1.0f,1);
    }
};

#endif
