#include "lib/universal_include.h"

#include <iostream>

#include "lib/input/win32_eventhandler.h"

#include "lib/window_manager.h"
#include "lib/window_manager_win32.h"
#include "lib/debug_utils.h"

#include "app.h"

using std::cerr;

typedef std::vector<W32EventProcessor *> ProcList;
typedef ProcList::iterator ProcIt;


bool g_windowHasFocus = true;
bool g_altTabBound = false;
HWND g_hwnd = NULL;


W32EventHandler::W32EventHandler() : w32eventprocs() {}


LRESULT CALLBACK W32EventHandler::WndProc( HWND hWnd, UINT message,
                                           WPARAM wParam, LPARAM lParam )
{
	static bool s_previousFocus = false;
	g_windowHasFocus = GetForegroundWindow() == g_windowManager->m_win32Specific->m_hWnd;

	if (!g_windowHasFocus || !s_previousFocus) {
		// When switching away, ALT is often left down. 
		// Let's clear it.

		extern signed char g_keyDeltas[KEY_MAX];
		extern signed char g_keys[KEY_MAX];

		if (g_keys[KEY_ALT]) { 
			g_keyDeltas[KEY_ALT] = -1;
			g_keys[KEY_ALT] = 0;		
		}
	}

	s_previousFocus = g_windowHasFocus;

	switch (message) 
	{
		case WM_SIZING:
		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
		case WM_DESTROY:
		case WM_CREATE:
		case WM_COMMAND:
		case WM_NCHITTEST:		// Mouse move or button
			return -1;

		case WM_SETCURSOR:
			{
				POINT pt;
				GetCursorPos(&pt);
				LPARAM lparam = (unsigned long) (pt.x & 0xFFFF) | (unsigned long) ((pt.y & 0xFFFF) << 16);
				if (DefWindowProc(hWnd, WM_NCHITTEST, 0, lparam) == HTCLIENT)
					SetCursor(NULL);
				else
					return -1;
			}

		case WM_MOVE:
			g_windowManager->WindowMoved();
			return -1;

		case WM_CANCELMODE:
			return 0;

		case WM_CLOSE:
			g_app->m_requestQuit = true;
			return 0;

		case WM_INPUTLANGCHANGE:
			DebugOut( "Input language change: w = %d, l = %d\n", wParam, lParam );
			// Might want to reload key bindings and translations here if we can be bothered.
			return 0;
	}

	int ans = -1;
	for ( ProcIt i = w32eventprocs.begin(); i != w32eventprocs.end(); ++i ) {
		ans = (*i)->WndProc( hWnd, message, wParam, lParam );
		if ( ans != -1 ) break;
	}
	return ans;
}


void W32EventHandler::AddEventProcessor( W32EventProcessor *_driver )
{
	if ( _driver )
		w32eventprocs.push_back( _driver );
	else
		cerr << "AddEventProcessor: _driver is NULL\n";
	g_hwnd = g_windowManager->m_win32Specific->m_hWnd;
}


void W32EventHandler::RemoveEventProcessor( W32EventProcessor *_driver )
{
	for ( ProcIt i = w32eventprocs.begin(); i != w32eventprocs.end(); ++i )
		if ( _driver == *i ) w32eventprocs.erase( i );
}


void W32EventHandler::ResetWindowHandle()
{
    UnbindAltTab();
    if( g_altTabBound ) BindAltTab();
}


void W32EventHandler::BindAltTab()
{
	DarwiniaDebugAssert(g_windowManager->m_win32Specific->m_hWnd);
	RegisterHotKey(g_windowManager->m_win32Specific->m_hWnd, 0, MOD_ALT, VK_TAB);
    g_altTabBound = true;
}


void W32EventHandler::UnbindAltTab()
{
	DarwiniaDebugAssert(g_windowManager->m_win32Specific->m_hWnd);
	UnregisterHotKey(g_hwnd, 0);
	UnregisterHotKey(g_windowManager->m_win32Specific->m_hWnd, 0);
}


bool W32EventHandler::WindowHasFocus() { return g_windowHasFocus; }


W32EventHandler *getW32EventHandler()
{
	EventHandler *handler = g_eventHandler;
	return dynamic_cast<W32EventHandler *>( handler );
}
