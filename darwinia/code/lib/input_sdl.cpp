#include "lib/universal_include.h"

#include <time.h>
#include <SDL.h>
#include <eclipse.h>
#include <string.h>

#include "lib/avi_generator.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/input.h"
#include "lib/math_utils.h"
#include "lib/preferences.h"
#include "lib/preference_names.h"
#include "lib/window_manager.h"
#include "lib/resource.h"
#include "lib/text_stream_readers.h"

#include "interface/prefs_screen_window.h"
#include "interface/debugmenu.h"

#include "app.h"
#include "camera.h"
#include "renderer.h"

InputManager *g_inputManager = NULL;
signed char g_keyDeltas[KEY_MAX];
signed char g_keys[KEY_MAX];
double g_keyDownTime[KEY_MAX];


// *** Constructor
InputManager::InputManager()
:   m_cameraLog(NULL),
	m_numCameraItems(0),
	m_maxCameraItems(0),
	m_inputLog(NULL),
	m_numEvents(0),
	m_maxNumEvents(0),
	m_eventNumber(0),
	m_frameNumber(0),
	m_replaying(false),
	m_logging(false),
	m_windowHasFocus(true),
    m_lmb(false),
    m_mmb(false),
    m_rmb(false),
    m_lmbOld(false),
    m_mmbOld(false),
    m_rmbOld(false),
	m_rawLmbClicked(false),
	m_xineramaOffsetHack(false),
    m_lmbClicked(false),
    m_mmbClicked(false),
    m_rmbClicked(false),
    m_lmbUnClicked(false),
    m_mmbUnClicked(false),
    m_rmbUnClicked(false),
    m_mouseX(0),
    m_mouseY(0),
    m_mouseZ(0),
    m_mouseOldX(0),
    m_mouseOldY(0),
    m_mouseOldZ(0),
	m_numKeysPressed(0),
    m_mouseVelX(0),
    m_mouseVelY(0),
    m_mouseVelZ(0)
{
	memset(g_keys, 0, KEY_MAX);
	memset(g_keyDeltas, 0, KEY_MAX);
	memset(m_keyNewDeltas, 0, KEY_MAX);

#ifdef TARGET_OS_LINUX
	m_xineramaOffsetHack = g_prefsManager->GetInt("XineramaHack", 1);
#endif // TARGET_OS_LINUX
}


int ConvertSDLKeyIdToWin32KeyId(SDLKey _keyCode)
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
	}

	if (keyCode >= 97 && keyCode <= 122)		keyCode -= 32;	// a-z
	else if (keyCode >= 282 && keyCode <= 293)	keyCode -= 170; // Function keys
	else if (keyCode >= 8 && keyCode <= 57)		keyCode -= 0;	// 0-9 + BACKSPACE + TAB
	else if (keyCode >= 303 && keyCode <= 304)	keyCode = 16;
	else if (keyCode >= 305 && keyCode <= 306)	keyCode = 17;
	else if (keyCode >= 307 && keyCode <= 308)	keyCode = 18;
	else if (keyCode >= 309 && keyCode <= 310)	keyCode = 224;
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


void InputManager::ResetWindowHandle()
{
}


static void AdjustForBuggyXinerama(int &_xrel, int &_yrel)
{
	/* Linux has buggy multiple screen support which causes the
	   mouse events to be all screwed up */

#ifdef TARGET_OS_LINUX
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
		printf("XINERAMA offset guess: %d, %d\n", offsetX, offsetY);
	}

	_xrel -= offsetX;
	_yrel -= offsetY;
#endif // TARGET_OS_LINUX
}

static void BoundMouseXY(int &_x, int &_y)
{
	/* Keep the mouse inside the window */

	int screenW = g_app->m_renderer->ScreenW();
	int screenH = g_app->m_renderer->ScreenH();

	if (_x < 0) _x = 0;
	if (_y < 0) _y = 0;
	if (_x >= screenW) _x = screenW - 1;
	if (_y >= screenH) _y = screenH - 1;
}


