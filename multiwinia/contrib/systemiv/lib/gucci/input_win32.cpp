 #include "lib/universal_include.h"

#include <time.h>

#include "lib/hi_res_time.h"
#include "lib/eclipse/eclipse.h"
#include "lib/debug_utils.h"

#include "input.h"
#include "input_win32.h"
#include "window_manager.h"
#include "window_manager_win32.h"

HWND g_hwnd = NULL;

// *** Constructor
InputManagerWin32::InputManagerWin32()
:   InputManager()
{
	g_hwnd = GetWindowManagerWin32()->m_hWnd;
}


void InputManagerWin32::ResetWindowHandle()
{
    UnbindAltTab();
    if( m_altTabBound ) BindAltTab();
}


void InputManagerWin32::BindAltTab()
{
	AppAssert(GetWindowManagerWin32()->m_hWnd);
	RegisterHotKey(GetWindowManagerWin32()->m_hWnd, 0, MOD_ALT, VK_TAB);
    m_altTabBound = true;
}


void InputManagerWin32::UnbindAltTab()
{
	AppAssert(GetWindowManagerWin32()->m_hWnd);
	UnregisterHotKey(g_hwnd, 0);
	UnregisterHotKey(GetWindowManagerWin32()->m_hWnd, 0);
}


void InputManagerWin32::WaitEvent()
{
	// Wait for some kind of event.
    // TODO
}


// Returns 0 if the event is handled here, -1 otherwise
int InputManagerWin32::EventHandler(unsigned int message, unsigned int wParam, int lParam, bool _isAReplayedEvent)
{
	bool logEvent = false;

	switch (message) 
	{
		case WM_SIZING:
		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
		case WM_DESTROY:
		case WM_CREATE:
		case WM_COMMAND:
		case WM_SETCURSOR:		// Cursor move
		case WM_NCHITTEST:		// Mouse move or button
        case WM_CLOSE:
			return -1;
	
		case WM_CANCELMODE:
			return 0;

		case WM_SETFOCUS:
			m_windowHasFocus = true;
			if( m_altTabBound ) BindAltTab();
			return 0;

		case WM_KILLFOCUS:
			m_windowHasFocus = false;
			UnbindAltTab();
			return 0;

        case WM_LBUTTONDOWN:
			m_lmb = true;
			g_windowManager->CaptureMouse();
			logEvent = true;
			break;
		case WM_LBUTTONUP:
			m_lmb = false;
			g_windowManager->UncaptureMouse();
			logEvent = true;
			break;
		case WM_MBUTTONDOWN:
			m_mmb = true;
			g_windowManager->CaptureMouse();
			logEvent = true;
			break;
		case WM_MBUTTONUP:
			m_mmb = false;
			g_windowManager->UncaptureMouse();
			logEvent = true;
			break;
		case WM_RBUTTONDOWN:
			m_rmb = true;
			g_windowManager->CaptureMouse();
			logEvent = true;
			break;
		case WM_RBUTTONUP:
			m_rmb = false;
			g_windowManager->UncaptureMouse();
			logEvent = true;
			break;

		case 0x020A:
//		case WM_MOUSEWHEEL:
		{
			int move = (short)HIWORD(wParam) / 120;
			m_mouseZ += move;
			logEvent = true;
			break;
		}
        
		case WM_MOUSEMOVE:
		{
			short newPosX = lParam & 0xFFFF;
			short newPosY = short(lParam >> 16);
			m_mouseX = newPosX;
			m_mouseY = newPosY;
			logEvent = true;
			break;
		}

		case WM_HOTKEY:
			if (wParam == 0)
			{
				g_keys[KEY_TAB] = 1;
				m_keyNewDeltas[KEY_TAB] = 1;
			}
			break;
			
		case WM_SYSKEYUP:
			if (wParam == KEY_ALT)
			{
				// ALT pressed
				g_keys[wParam] = 0;
			}
			else
			{
				g_keys[wParam] = 0;
			}
			logEvent = true;
			break;
			
		case WM_SYSKEYDOWN:
		{
			int flags = (short)HIWORD(lParam);
			if (wParam == KEY_ALT)
			{
				g_keys[wParam] = 1;
			}
			else if (flags & KF_ALTDOWN)
			{
				m_keyNewDeltas[wParam] = 1;
				g_keys[wParam] = 1;
				if (wParam == KEY_F4)
				{
					exit(0);
				}
			}
			logEvent = true;
			break;
		}

		case WM_KEYUP:
		{
			AppAssert(wParam >= 0 && wParam < KEY_MAX);
			if (g_keys[wParam] != 0)
			{
//				m_keyNewDeltas[wParam] = -1;
				g_keys[wParam] = 0;
			}
			logEvent = true;
			break;
		}

		case WM_KEYDOWN:
		{
			AppAssert(wParam >= 0 && wParam < KEY_MAX);

			if (g_keys[wParam] != 1)
			{
				m_keyNewDeltas[wParam] = 1;
				g_keys[wParam] = 1;
			}                
            
            unsigned char keys[256];
            memset( keys, 0, 256*sizeof(unsigned char) );
            for( int i = 0; i < 256; ++i )
                if( g_keys[i] ) keys[i] = 0xff;
            
            unsigned short ascii;
            ToAscii( wParam, wParam, keys, &ascii, 0 );

			EclUpdateKeyboard( wParam, false, false, false, (unsigned char) ascii );
			logEvent = true;
			break;
		}

		default:
			return -1;		
	}
	
	// Run this for every event we get, to handle the case of multiple mouse events in a
	// single update cycle.
	EclUpdateMouse( g_inputManager->m_mouseX, 
					g_inputManager->m_mouseY, 
					g_inputManager->m_lmb, 
					g_inputManager->m_rmb );

	return 0;
}