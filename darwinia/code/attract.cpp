#include "lib/universal_include.h"

#include <eclipse.h>

#include "lib/input/input.h"

#include "app.h"
#include "script.h"
#include "attract.h"
#include "taskmanager_interface.h"

AttractMode::AttractMode()
:	m_running(false)
{
	strcpy(m_userProfile, "none");
}

void AttractMode::Advance()
{
	if( !m_running )
	{
		if( g_inputManager->controlEvent( ControlStartAttractMode ) ) // change to a timer for user idleness
		{
			StartAttractMode();
		}
	}
	else
	{
		if( g_inputManager->controlEvent( ControlStopAttractMode ) ||
			!g_app->m_script->IsRunningScript() ) // change to check for any user activity
		{
			EndAttractMode();
		}
	}
}

void AttractMode::StartAttractMode()
{
	strcpy( m_userProfile, g_app->m_userProfileName );
	g_app->SaveProfile(true, true);
	g_app->SetProfileName( "AttractMode" );
    g_app->m_taskManagerInterface->m_lockTaskManager = true;
	if( g_app->LoadProfile() )
	{
		g_app->m_script->RunScript("attractmode.txt");
		m_running = true;
	}

	LList<EclWindow *> *windows = EclGetWindows();
    while (windows->Size() > 0) {
        EclWindow *w = windows->GetData(0);
        EclRemoveWindow(w->m_name);
    }


}

void AttractMode::EndAttractMode()
{
    g_app->m_script->m_waitForCamera = false;
	g_app->m_script->RunScript("endattract.txt");
	g_app->SetProfileName( m_userProfile );
	g_app->LoadProfile();

    g_app->m_taskManagerInterface->m_lockTaskManager = false;

	m_running = false;
	strcpy(m_userProfile, "none");
}