static bool s_switchingFromFullscreenMode = false;

static void SetMouseVisible(bool _visible)
{
	if (_visible) {
		g_windowManager->EnsureMouseUncaptured();
		SDL_ShowCursor(SDL_ENABLE);
	}
	else 
		SDL_ShowCursor(g_windowManager->MouseVisible() ? SDL_ENABLE : SDL_DISABLE);
}

// Returns 0 if the event is handled here, -1 otherwise
int InputManager::EventHandler(unsigned int message, unsigned int wParam, int lParam, bool _isAReplayedEvent)
{
	SDL_Event *sdlEvent = (SDL_Event*)wParam;

	switch (message)
	{
		case SDL_MOUSEBUTTONUP:
		{
			if (sdlEvent->button.button == SDL_BUTTON_LEFT)	
				m_lmb = false;
			else if (sdlEvent->button.button == SDL_BUTTON_MIDDLE)	
				m_mmb = false;
			else if (sdlEvent->button.button == SDL_BUTTON_RIGHT)	
				m_rmb = false;
			break;
		}

		case SDL_MOUSEBUTTONDOWN:
		{
			if (sdlEvent->button.button == SDL_BUTTON_LEFT)	
				m_lmb = true;
			else if (sdlEvent->button.button == SDL_BUTTON_MIDDLE)	
				m_mmb = true;
			else if (sdlEvent->button.button == SDL_BUTTON_RIGHT)	
				m_rmb = true;
			else if (sdlEvent->button.button == SDL_BUTTON_WHEELUP) 
				m_mouseZ += 3;
			else if (sdlEvent->button.button == SDL_BUTTON_WHEELDOWN) 
				m_mouseZ -= 3;
			break;
		}

		case SDL_MOUSEMOTION:
		{		
			if (g_windowManager->Captured()) {

				int xrel = sdlEvent->motion.xrel;
				int yrel = sdlEvent->motion.yrel;

				if (m_xineramaOffsetHack) 
					AdjustForBuggyXinerama(xrel, yrel);

				m_mouseX += xrel;
				m_mouseY += yrel;

#ifdef _DEBUG				
				printf("Mouse (%d,%d) - Rel (%d, %d) - Adjusted (%d, %d)\n", 
					   sdlEvent->motion.x, sdlEvent->motion.y,
					   sdlEvent->motion.xrel, sdlEvent->motion.yrel,
					   xrel, yrel);
#endif // _DEBUG
					   
				BoundMouseXY(m_mouseX, m_mouseY);
			}
			else {
				m_mouseX = sdlEvent->motion.x;
				m_mouseY = sdlEvent->motion.y;
			}
			break;
		}

		case SDL_KEYUP:
		{
			int keyCode = ConvertSDLKeyIdToWin32KeyId(sdlEvent->key.keysym.sym);
			DarwiniaDebugAssert(keyCode >= 0 && keyCode < KEY_MAX);
			if (g_keys[keyCode] != 0)
			{
				m_numKeysPressed--;
				g_keys[keyCode] = 0;
			}
			
#ifdef TARGET_OS_MACOSX
			// Caps lock (Key down = Caps Lock is off)
			// Sometimes we get an event for caps lock even though we are not focused
			// (fullscreen or windowed and focused). We must discard it as we will get
			// another one when the app is properly focused.
			bool hasFocus = !g_windowManager->Windowed() || (SDL_GetAppState() & SDL_APPINPUTFOCUS);
			if (keyCode == KEY_CAPSLOCK && hasFocus) 
				ChangeWindowHasFocus(true);
#endif
			break;
		}

		case SDL_KEYDOWN:
		{
			int keyCode = ConvertSDLKeyIdToWin32KeyId(sdlEvent->key.keysym.sym);
			DarwiniaDebugAssert(keyCode >= 0 && keyCode < KEY_MAX);
			if (g_keys[keyCode] != 1)
			{
				m_keyNewDeltas[keyCode] = 1;
				m_numKeysPressed++;
				g_keys[keyCode] = 1;
				g_keyDownTime[keyCode] = GetHighResTime();
			}
			EclUpdateKeyboard( keyCode, false, false, false );

			// Iconify on Ctrl-Z
			if (m_keyNewDeltas[KEY_Z] && g_keys[KEY_CONTROL]) {
				SDL_WM_IconifyWindow();
			}
			
#ifdef TARGET_OS_MACOSX
			// Switch to windowed (temporarily) for Command-TAB
			if (!g_windowManager->Windowed() && keyCode == KEY_TAB && g_keys[KEY_META]) 
					SetWindowed(true, false, s_switchingFromFullscreenMode);

			// Toggle full screen on Command-F
			else if (g_keys[KEY_META] && keyCode == KEY_F)
				DebugKeyBindings::ToggleFullscreenButton();

			// Pause on caps lock down
			else if (keyCode == KEY_CAPSLOCK) 
				ChangeWindowHasFocus(false);
#endif
			break;
		}
		
		case SDL_QUIT:
			exit(0);
			break;
		
		case SDL_ACTIVEEVENT: {
#ifdef _DEBUG
				printf("SDL_ACTIVEEVENT: SDL_APPMOUSEFOCUS:%d SDL_APPINPUTFOCUS:%d SDL_APPACTIVE:%d\n",
					SDL_GetAppState()&SDL_APPMOUSEFOCUS,
					SDL_GetAppState()&SDL_APPINPUTFOCUS,
					SDL_GetAppState()&SDL_APPACTIVE);
#endif

				// Ignore this event if capslock down (Paused)
#ifdef TARGET_OS_MACOSX
				if (g_keys[KEY_CAPSLOCK]) {
					// printf("active event ignored due to capslock\n");
					break;
				}
#endif

				bool newWindowHasFocus = (SDL_GetAppState() & SDL_APPINPUTFOCUS) != 0;
				static unsigned lastEventTime = 0;
				unsigned eventTime = SDL_GetTicks();
				unsigned elapsedTimeSinceLastEvent = eventTime - lastEventTime;

				lastEventTime = eventTime;

				// Ignore duplicate events
				if (elapsedTimeSinceLastEvent < 100)
					break;

#ifdef TARGET_OS_MACOSX
				// There is a bug on the mac, such that when switching to full screen
				// sometimes the application doesn't get input focus.
				if (!g_windowManager->Windowed())
					newWindowHasFocus = true;
#endif
			
				if (!s_switchingFromFullscreenMode) 
				  ChangeWindowHasFocus(newWindowHasFocus);
				s_switchingFromFullscreenMode = false;

				return 0;
			}
			
		default:
			return -1;
	}

	return 0;
}

