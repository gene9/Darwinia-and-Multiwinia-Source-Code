#include "lib/universal_include.h"
#include "MultiwiniaCocoaBridge.h"
#include "app.h"
#include "multiwinia.h"
#include "server.h"
#include "lib/hi_res_time.h"
#include "lib/window_manager_sdl.h"

int WindowManagerSDLIsFullscreen()
{
	return g_windowManager && !g_windowManager->Windowed();
}


void WindowManagerSDLPreventFullscreenStartup()
{
	static_cast<WindowManagerSDL *>(g_windowManager)->PreventFullscreenStartup();
}


bool WindowManagerSDLGamePaused()
{
	return g_app->m_multiwinia && g_app->m_multiwinia->GameRunning();
}


bool WindowManagerSDLToggleGamePaused()
{
	return g_app->ToggleGamePaused();
}


// In single-player games, don't try to run in fast forward to catch up for the
// time we were asleep.
void WindowManagerSDLAwakeFromSleep()
{
	if (g_app->m_server && g_app->m_server->m_clients.Size() > 0)
		g_app->m_server->SkipToTime( GetHighResTime() );
}