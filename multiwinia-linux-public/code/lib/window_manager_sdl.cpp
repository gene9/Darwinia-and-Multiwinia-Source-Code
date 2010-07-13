#include "lib/universal_include.h"

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "window_manager_sdl.h"
#include "lib/input/sdl_eventhandler.h"
#include "lib/input/inputdriver_sdl_mouse.h"
//#include "input.h"
#include "network/server.h"
#include "app.h"

//#include "app.h"
//#include "renderer.h"

#ifdef TARGET_OS_LINUX
#include "contrib/binreloc/prefix.h"
//#include "spawn.h"
#endif

#ifdef TARGET_OS_MACOSX
#include <ApplicationServices/ApplicationServices.h>
#endif

SDLMouseInputDriver *g_sdlMouseDriver = NULL;

// Uncomment if you want lots of output in debug mode.
//#define VERBOSE_DEBUG

// *** Constructor
WindowManagerSDL::WindowManagerSDL()
	:	m_tryingToCaptureMouse(false),
		m_setVideoMode(false),
		m_preventFullscreenStartup(false)
{
	AppReleaseAssert(SDL_Init(SDL_INIT_VIDEO) == 0, "Couldn't initialise SDL");

	ListAllDisplayModes();
	SaveDesktop();
}

WindowManagerSDL::~WindowManagerSDL()
{
	while ( m_resolutions.ValidIndex ( 0 ) )
	{
		Resolution *res = m_resolutions.GetData ( 0 );
		delete res;
		m_resolutions.RemoveData ( 0 );
	}
	m_resolutions.EmptyAndDelete();
	SDL_Quit();
}

void WindowManagerSDL::ListAllDisplayModes()
{
	SDL_Rect **validModes = SDL_ListModes(NULL, SDL_OPENGL|SDL_FULLSCREEN);

	for (; *validModes; validModes++) {
		SDL_Rect *mode = *validModes;
		Resolution *res = new Resolution(mode->w, mode->h);
		if (GetResolutionId(mode->w, mode->h) == -1)
			m_resolutions.PutData( res );
	}
}


void WindowManagerSDL::PreventFullscreenStartup()
{
	if (!m_setVideoMode)
		m_preventFullscreenStartup = true;
}


void WindowManagerSDL::SaveDesktop()
{
    const SDL_VideoInfo* info = SDL_GetVideoInfo();

    m_desktopColourDepth = info->vfmt->BitsPerPixel;
    m_desktopRefresh = 60;

#ifdef TARGET_OS_MACOSX
	CGRect rect = CGDisplayBounds( CGMainDisplayID() );
    m_desktopScreenW = rect.size.width;
    m_desktopScreenH = rect.size.height;
#else
    m_desktopScreenW = info->current_w;
    m_desktopScreenH = info->current_h;
#endif
}


void WindowManagerSDL::RestoreDesktop()
{
}


void WindowManagerSDL::NastySetMousePos(int x, int y)
{
	m_x = x;
	m_y = y;
}


void WindowManagerSDL::NastyMoveMouse(int x, int y)
{ }


