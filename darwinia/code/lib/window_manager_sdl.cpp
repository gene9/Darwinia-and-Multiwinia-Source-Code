#include "lib/universal_include.h"

#include <SDL.h>
#include <SDL_version.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <fenv.h>

#include "lib/debug_utils.h"
#include "lib/input.h"
#include "lib/preferences.h"
#include "lib/window_manager.h"

#include "app.h"
#include "main.h"
#include "renderer.h"

#ifdef TARGET_OS_LINUX
#include "prefix.h"
#endif

#ifdef AMBROSIA_REGISTRATION
#include "lib/ambrosia.h"
#include "file_tool.h"
#include "interface_tool.h"
#include "platform.h"
#endif

WindowManager *g_windowManager = NULL;


// *** Constructor
WindowManager::WindowManager()
	:	m_mousePointerVisible(true), m_invertY(true), m_warpInvertY(false), m_windowed(false), 
		m_mouseCaptured(false)
{
	m_win32Specific = NULL;
	DarwiniaReleaseAssert(SDL_Init(SDL_INIT_VIDEO) == 0, "Couldn't initialise SDL");
	
	SDL_Rect **validModes = SDL_ListModes(NULL, SDL_OPENGL|SDL_FULLSCREEN);

	for (; *validModes; validModes++) {
		SDL_Rect *mode = *validModes;
		Resolution *res = new Resolution(mode->w, mode->h);
		if (GetResolutionId(mode->w, mode->h) == -1)
			m_resolutions.PutData( res );
	}
}

// Returns an index into the list of already registered resolutions
int WindowManager::GetResolutionId(int _width, int _height)
{
        for (int i = 0; i < m_resolutions.Size(); ++i)
        {
                Resolution *res = m_resolutions[i];
                if (res->m_width == _width && res->m_height == _height)
                {
                        return i;
                }
        }

        return -1;
}

Resolution *WindowManager::GetResolution( int _id )
{
    if( m_resolutions.ValidIndex(_id) )
    {
        return m_resolutions[_id];
    }

    return NULL;
}

bool WindowManager::Windowed()
{
    return m_windowed;
}

WindowManager::~WindowManager()
{
	SDL_Quit();
}

bool WindowManager::CreateWin(int &_width, int &_height, bool _windowed, int _colourDepth, int _refreshRate, int _zDepth)
{
    int bpp = 0;
    int flags = 0;

    const SDL_VideoInfo* info = SDL_GetVideoInfo();
	SDL_PixelFormat vfmt = *info->vfmt;
	
    DarwiniaReleaseAssert(info, "SDL_GetVideoInfo failed: %s", SDL_GetError());
	
	m_windowed = _windowed;

	// Set the flags for creating the mode
    flags = SDL_OPENGL; 
	if (!_windowed)
	{
		flags |= SDL_FULLSCREEN;
		m_invertY = false;
		m_warpInvertY = false;
		
		// Look for the best valid video mode
		if (_colourDepth != -1)
			vfmt.BitsPerPixel = _colourDepth;	
		SDL_Rect **validModes = SDL_ListModes(&vfmt, flags);
		SDL_Rect *bestMode = NULL;
		unsigned bestDiagonalDifference = (unsigned) -1;
			
		if (validModes == NULL)
			return false;

		for (; *validModes; validModes++) {
			SDL_Rect *mode = *validModes;
			unsigned diagonalDifference = (_width - mode->w) * (_width - mode->w) + 
										  (_height - mode->h) * (_height - mode->h);
			
			if (bestMode == NULL || diagonalDifference < bestDiagonalDifference) {
				bestMode = mode;
				bestDiagonalDifference = diagonalDifference;
			}
		}
		DarwiniaReleaseAssert(bestMode != NULL, "Failed to find any valid video modes");
	
		m_screenW = bestMode->w;
		m_screenH = bestMode->h;		
		bpp = vfmt.BitsPerPixel;
	}
	else {
#ifdef TARGET_OS_MACOSX
		const SDL_version *linkedVersion = SDL_Linked_Version();
		// SDL Version prior to 1.2.8 had to be inverted in windowed mode
		m_invertY = linkedVersion->major * 1000 + linkedVersion->minor * 100 + linkedVersion->patch <= 1207;
		// SDL Versions <= 1.2.8 still require warp Y coordinate to be inverted
		m_warpInvertY = true;
		printf("%sinverting Y coordinate in windowed mode\n", (m_invertY ? "" : "not "));
		printf("%swarp-inverting Y coordinate in windowed mode\n", (m_warpInvertY ? "" : "not "));
		
#endif
#ifdef TARGET_OS_LINUX
		m_invertY = false;
		m_warpInvertY = false;
#endif
		// Usually any combination is OK for windowed mode.
		m_screenW = _width;
		m_screenH = _height;
		
		// Add it to the list of screen resolutions if need be
		Resolution *found = NULL;
		for (int i = 0; i < m_resolutions.Size(); i++) {
			Resolution *res = m_resolutions.GetData(i);
			if (res->m_width == _width && res->m_height == _height) {
				found = res;
				break;
			}
		}
		
		if (!found) {
			Resolution *res = new Resolution(_width, _height);
			m_resolutions.PutData(res);
		}
		bpp = info->vfmt->BitsPerPixel;
	}	
	
	switch (bpp) {	
		case 24:
		case 32:
			SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
			break;
	
		case 16:
		default:
			SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
		break;
	}
	
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	bool setVideoMode = false;
	
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, _zDepth );
	
	if (!_windowed && SDL_VideoModeOK(m_screenW, m_screenH, _colourDepth, flags))
		bpp = _colourDepth;
	
	setVideoMode = SDL_SetVideoMode(m_screenW, m_screenH, bpp, flags);
	
	// Fall back to a 16 bit Z-Buffer
	if (!setVideoMode) {
		_zDepth = 16;
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, _zDepth );
		setVideoMode = SDL_SetVideoMode(m_screenW, m_screenH, bpp, flags);
	}
	
	if (!setVideoMode)
		return false;
		
	// Pass back the actual values to the Renderer
	_width = m_screenW;
	_height = m_screenH;

	// Hide the mouse pointer again
	if (!m_mousePointerVisible)
		HideMousePointer();

	if (m_mouseCaptured)
		CaptureMouse();
		
	return true;
}

