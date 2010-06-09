#include "lib/universal_include.h"
#include "app.h"
#include "MainMenuCallbacks.h"
#include "interface/debugmenu.h"

extern "C" void MainMenuQuit()
{
	if (g_app)
		g_app->m_requestQuit = 1;
}

extern "C"
void ToggleMainMenu()
{
	DebugKeyBindings::DebugMenu();
}

extern "C"
void ToggleFrameRate()
{
	DebugKeyBindings::FPSButton();
}

extern "C"
void ToggleProfiler()
{
#ifdef PROFILER_ENABLED
	DebugKeyBindings::ProfileButton();
#endif
}

extern "C"
void ToggleFullscreen()
{
	DebugKeyBindings::ToggleFullscreenButton();
}

