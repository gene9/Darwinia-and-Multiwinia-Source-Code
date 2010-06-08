#ifndef __MULTIWINIAEDITORBUTTON__
#define __MULTIWINIAEDITORBUTTON__

#include "interface/multiwinia_window.h"

class MultiwiniaEditorButton : public GameMenuButton
{
public:
	MultiwiniaEditorButton()
	:	GameMenuButton(UnicodeString())
	{
	}

	void MouseUp()
	{
        if( m_inactive )
		{
            GameMenuButton::MouseUp();
			((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageBuyMeNow;
		}
        else
        {
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
            g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
        
		    EclRegisterWindow( new MultiwiniaWindow() );
            g_app->m_doMenuTransition = true;
        }
	}
};

#endif