void WindowManager::DestroyWin()
{
	// Apparently no action required here
}

void WindowManager::Flip()
{
	SDL_GL_SwapBuffers();
}


void WindowManager::NastyPollForMessages()
{
}

extern void DiscardExtraneousMouseMotionEvents();

void WindowManager::NastySetMousePos(int x, int y)
{
}

bool WindowManager::Captured()
{
	return m_mouseCaptured;
}

bool WindowManager::MouseVisible()
{
	return m_mousePointerVisible;
}

void WindowManager::CaptureMouse()
{
	// Don't grab if we don't have focus
	if (!g_inputManager->m_windowHasFocus)
		return;
		
	SDL_WM_GrabInput(SDL_GRAB_ON);
	m_mouseCaptured = true;
}

void WindowManager::EnsureMouseCaptured()
{
	if (!m_mouseCaptured)
		CaptureMouse();
}

void WindowManager::UncaptureMouse()
{
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	m_mouseCaptured = false;
}

void WindowManager::EnsureMouseUncaptured()
{
	// Stay grabbed in fullscreen mode
	// So that mouse doesn't jump to it's actual position, and
	// for buggy linux Xinerama (see input_sdl.cpp)
	if (!m_windowed)
		return;

	if (m_mouseCaptured)
		UncaptureMouse();
}

void WindowManager::EnableOpenGL(int _colourDepth, int _zDepth)
{
}

void WindowManager::DisableOpenGL()
{
}

void WindowManager::HideMousePointer()
{
	SDL_ShowCursor(false);
	m_mousePointerVisible = false;
}

void WindowManager::UnhideMousePointer()
{
	SDL_ShowCursor(true);
	m_mousePointerVisible = true;
}

void WindowManager::OpenWebsite( char *_url )
{	
#ifdef TARGET_OS_MACOSX 
#ifdef AMBROSIA_REGISTRATION
	 IT_OpenBrowser(_url);
#else
	const char *cmd = "/usr/bin/open";
	char buffer[strlen(_url) + 1 + strlen(cmd) + 1];
	sprintf(buffer, "%s %s", cmd, _url);
	system(buffer);
#endif
#elif defined TARGET_OS_LINUX
	const char *cmd = "/bin/sh open-www.sh";
	char buffer[strlen(_url) + 1 + strlen(cmd) + 1];
	sprintf(buffer, "%s %s", cmd, _url);
	system(buffer);
#endif
}

#if defined(TARGET_OS_LINUX) || defined (TARGET_OS_MACOSX)
void SetupMemoryAccessHandlers();
void SetupPathToProgram(const char *program);

#include <string>
#include <unistd.h>

char *g_origWorkingDir = NULL;

void ChangeToProgramDir(const char *program)
{
	std::string dir(program);
	std::string::size_type pos = dir.find_last_of('/');

	// Store the original working directory
	g_origWorkingDir = new char [PATH_MAX];
	getcwd(g_origWorkingDir, PATH_MAX);

	if (pos != std::string::npos) 
		dir.erase(pos);
	DarwiniaReleaseAssert(
		chdir(dir.c_str()) == 0, 
		"Failed to change directory to %s", dir.c_str());
}