void InputManager::ChangeWindowHasFocus(bool _newWindowHasFocus)
{
	if (m_windowHasFocus == _newWindowHasFocus)
		return;
		
	m_windowHasFocus = _newWindowHasFocus;
	
	// Pause time when paused.
	if (m_windowHasFocus) 
		SetRealTimeMode();
	else
		SetFakeTimeMode();

	// If we get an activation event in windowed mode
	// check to see if we need to return to full screen mode
	if (m_windowHasFocus && g_windowManager->Windowed() && !g_prefsManager->GetInt( SCREEN_WINDOWED_PREFS_NAME ))
		RestartWindowManagerAndRenderer();
	
	SetMouseVisible(!m_windowHasFocus);
		
	bool audioPlaying = SDL_GetAudioStatus() == SDL_AUDIO_PLAYING;
	
	if (audioPlaying != m_windowHasFocus) 
		SDL_PauseAudio(!m_windowHasFocus);
}

void InputManager::IncreaseCameraLogSize()
{
	int newSize = m_maxCameraItems * 2;
	if (newSize < 1000) newSize = 1000;
	CameraLogItem *newLog = new CameraLogItem[newSize * 2];
	memcpy(newLog, m_cameraLog, m_maxCameraItems * sizeof(CameraLogItem));
	delete [] m_cameraLog;
	m_cameraLog = newLog;
	m_maxCameraItems = newSize;
}


