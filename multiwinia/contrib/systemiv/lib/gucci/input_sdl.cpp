#include "lib/universal_include.h"

#include <time.h>
#include <SDL/SDL.h>
#include <eclipse.h>
#include <string.h>

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/math/math_utils.h"
#include "lib/preferences.h"
#include "lib/resource/resource.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/language_table.h"
#include "input_sdl.h"
#include "window_manager.h"

// Uncomment for lots of output
// #define VERBOSE_DEBUG

double g_keyDownTime[KEY_MAX];

// *** Constructor
InputManagerSDL::InputManagerSDL()
:	InputManager(),
	m_rawRmb(false),
	m_emulatedRmb(false)
#ifdef TARGET_OS_LINUX
	,m_xineramaOffsetHack( g_preferences->GetInt("XineramaHack", 1) )
#endif // TARGET_OS_LINUX
{
	// Don't let SDL do mouse button emulation for us; we'll do it ourselves if
	// we want it.
	putenv("SDL_HAS3BUTTONMOUSE=1");
	m_shouldEmulateRmb = g_preferences->GetInt("UseOneButtonMouseModifiers", 0);

	SDL_EnableUNICODE(true);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, 90);
}


static int ConvertSDLKeyIdToWin32KeyId(SDLKey _keyCode)
{
	int keyCode = (int) _keyCode;

	// Specific mappings
	switch (keyCode) {
		case SDLK_COMMA: return KEY_COMMA;
		case SDLK_PERIOD: return KEY_STOP;
		case SDLK_SLASH: return KEY_SLASH;
		case SDLK_QUOTE: return KEY_QUOTE;
		case SDLK_LEFTBRACKET: return KEY_OPENBRACE;
		case SDLK_RIGHTBRACKET: return KEY_CLOSEBRACE;
		case SDLK_MINUS: return KEY_MINUS;
		case SDLK_EQUALS: return KEY_EQUALS;
		case SDLK_SEMICOLON: return KEY_COLON;
		case SDLK_LEFT: return KEY_LEFT;
		case SDLK_RIGHT: return KEY_RIGHT;
		case SDLK_UP: return KEY_UP;
		case SDLK_DOWN: return KEY_DOWN;
		case SDLK_DELETE: return KEY_DEL;
		case SDLK_CAPSLOCK: return KEY_CAPSLOCK;
		case SDLK_BACKSLASH: return KEY_BACKSLASH;
		case SDLK_BACKQUOTE: return KEY_TILDE;
		case SDLK_KP0: return KEY_0_PAD;		
		case SDLK_KP1: return KEY_1_PAD;		
		case SDLK_KP2: return KEY_2_PAD;		
		case SDLK_KP3: return KEY_3_PAD;		
		case SDLK_KP4: return KEY_4_PAD;		
		case SDLK_KP5: return KEY_5_PAD;		
		case SDLK_KP6: return KEY_6_PAD;		
		case SDLK_KP7: return KEY_7_PAD;		
		case SDLK_KP8: return KEY_8_PAD;		
		case SDLK_KP9: return KEY_9_PAD;		
		case SDLK_KP_PERIOD: return KEY_DEL_PAD;		
		case SDLK_KP_DIVIDE: return KEY_SLASH_PAD;	
		case SDLK_KP_MULTIPLY: return KEY_ASTERISK;	
		case SDLK_KP_MINUS:	return KEY_MINUS_PAD;	
		case SDLK_KP_PLUS: return KEY_PLUS_PAD;		
		case SDLK_KP_ENTER:	return KEY_ENTER;
		case SDLK_KP_EQUALS: return KEY_EQUALS;
	}

	if (keyCode >= 97 && keyCode <= 122)		keyCode -= 32;	// a-z
	else if (keyCode >= 282 && keyCode <= 293)	keyCode -= 170; // Function keys
	else if (keyCode >= 8 && keyCode <= 57)		keyCode -= 0;	// 0-9 + BACKSPACE + TAB
	else if (keyCode >= 303 && keyCode <= 304)	keyCode = KEY_SHIFT;
	else if (keyCode >= 305 && keyCode <= 306)	keyCode = KEY_CONTROL;
	else if (keyCode >= 307 && keyCode <= 308)	keyCode = KEY_ALT;
	else if (keyCode >= 309 && keyCode <= 310)	keyCode = KEY_META;
	else keyCode = 0;

	return keyCode;
}

static inline int SignOf(int x) 
{
	if (x >= 0)
		return 1;
	else
		return 0;
}


void InputManagerSDL::ResetWindowHandle()
{
}

// Temporarily disabled until I find where m_renderer has run off to in Defcon. -- Steven
#if defined TARGET_OS_LINUX && 0
static void AdjustForBuggyXinerama(int &_xrel, int &_yrel)
{
	/* Linux has buggy multiple screen support which causes the
	   mouse events to be all screwed up */

	// Xinerama offset
	static int offsetX = 0, offsetY = 0;
	static int screenW = 0, screenH = 0;
	static int frameCount = 0;

	if (screenW != g_app->m_renderer->ScreenW() ||
		screenH != g_app->m_renderer->ScreenH()) {
		// Resolution changed

		screenW = g_app->m_renderer->ScreenW();
		screenH = g_app->m_renderer->ScreenH();

		// We want to skip the first few mouse events
		// As we can get some funny behaviour after a resolution change
		frameCount = 3;
	}
	
	if (frameCount >= 0)
		frameCount--;
	
	if (frameCount == 0) {
		// Guess Xinerama offset
		int hypot = sqrt(_xrel*_xrel + _yrel*_yrel);
		if (hypot >= 550) {
			offsetX = (_xrel + SignOf(_xrel)*32) / 64 * 64;
			offsetY = (_yrel + SignOf(_yrel)*32) / 64 * 64;
		}
		else {
			offsetX = 0;
			offsetY = 0;
		}
		AppDebugOut("XINERAMA offset guess: %d, %d\n", offsetX, offsetY);
	}

	_xrel -= offsetX;
	_yrel -= offsetY;
}
#endif // TARGET_OS_LINUX