bool WindowManagerSDL::CreateWin(int _width, int _height, bool _windowed, int _colourDepth, int _refreshRate,
								 int _zDepth, bool _waitVRT, bool _antiAlias, const wchar_t *_title)
{
    int bpp = 0;
    int flags = 0;
	SDL_Init(SDL_INIT_VIDEO);
    const SDL_VideoInfo* info = SDL_GetVideoInfo();
	SDL_PixelFormat vfmt = *info->vfmt;
	
    AppReleaseAssert(info, "SDL_GetVideoInfo failed: %s", SDL_GetError());
	
	m_windowed = _windowed || m_preventFullscreenStartup;
	m_preventFullscreenStartup = false;

	// Set the flags for creating the mode
    flags = SDL_OPENGL; 
	if (!_windowed)
	{
		flags |= SDL_FULLSCREEN;
		
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
		AppReleaseAssert(bestMode != NULL, "Failed to find any valid video modes");
	
		m_screenW = bestMode->w;
		m_screenH = bestMode->h;		
		bpp = vfmt.BitsPerPixel;
	}
	else {
#ifdef TARGET_OS_MACOSX
		const SDL_version *linkedVersion = SDL_Linked_Version();
		// We ensure that the we are linked with SDL version >= 1.2.9 because
		// previous versions had major problems with the coordinate system 
		// when using OpenGL, in windowed and in full-screen mode.
		
		AppReleaseAssert(linkedVersion->major * 1000 + linkedVersion->minor * 100 + linkedVersion->patch >= 1209,
			"App requires at to run with SDL version 1.2.9 or greater");		
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
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, _zDepth );

 	if ( _antiAlias ) {		
 		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 2 );
 		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, 4 );
 	}
	else {
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 0 );
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, 0 );
	}
	
	// Synchronize to the vertical refresh rate of the monitor, typically 60Hz. This
	// does end up causing graphics flushing to block eventually. But that's what we
	// want.
	SDL_GL_SetAttribute ( SDL_GL_SWAP_CONTROL, 1 );
	
	bpp = SDL_VideoModeOK(m_screenW, m_screenH, _colourDepth, flags);
	if (!bpp)
		return false;
	
	m_setVideoMode = SDL_SetVideoMode(m_screenW, m_screenH, bpp, flags);

	// Fall back to not antialiased
	if (!m_setVideoMode && _antiAlias) {
		_antiAlias = 0;
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 0 );
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, 0 );
		m_setVideoMode = SDL_SetVideoMode(m_screenW, m_screenH, bpp, flags);
	}
	
	// Fall back to a 16 bit Z-Buffer
	if (!m_setVideoMode && _zDepth != 16) {
		AppDebugOut ( "SDL_SetVideoMode failed with '%s'. Switching to 16-bit Z-Buffer.\n", SDL_GetError() );
		_zDepth = 16;
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, _zDepth );
		m_setVideoMode = SDL_SetVideoMode(m_screenW, m_screenH, bpp, flags);
	}
	
	if (!m_setVideoMode)
	{
		AppDebugOut ( "SDL_SetVideoMode failed with '%s'. Can't continue.\n", SDL_GetError() );
		return false;
	}

	// Pass back the actual values to the Renderer
	_width = m_screenW;
	_height = m_screenH;
	_colourDepth = bpp;

	if (m_mouseCaptured)
		CaptureMouse();
	
	UnicodeString unicodeTitle(_title);
	const char *asciiTitle = unicodeTitle.GetCharArray();
	SDL_WM_SetCaption(asciiTitle, asciiTitle);

	//Reset the mouse pointer visibility when we recrate the window.
	SDL_ShowCursor(m_mousePointerVisible);
	
	#ifdef TARGET_OS_LINUX
	//We have to re-enable unicode whenever we re-create the window.
	//As on linux we actually destroy the old window, such state is not
	//saved.
	SDL_EnableUNICODE(1);
	
	//Prevent the input manager from stuffing up when the mouse
	//coordinates change without valid xrels (occurs during resolution
	//change).
	SDL_Event e;
	e.type = SDL_MOUSEMOTION;
	e.motion.xrel = -m_x;
	e.motion.yrel = -m_y;
	static_cast<SDLEventHandler *>(g_eventHandler)->HandleSDLEvent(e);
	#endif
	return true;
}


void WindowManagerSDL::DestroyWin()
{
#ifdef TARGET_OS_LINUX
		glFinish();
		SDL_GetMouseState(&m_x,&m_y);
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_WM_GrabInput(SDL_GRAB_OFF); //Just in case
#endif
}


void WindowManagerSDL::Flip()
{
	PollForMessages();
	
	if (m_tryingToCaptureMouse) 
		g_windowManager->CaptureMouse();
	
	SDL_GL_SwapBuffers();
}

void WindowManagerSDL::PollForMessages()
{
	SDL_Event sdlEvent;

	while (SDL_PollEvent(&sdlEvent))
		static_cast<SDLEventHandler *>(g_eventHandler)->HandleSDLEvent(sdlEvent);
	
	if (m_tryingToCaptureMouse)
		CaptureMouse();
}


void WindowManagerSDL::EnsureMouseCaptured()
{
	if (g_app->m_gameMode != App::GameModeNone)
		CaptureMouse();
}


void WindowManagerSDL::EnsureMouseUncaptured()
{
	UncaptureMouse();
}