void InputManager::IncreaseLogSize()
{
	int newSize = m_maxNumEvents * 2;
	if (newSize < 1000) newSize = 1000;
	InputLogItem *newLog = new InputLogItem[newSize];
	if (m_inputLog)
	{
		memcpy(newLog, m_inputLog, m_maxNumEvents * sizeof(InputLogItem));
		delete [] m_inputLog;
	}
	m_inputLog = newLog;
	m_maxNumEvents = newSize;
}


// *** Advance
void InputManager::Advance()
{
#ifdef AVI_GENERATOR
	if (m_replaying)
	{
		while (m_inputLog[m_eventNumber].m_frameNumber <= m_frameNumber &&
			   m_eventNumber < m_numEvents)
		{
			EventHandler(m_inputLog[m_eventNumber].m_messageId,
						 m_inputLog[m_eventNumber].m_wParam,
						 m_inputLog[m_eventNumber].m_lParam,
						 true);
			m_eventNumber++;
		}

		g_app->m_camera->SetTarget(m_cameraLog[m_currentCameraItemIdx].m_pos,
								   m_cameraLog[m_currentCameraItemIdx].m_front,
								   m_cameraLog[m_currentCameraItemIdx].m_up);
		g_app->m_camera->CutToTarget();
		m_currentCameraItemIdx++;

		if (m_eventNumber >= m_numEvents)
		{
			StopReplay();
			g_app->m_aviGenerator->ReleaseEngine();
			delete g_app->m_aviGenerator;
			g_app->m_aviGenerator = NULL;
		}
	}

	if (m_logging)
	{
		if (m_numCameraItems == m_maxCameraItems)
		{
			IncreaseCameraLogSize();
		}

		CameraLogItem *item = &m_cameraLog[m_numCameraItems];
		item->m_front = g_app->m_camera->GetFront();
		item->m_up = g_app->m_camera->GetUp();
		item->m_pos = g_app->m_camera->GetPos();
		
		m_numCameraItems++;
	}
#endif // AVI_GENERATOR
	
	memcpy(g_keyDeltas, m_keyNewDeltas, KEY_MAX);
	memset(m_keyNewDeltas, 0, KEY_MAX);

	m_lmbClicked = m_lmb == true && m_lmbOld == false;
	m_mmbClicked = m_mmb == true && m_mmbOld == false;
	m_rmbClicked = m_rmb == true && m_rmbOld == false;
	m_rawLmbClicked = m_lmbClicked;
	m_lmbUnClicked = m_lmb == false && m_lmbOld == true;
	m_mmbUnClicked = m_mmb == false && m_mmbOld == true;
	m_rmbUnClicked = m_rmb == false && m_rmbOld == true;
	m_lmbOld = m_lmb;
	m_mmbOld = m_mmb;
	m_rmbOld = m_rmb;

	m_mouseVelX = m_mouseX - m_mouseOldX;
	m_mouseVelY = m_mouseY - m_mouseOldY;
	m_mouseVelZ = m_mouseZ - m_mouseOldZ;
	m_mouseOldX = m_mouseX;
	m_mouseOldY = m_mouseY;
	m_mouseOldZ = m_mouseZ;

	m_frameNumber++;
}


