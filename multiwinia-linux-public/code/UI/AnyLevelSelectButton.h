#ifndef __ANYLEVELSELECTBUTTON__
#define __ANYLEVELSELECTBUTTON__

class AnyLevelSelectButton : public LevelSelectButtonBase
{
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
		RenderBackground( realX, realY, highlighted, clicked );
		RenderMapName( m_caption, realX, realY, highlighted, clicked, false );
	}

	void MouseUp()
	{
        g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
        GameMenuWindow *main = (GameMenuWindow *) m_parent;

		main->m_customMapId = -1;
		main->m_newPage = GameMenuWindow::PageJoinGame; 
	}
};

#endif