void WindowManagerSDL::CaptureMouse()
{
	if (m_mouseCaptured)
		return;
		
#ifdef TARGET_OS_MACOSX
	// Important not to grab the mouse 
	// until it's in the window proper. Otherwise
	// we might end up doing some strange things on MAC OS X
	 if (!(SDL_GetAppState() & SDL_APPMOUSEFOCUS)) {
		m_tryingToCaptureMouse = true;
		return;
	}
#endif
		
	// Don't grab if we don't have focus
	if (!g_eventHandler->WindowHasFocus())
		return;
		
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_WM_GrabInput(SDL_GRAB_ON);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);

	m_mouseCaptured = true;
	m_tryingToCaptureMouse = false;
}


void WindowManagerSDL::UncaptureMouse()
{
	m_tryingToCaptureMouse = false;
	if (!m_mouseCaptured)
		return;
		
#ifdef VERBOSE_DEBUG
	AppDebugOut("Uncapturing mouse\n");
#endif
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_WarpMouse(m_x, m_y);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	g_sdlMouseDriver->SetMousePosNoVelocity(m_x, m_y);

	m_mouseCaptured = false;
}


void WindowManagerSDL::HideMousePointer()
{
	SDL_ShowCursor(false);
	m_mousePointerVisible = false;
}


void WindowManagerSDL::UnhideMousePointer()
{
	SDL_ShowCursor(true);
	m_mousePointerVisible = true;
}


void WindowManagerSDL::SetMousePos(int x, int y)
{
	if (!m_mouseCaptured)
		SDL_WarpMouse(x, y);
}


void WindowManagerSDL::OpenWebsite( const char *_url )
{	
#ifdef TARGET_OS_MACOSX 
	CFURLRef url = CFURLCreateWithBytes(NULL, (const UInt8 *)_url, strlen(_url),
										kCFStringEncodingASCII, NULL);
	if (url)
	{
		LSOpenCFURLRef(url, NULL);
		CFRelease(url);
	}
#elif defined TARGET_OS_LINUX
	/* Child */
	//char * const args[4] = { "/bin/sh", "open-www.sh", (char *)_url,  NULL };
	//spawn("/bin/sh", args);
	char cmd[256];
	AppReleaseAssert(strlen(_url) < 255, "_url is too long");
	sprintf(cmd,"./open-www.sh %s",_url);
	system(cmd);
#endif
}

void WindowManagerSDL::HideWin()
{
#ifdef TARGET_OS_MACOSX
	ProcessSerialNumber me;
	
	GetCurrentProcess(&me);
	ShowHideProcess(&me, false);
#endif
}

#ifdef TARGET_OS_LINUX
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
	AppReleaseAssert(
		chdir(dir.c_str()) == 0, 
		"Failed to change directory to %s", dir.c_str());
}

#endif // TARGET_OS_LINUX || TARGET_OS_MACOSX

void AppMain( const char *_cmdLine );

int main(int argc, char *argv[])
{
	g_windowManager = new WindowManagerSDL();
	
#if defined(TARGET_OS_LINUX)// && defined(TARGET_NO_CMAKE_HACKS)
	SetupPathToProgram(SELFPATH);
	ChangeToProgramDir(SELFPATH);
#endif

	if (argc > 1) {
		if (strncmp(argv[1], "-v", 2) == 0) {
			puts(APP_VERSION);
			return 0;
		}
	}
	
	SDL_version compiledVersion;
	SDL_VERSION(&compiledVersion);
	const SDL_version *linkedVersion = SDL_Linked_Version();
	
#ifdef DUMP_DEBUG_LOG
    AppDebugOutRedirect("debug.txt");
#endif
	AppDebugOut("SDL Version: Compiled against %d.%d.%d, running with %d.%d.%d\n", 
		        compiledVersion.major, compiledVersion.minor, compiledVersion.patch,
		        linkedVersion->major, linkedVersion->minor, linkedVersion->patch);

#if defined(TARGET_OS_LINUX) && defined(TARGET_NO_CMAKE_HACKS)
	// Setup illegal memory access handler
	// See debug_utils_gcc.cpp
	SetupMemoryAccessHandlers();
#endif

	AppMain(""); // don't bother passing through command line args
	
	return 0;
}

void WindowManagerSDL::NastyPollForMessages()
{

}

PlatformWindow *WindowManagerSDL::Window()
{
	SDL_SysWMinfo *info = (SDL_SysWMinfo *)malloc(sizeof(SDL_SysWMinfo));
	
	SDL_VERSION(&(info->version));
	if (SDL_GetWMInfo(info) != -1)
		return (PlatformWindow *)info;
	else
	{
		free(info);
		return NULL;
	}
}