void InputManager::PollForMessages()
{
	if (!m_replaying)
	{
		SDL_Event sdlEvent;
		bool hadMouseEdge = false, hadKeyboardEdge = false;

		while (SDL_PollEvent(&sdlEvent))
		{
			// We need to make sure that we get all the events for the
			// mouse buttons and keys, so if there are multiple such events
			// defer them until the next cycle.
			bool isKeyboardEdge = 
				sdlEvent.type == SDL_KEYDOWN || 
				sdlEvent.type == SDL_KEYUP;

			if (isKeyboardEdge) {
				if (hadKeyboardEdge) {
					SDL_PushEvent(&sdlEvent);
					break;
				}
				hadKeyboardEdge = true;
			}
			
			bool isMouseEdge = 
				sdlEvent.type == SDL_MOUSEBUTTONDOWN || 
				sdlEvent.type == SDL_MOUSEBUTTONUP;

			if (isMouseEdge) {
				if (hadMouseEdge) {
					SDL_PushEvent(&sdlEvent);
					break;
				}
				hadMouseEdge = true;
			}
		
			EventHandler(sdlEvent.type, (unsigned int)&sdlEvent, 0);
		}
	}
}

void InputManager::WaitEvent()
{
	// Wait for some kind of event.
	SDL_Event event;
	SDL_WaitEvent(&event);
	SDL_PushEvent(&event);
}



void InputManager::SetMousePos(int x, int y)
{
	// NB: Only works if the mouse is captured
	m_mouseX = x;
	m_mouseY = y;
}


void InputManager::StartLogging()
{
	DarwiniaDebugAssert(!m_logging && !m_replaying);

	m_eventNumber = 0;
	m_frameNumber = 0;
	m_logging = true;

	m_initialCamPos = g_app->m_camera->GetPos();
	m_initialCamFront = g_app->m_camera->GetFront();
	m_initialCamUp = g_app->m_camera->GetUp();
}


void InputManager::StopLogging(char const *_filename)
{
	DarwiniaDebugAssert(m_logging && !m_replaying);

	FILE *out = fopen(_filename, "w");
	DarwiniaDebugAssert(out);

	fprintf(out, "CamPos %.4f %.4f %.4f\n", m_initialCamPos.x, m_initialCamPos.y, m_initialCamPos.z);
	fprintf(out, "CamFront %.4f %.4f %.4f\n", m_initialCamFront.x, m_initialCamFront.y, m_initialCamFront.z);
	fprintf(out, "CamUp %.4f %.4f %.4f\n", m_initialCamUp.x, m_initialCamUp.y, m_initialCamUp.z);


	//
	// Write the event log

	// We miss out the last three events because they are the events that click the stop 
	// button on the logging window
	m_numEvents -= 3;

	fprintf(out, "NumEvents %d\n", m_numEvents);

	for (int i = 0; i < m_numEvents; ++i)
	{
		fprintf(out, "%05d %04x %08x", 
				m_inputLog[i].m_frameNumber,
				m_inputLog[i].m_messageId,
				m_inputLog[i].m_lParam);	

		if (m_inputLog[i].m_wParam != 0x0000000)
		{
			fprintf(out, " %08x\n",	m_inputLog[i].m_wParam);
		}
		else
		{
			fprintf(out, "\n");
		}
	}
	

	//
	// Write the camera log

	for (int i = 0; i < m_numCameraItems; ++i)
	{
		CameraLogItem *item = &m_cameraLog[i];
		fprintf(out, "%.3f %.3f %.3f  ", item->m_pos.x, item->m_pos.y, item->m_pos.z);
		fprintf(out, "%.4f %.4f %.4f  ", item->m_front.x, item->m_front.y, item->m_front.z);
		fprintf(out, "%.4f %.4f %.4f\n", item->m_up.x, item->m_up.y, item->m_up.z);
	}

	fclose(out);

	m_eventNumber = 0;
	m_frameNumber = 0;

	delete [] m_inputLog;
	m_inputLog = NULL;
	m_numEvents = 0;
	m_maxNumEvents = 0;
	
	m_logging = false;
}


