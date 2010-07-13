#include "lib/universal_include.h"

#include <SDL/SDL.h>

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <fenv.h>

#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "window_manager_sdl.h"
#include "input.h"

//#include "app.h"
//#include "renderer.h"

#ifdef TARGET_OS_LINUX
#include "prefix.h"
#include "spawn.h"
#endif

#ifdef AMBROSIA_REGISTRATION
#include "lib/ambrosia.h"
#include "file_tool.h"
#include "interface_tool.h"
#include "platform.h"
#endif

#ifdef TARGET_OS_MACOSX
#include <ApplicationServices/ApplicationServices.h>
#endif

// Hack to get access to command line parameters elsewhere
int g_argc;
char ** g_argv;

// Uncomment if you want lots of output in debug mode.
//#define VERBOSE_DEBUG

// *** Constructor
WindowManagerSDL::WindowManagerSDL()
	:	m_tryingToCaptureMouse(false)
{
	AppReleaseAssert(SDL_Init(SDL_INIT_VIDEO) == 0, "Couldn't initialise SDL");

	ListAllDisplayModes();
	SaveDesktop();
}

WindowManagerSDL::~WindowManagerSDL()
{
	while ( m_resolutions.ValidIndex ( 0 ) )
	{
		WindowResolution *res = m_resolutions.GetData ( 0 );
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
		WindowResolution *res = new WindowResolution(mode->w, mode->h);
		if (GetResolutionId(mode->w, mode->h) == -1)
			m_resolutions.PutData( res );
	}
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
	#warning "Need to do code for linux to determine default WindowResolution"

    m_desktopScreenW = 1024;
    m_desktopScreenH = 768;
#endif
}


void WindowManagerSDL::RestoreDesktop()
{
}


bool WindowManagerSDL::CreateWin(int _width, int _height, bool _windowed, int _colourDepth, int _refreshRate, int _zDepth, int _antiAlias, const char *_title)
{
    int bpp = 0;
    int flags = 0;

    const SDL_VideoInfo* info = SDL_GetVideoInfo();
	SDL_PixelFormat vfmt = *info->vfmt;
	
    AppReleaseAssert(info, "SDL_GetVideoInfo failed: %s", SDL_GetError());
	
	m_windowed = _windowed;

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
		WindowResolution *found = NULL;
		for (int i = 0; i < m_resolutions.Size(); i++) {
			WindowResolution *res = m_resolutions.GetData(i);
			if (res->m_width == _width && res->m_height == _height) {
				found = res;
				break;
			}
		}
		
		if (!found) {
			WindowResolution *res = new WindowResolution(_width, _height);
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

 	if ( _antiAlias ) {		
 		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 2 );
 		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, 4 );
 	}
	else {
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 0 );
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, 0 );
	}
	
	bpp = SDL_VideoModeOK(m_screenW, m_screenH, _colourDepth, flags);
	if (!bpp)
		return false;
	
	setVideoMode = SDL_SetVideoMode(m_screenW, m_screenH, bpp, flags);

	// Fall back to not antialiased
	if (!setVideoMode && _antiAlias) {
		_antiAlias = 0;
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLEBUFFERS, 0 );
		SDL_GL_SetAttribute ( SDL_GL_MULTISAMPLESAMPLES, 0 );
		setVideoMode = SDL_SetVideoMode(m_screenW, m_screenH, bpp, flags);
	}
	
	// Fall back to a 16 bit Z-Buffer
	if (!setVideoMode && _zDepth != 16) {
		AppDebugOut ( "SDL_SetVideoMode failed with '%s'. Switching to 16-bit Z-Buffer.\n", SDL_GetError() );
		_zDepth = 16;
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, _zDepth );
		setVideoMode = SDL_SetVideoMode(m_screenW, m_screenH, bpp, flags);
	}
	
	if (!setVideoMode)
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
		
	SDL_WM_SetCaption(_title, _title);

	return true;
}


void WindowManagerSDL::DestroyWin()
{
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
	{	
		int result = g_inputManager->EventHandler(sdlEvent.type, (unsigned int)&sdlEvent, 0);
		
		if (result == -1 && m_secondaryMessageHandler)
			m_secondaryMessageHandler( sdlEvent.type, (unsigned int)&sdlEvent, 0 );
	}
	
	if (m_tryingToCaptureMouse)
		CaptureMouse();
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
	if (!g_inputManager->m_windowHasFocus)
		return;
		
	SDL_WM_GrabInput(SDL_GRAB_ON);
	m_mouseCaptured = true;
	m_tryingToCaptureMouse = false;
}


void WindowManagerSDL::UncaptureMouse()
{
	m_tryingToCaptureMouse = false;

	// Stay grabbed in fullscreen mode
	// So that mouse doesn't jump to it's actual position, and
	// for buggy linux Xinerama (see input_sdl.cpp)
	if (!m_windowed)
		return;
		
#ifdef VERBOSE_DEBUG
	AppDebugOut("Uncapturing mouse\n");
#endif
	SDL_WM_GrabInput(SDL_GRAB_OFF);
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
#ifdef AMBROSIA_REGISTRATION
	 IT_OpenBrowser(_url);
#else
	const char *cmd = "/usr/bin/open";
	char buffer[strlen(_url) + 1 + strlen(cmd) + 1];
	sprintf(buffer, "%s %s", cmd, _url);
	system(buffer);
#endif
#elif defined TARGET_OS_LINUX
	/* Child */
	char * const args[4] = { "/bin/sh", "open-www.sh", (char *)_url,  NULL };
	spawn("/bin/sh", args);
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

#if defined(TARGET_OS_LINUX)
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

int main(int argc, char *argv[])
{
	g_argc = argc;
	g_argv = argv;

#if defined(TARGET_OS_LINUX)
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

#if defined(TARGET_OS_LINUX) 
	// Setup illegal memory access handler
	// See debug_utils_gcc.cpp
	SetupMemoryAccessHandlers();
#endif

	AppMain();
	
	return 0;
}

