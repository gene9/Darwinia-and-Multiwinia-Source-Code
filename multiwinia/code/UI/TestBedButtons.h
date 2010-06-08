#ifndef _include_testbed_button_h
#define _include_testbed_button_h

#ifdef TESTBED_ENABLED

class StartTestbedButton : public GameMenuButton
{
public:
    StartTestbedButton( TESTBED_TYPE _type )
    :   GameMenuButton(UnicodeString()),
		m_type(_type)
    {
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;

		// Testing out the testbed stuff
		g_app->SetTestBedMode(TESTBED_ON);
		g_app->SetTestBedState(TESTBED_BUILD_GAME_ENTRIES);
		g_app->SetTestBedType(m_type);
		g_app->SetTestBedGameMode(-1);
		g_app->SetTestBedGameMap(-1);
		g_app->ResetGameCount();
		g_app->ResetNumSyncErrors();
		g_app->SetTestBedRenderOff();
		g_app->m_iGameIndex = 4;
		g_app->m_iLastGameIndex = 0;
	}

private:
	TESTBED_TYPE m_type;
};

#endif

#endif