void InputManager::StartReplay(char const *_filename)
{
	DarwiniaDebugAssert(!m_logging && !m_replaying);

	m_replaying = true;
	m_frameNumber = 0;
	m_eventNumber = 0;

	m_numCameraItems = 0;
	m_currentCameraItemIdx = 0;

	TextFileReader in(_filename);
	DarwiniaDebugAssert(in.IsOpen());

	int totalNumEvents;
	m_numEvents = 0;
	while (in.ReadLine())
	{
		char *c = in.GetNextToken();
		if (!c) continue;
		
		if (stricmp("CamPos", c) == 0)
		{
			m_initialCamPos.x = atof(in.GetNextToken());
			m_initialCamPos.y = atof(in.GetNextToken());
			m_initialCamPos.z = atof(in.GetNextToken());
			continue;
		}
		else if (stricmp("CamFront", c) == 0)
		{
			m_initialCamFront.x = atof(in.GetNextToken());
			m_initialCamFront.y = atof(in.GetNextToken());
			m_initialCamFront.z = atof(in.GetNextToken());
			continue;
		}
		else if (stricmp("CamUp", c) == 0)
		{
			m_initialCamUp.x = atof(in.GetNextToken());
			m_initialCamUp.y = atof(in.GetNextToken());
			m_initialCamUp.z = atof(in.GetNextToken());
			continue;
		}
		else if (stricmp("NumEvents", c) == 0)
		{
			totalNumEvents = atoi(in.GetNextToken());
			continue;
		}

		if (m_numEvents < totalNumEvents)
		{
			// Add this item to the log
			if (m_numEvents >= m_maxNumEvents)
			{
				IncreaseLogSize();
			}

			// Read frame number
			m_inputLog[m_numEvents].m_frameNumber = atoi(c);
			
			// Read message ID
			c = in.GetNextToken();
			m_inputLog[m_numEvents].m_messageId = strtoul(c, NULL, 16); 
			
			// Read lParam
			c = in.GetNextToken();
			m_inputLog[m_numEvents].m_lParam = strtoul(c, NULL, 16); 
			
			// Read wParam
			if (in.TokenAvailable())
			{
				c = in.GetNextToken();
				m_inputLog[m_numEvents].m_wParam = strtoul(c, NULL, 16); 
			}
			else
			{
				m_inputLog[m_numEvents].m_wParam = 0; 
			}

			m_numEvents++;
		}
		else
		{
			if (m_numCameraItems == m_maxCameraItems)
			{
				IncreaseCameraLogSize();
			}

			m_cameraLog[m_numCameraItems].m_pos.x = atof(c);
			m_cameraLog[m_numCameraItems].m_pos.y = atof(in.GetNextToken());
			m_cameraLog[m_numCameraItems].m_pos.z = atof(in.GetNextToken());

			m_cameraLog[m_numCameraItems].m_front.x = atof(in.GetNextToken());
			m_cameraLog[m_numCameraItems].m_front.y = atof(in.GetNextToken());
			m_cameraLog[m_numCameraItems].m_front.z = atof(in.GetNextToken());

			m_cameraLog[m_numCameraItems].m_up.x = atof(in.GetNextToken());
			m_cameraLog[m_numCameraItems].m_up.y = atof(in.GetNextToken());
			m_cameraLog[m_numCameraItems].m_up.z = atof(in.GetNextToken());

			m_numCameraItems++;
		}
	}

	g_app->m_camera->SetTarget(m_initialCamPos, m_initialCamFront, m_initialCamUp);
	g_app->m_camera->CutToTarget();
	g_app->m_camera->RequestMode(Camera::ModeReplay);
	g_app->m_camera->SetDebugMode(Camera::DebugModeNever);

    m_replayStartTime = time(NULL);
}


void InputManager::StopReplay()
{
	DarwiniaDebugAssert(!m_logging && m_replaying);

	delete [] m_inputLog;
	m_inputLog = NULL;
	m_maxNumEvents = 0;

	m_replaying = false;
	m_frameNumber = 0;
	m_eventNumber = 0;

	g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
	g_app->m_camera->SetDebugMode(Camera::DebugModeAuto);
}

float InputManager::ReplayPercentDone()
{
    return( 100.0f * (float) m_eventNumber / (float) m_numEvents );
}