static void BoundMouseXY(int &_x, int &_y)
{
	/* Keep the mouse inside the window */

	int screenW = g_windowManager->WindowW();
	int screenH = g_windowManager->WindowH();

	if (_x < 0) _x = 0;
	if (_y < 0) _y = 0;
	if (_x >= screenW) _x = screenW - 1;
	if (_y >= screenH) _y = screenH - 1;
}

// Returns 0 if the event is handled here, -1 otherwise
int InputManagerSDL::EventHandler(unsigned int message, unsigned int wParam, int lParam, bool _isAReplayedEvent)
{
	SDL_Event *sdlEvent = (SDL_Event*)wParam;

	switch (message)
	{
		case SDL_MOUSEBUTTONUP:
		{
			if (sdlEvent->button.button == SDL_BUTTON_LEFT)	
			{
				m_lmb = false;
				m_emulatedRmb = false;
			}
			else if (sdlEvent->button.button == SDL_BUTTON_MIDDLE)	
				m_mmb = false;
			else if (sdlEvent->button.button == SDL_BUTTON_RIGHT)
				m_rawRmb = false;
				
			m_rmb = m_shouldEmulateRmb ? (m_rawRmb || m_emulatedRmb) : m_rawRmb;
			break;
		}

		case SDL_MOUSEBUTTONDOWN:
		{
			if (sdlEvent->button.button == SDL_BUTTON_LEFT)
			{
				if (g_keys[KEY_CONTROL])
					m_emulatedRmb = true;
				else
					m_lmb = true;
			}
			else if (sdlEvent->button.button == SDL_BUTTON_MIDDLE)	
				m_mmb = true;
			else if (sdlEvent->button.button == SDL_BUTTON_RIGHT)	
				m_rawRmb = true;
			else if (sdlEvent->button.button == SDL_BUTTON_WHEELUP) 
				m_mouseZ += 3;
			else if (sdlEvent->button.button == SDL_BUTTON_WHEELDOWN) 
				m_mouseZ -= 3;
				
			m_rmb = m_shouldEmulateRmb ? (m_rawRmb || m_emulatedRmb) : m_rawRmb;
			break;
		}

		case SDL_MOUSEMOTION:
		{		
			if (g_windowManager->Captured()) {

				int xrel = sdlEvent->motion.xrel;
				int yrel = sdlEvent->motion.yrel;

// Temporarily disabled until I find where m_renderer is in Defcon. -- Steven
#if defined TARGET_OS_LINUX && 0
				if (m_xineramaOffsetHack) 
					AdjustForBuggyXinerama(xrel, yrel);
#endif // TARGET_OS_LINUX

				m_mouseX += xrel;
				m_mouseY += yrel;

#ifdef VERBOSE_DEBUG				
				AppDebugOut("Mouse captured (%d,%d) - Rel (%d, %d) - Adjusted (%d, %d)\n", 
					        sdlEvent->motion.x, sdlEvent->motion.y,
					        sdlEvent->motion.xrel, sdlEvent->motion.yrel,
					        xrel, yrel);
#endif // VERBOSE_DEBUG
					   
				BoundMouseXY(m_mouseX, m_mouseY);
			}
			else {
#ifdef VERBOSE_DEBUG				
				AppDebugOut("Mouse uncaptured (%d,%d) - Rel (%d, %d)\n", 
					        sdlEvent->motion.x, sdlEvent->motion.y,
					        sdlEvent->motion.xrel, sdlEvent->motion.yrel);
#endif // VERBOSE_DEBUG
				m_mouseX = sdlEvent->motion.x;
				m_mouseY = sdlEvent->motion.y;
			}
			break;
		}

		case SDL_KEYUP:
		{
			int keyCode = ConvertSDLKeyIdToWin32KeyId(sdlEvent->key.keysym.sym);
			AppDebugAssert(keyCode >= 0 && keyCode < KEY_MAX);
			if (g_keys[keyCode] != 0)
			{
				m_numKeysPressed--;
				g_keys[keyCode] = 0;
			}
			
			break;
		}

		case SDL_KEYDOWN:
		{
			int keyCode = ConvertSDLKeyIdToWin32KeyId(sdlEvent->key.keysym.sym);
			AppDebugAssert(keyCode >= 0 && keyCode < KEY_MAX);
			if (g_keys[keyCode] != 1)
			{
				m_keyNewDeltas[keyCode] = 1;
				m_numKeysPressed++;
				g_keys[keyCode] = 1;
				g_keyDownTime[keyCode] = GetHighResTime();
			}
			
			EclUpdateKeyboard( keyCode, false, false, false, sdlEvent->key.keysym.unicode );

			break;
		}
		
		case SDL_ACTIVEEVENT:
		{
			if (sdlEvent->active.state & SDL_APPINPUTFOCUS)
				m_windowHasFocus = sdlEvent->active.gain;
		}
		break;
		
		case SDL_QUIT:
			m_quitRequested = true;
			break;
					
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

void InputManagerSDL::WaitEvent()
{
	// Wait for some kind of event.
	SDL_Event event;
	SDL_WaitEvent(&event);
	SDL_PushEvent(&event);
}


// Alt Tab issues are Windows Specific
void InputManagerSDL::BindAltTab()
{
    m_altTabBound = true;
}


void InputManagerSDL::UnbindAltTab()
{
}