#endif // TARGET_OS_LINUX || TARGET_OS_MACOSX

#ifdef AMBROSIA_REGISTRATION

#define NUM_PIRATED_CODES 20

void CheckAllPirateCodes(bool *isValid, Uint64 licenseCode)
{
	// Check all the pirate codes and display a message and quit if any bogus
	UInt64 allPiratedCodes[NUM_PIRATED_CODES] = {
		RT3_PIRATED_FAKE_01,
		RT3_PIRATED_FAKE_02,
		RT3_PIRATED_FAKE_03,
		RT3_PIRATED_FAKE_04,
		RT3_PIRATED_FAKE_05,
		RT3_PIRATED_FAKE_06,
		RT3_PIRATED_FAKE_07,
		RT3_PIRATED_FAKE_08,
		RT3_PIRATED_FAKE_09,
		RT3_PIRATED_FAKE_10,
		RT3_PIRATED_FAKE_11,
		RT3_PIRATED_FAKE_12,
		RT3_PIRATED_FAKE_13,
		RT3_PIRATED_FAKE_14,
		RT3_PIRATED_FAKE_15,
		RT3_PIRATED_FAKE_16,
		RT3_PIRATED_FAKE_17,
		RT3_PIRATED_FAKE_18,
		RT3_PIRATED_FAKE_19,
		RT3_PIRATED_FAKE_20,
	};
	
	for (int i = 0; i < NUM_PIRATED_CODES; i++) 
		qRT3_LicenseTestPirate(licenseCode,allPiratedCodes[i],isValid[i]);
	
	return isValid;
}
#endif

// g_windowManager is assumed to be dynamically allocated
// and is deleted in main ordinarily
class DeleteWindowManagerOnExit {
	public:
	~DeleteWindowManagerOnExit()
	{
		delete g_windowManager;
		g_windowManager = NULL;
	}
};

static DeleteWindowManagerOnExit please;

int main(int argc, char *argv[])
{
	// It's really important under linux at least that
	// SDL_Quit is called to restore fullscreen mode.
	//
	// So we make WindowManager a static object, so that
	// it's destructor will be called on all normal
	// exits.

	g_windowManager	= new WindowManager;
	
#if defined(TARGET_OS_LINUX)
	SetupPathToProgram(SELFPATH);
	ChangeToProgramDir(SELFPATH);
#endif

	SDL_WM_SetCaption("Darwinia", NULL);

	if (argc > 1) {
		if (strncmp(argv[1], "-v", 2) == 0) {
			puts(DARWINIA_VERSION_STRING);
			return 0;
		}
	}
	
	SDL_version compiledVersion;
	SDL_VERSION(&compiledVersion);
	const SDL_version *linkedVersion = SDL_Linked_Version();
	
	printf("SDL Version: Compiled against %d.%d.%d, running with %d.%d.%d\n", 
		compiledVersion.major, compiledVersion.minor, compiledVersion.patch,
		linkedVersion->major, linkedVersion->minor, linkedVersion->patch);
		
#ifdef AMBROSIA_REGISTRATION
	// Ambrosia Registration Tool
	RT3_Open(true, VERSION_3_CODES, RT3_PRODUCT_NAME, "Darwinia");
	SystemSetQuitProc(RT3_Close);
	atexit(RT3_Close);
	
	// Ambrosia's Display Notice (Nag)
	Bool8 launchedRegApp;
	Result32 result = RT3_DisplayNotice(false, &launchedRegApp);
	DarwiniaReleaseAssert(result == 0, "Failed to display Ambrosia Registration Notice");
	
	// Check for pirate registration codes
	bool isValid[NUM_PIRATED_CODES];
	memset(isValid, 0, sizeof(bool) * NUM_PIRATED_CODES);
	CheckAllPirateCodes(isValid, RT3_GetLicenseCode());
	
	for (int i = 0; i < NUM_PIRATED_CODES; i++) {
		if (!isValid[i])
			FT_FileDelete(kPathRefPreferences, "Darwinia License");
		DarwiniaReleaseAssert(isValid[i], "Check registration code (1)");
	}
#endif // AMBROSIA_REGISTRATION

#if defined(TARGET_OS_LINUX) || defined (TARGET_OS_MACOSX)
	// Setup illegal memory access handler
	// See debug_utils_gcc.cpp
	SetupMemoryAccessHandlers();
#endif

	AppMain();
	
	return 0;
}