float InputManager::ReplayElapsedTime()
{
    int timeSoFar = time(NULL) - m_replayStartTime;
    return (float) timeSoFar;
}

float InputManager::ReplayTimeRemaining ()
{
    float percentDone = ReplayPercentDone();
    int timeSoFar = time(NULL) - m_replayStartTime;

    float timeForOnePercent = (float) timeSoFar/percentDone;
    float percentRemaining = 100.0f - percentDone;
    return percentRemaining * timeForOnePercent;
}

#ifdef TARGET_OS_MACOSX
#define META_KEY_NAME "COMMAND"
#else
#define META_KEY_NAME "META" 
#endif

static char *s_keyNames[] = 
{
	"unknown 0",
	"unknown 1",
	"unknown 2",
	"unknown 3",
	"unknown 4",
	"unknown 5",
	"unknown 6",
	"unknown 7",
	"BACKSPACE",
	"TAB",
	"unknown 10",
	"unknown 11",
	"unknown 12",
	"ENTER",
	"unknown 14",
	"unknown 15",
	"SHIFT",
	"CONTROL",
	"ALT",
	"PAUSE",
	"CAPSLOCK",
	"unknown 21",
	"unknown 22",
	"unknown 23",
	"unknown 24",
	"unknown 25",
	"unknown 26",
	"ESC",
	"unknown 28",
	"unknown 29",
	"unknown 30",
	"unknown 31",
	"SPACE",
	"PGUP",
	"PGDN",
	"END",
	"HOME",
	"LEFT",
	"UP",
	"RIGHT",
	"DOWN",
	"unknown 41",
	"unknown 42",
	"unknown 43",
	"unknown 44",
	"INSERT",
	"DEL",
	"unknown 47",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"unknown 58",
	"unknown 59",
	"unknown 60",
	"unknown 61",
	"unknown 62",
	"unknown 63",
	"unknown 64",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"unknown 91",
	"unknown 92",
	"unknown 93",
	"unknown 94",
	"unknown 95",
	"0_PAD",
	"1_PAD",
	"2_PAD",
	"3_PAD",
	"4_PAD",
	"5_PAD",
	"6_PAD",
	"7_PAD",
	"8_PAD",
	"9_PAD",
	"ASTERISK",
	"PLUS_PAD",
	"unknown 108",
	"MINUS_PAD",
	"DEL_PAD",
	"SLASH_PAD",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"unknown 124",
	"unknown 125",
	"unknown 126",
	"unknown 127",
	"unknown 128",
	"unknown 129",
	"unknown 130",
	"unknown 131",
	"unknown 132",
	"unknown 133",
	"unknown 134",
	"unknown 135",
	"unknown 136",
	"unknown 137",
	"unknown 138",
	"unknown 139",
	"unknown 140",
	"unknown 141",
	"unknown 142",
	"unknown 143",
	"NUMLOCK",
	"SCRLOCK",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"unknown 146-185",
	"COLON",
	"EQUALS",
	"COMMA",
	"MINUS",
	"STOP",
	"SLASH",
	"QUOTE",	//192
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"unknown 193-218",
	"OPENBRACE", // 219
	"BACKSLASH",
	"CLOSEBRACE",
	"unknown 222",
	"TILDE",
	META_KEY_NAME 
};


char const *InputManager::GetKeyName(int _keyId)
{
	DarwiniaDebugAssert(_keyId >= 0 && _keyId <= KEY_META);
	
	if (_keyId < 0 || _keyId > KEY_META)	
		return "Range";
	
	if (s_keyNames[_keyId][0] == 'u') 
		return s_keyNames[_keyId] + 8;
	return s_keyNames[_keyId];
}


int InputManager::GetKeyId(char const *_keyName)
{
	for (int i = 0; i <= KEY_TILDE; ++i)
	{
		if (stricmp(_keyName, s_keyNames[i]) == 0)
		{
			return i;
		}
	}

	return -1;
}
