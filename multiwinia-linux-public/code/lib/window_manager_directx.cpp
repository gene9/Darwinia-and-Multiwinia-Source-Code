#include "lib/universal_include.h"

#ifdef USE_DIRECT3D
#include "lib/ogl_extensions.h"

#include <limits.h>
#include <windows.h>
#include <shellapi.h>

#include "lib/debug_utils.h"
#include "lib/window_manager.h"
#include "lib/window_manager_directx.h"
#include "lib/input/inputdriver_win32.h"
#include "lib/input/win32_eventhandler.h"

#include "main.h"
#include "renderer.h"

static HINSTANCE g_hInstance;

#define WH_KEYBOARD_LL 13
#define LLKHF_ALTDOWN 0x20

bool Direct3DInit(HWND _hWnd, bool _windowed, int _width, int _height, int _colourDepth, int _zDepth, bool _waitVRT);
void Direct3DShutdown();
void Direct3DSwapBuffers();

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	W32EventHandler *w = getW32EventHandler();

	if ( !w || ( w->WndProc( hWnd, message, wParam, lParam ) == -1 ) )	
	{
		return DefWindowProc( hWnd, message, wParam, lParam );
	}

	return 0;
}


WindowManagerDirectX::WindowManagerDirectX()
:	m_hWnd(NULL),
	m_hDC(NULL),
	m_hRC(NULL)
{
	AppDebugAssert(g_windowManager == NULL);

	ListAllDisplayModes();

    SaveDesktop();
}


WindowManagerDirectX::~WindowManagerDirectX()
{
	DestroyWin();
	m_resolutions.EmptyAndDelete();
}


void WindowManagerDirectX::SaveDesktop()
{
    DEVMODE mode;
    ZeroMemory(&mode, sizeof(mode));
    mode.dmSize = sizeof(mode);
    bool success = EnumDisplaySettings ( NULL, ENUM_CURRENT_SETTINGS, &mode );
            
    m_desktopScreenW = mode.dmPelsWidth;
    m_desktopScreenH = mode.dmPelsHeight;
    m_desktopColourDepth = mode.dmBitsPerPel;
    m_desktopRefresh = mode.dmDisplayFrequency;
}


void WindowManagerDirectX::RestoreDesktop()
{
	/*
    //
    // Get current settings

    DEVMODE mode;
    ZeroMemory(&mode, sizeof(mode));
    mode.dmSize = sizeof(mode);
    bool success = EnumDisplaySettings ( NULL, ENUM_CURRENT_SETTINGS, &mode );

    //
    // has anything changed?

    bool changed = ( m_desktopScreenW != mode.dmPelsWidth ||
                     m_desktopScreenH != mode.dmPelsHeight ||
                     m_desktopColourDepth != mode.dmBitsPerPel ||
                     m_desktopRefresh != mode.dmDisplayFrequency );


    //
    // Restore if required

    if( changed )
    {
	    DEVMODE devmode;
	    devmode.dmSize = sizeof(DEVMODE);
	    devmode.dmBitsPerPel = m_desktopColourDepth;
	    devmode.dmPelsWidth = m_desktopScreenW;
	    devmode.dmPelsHeight = m_desktopScreenH;
	    devmode.dmDisplayFrequency = m_desktopRefresh;
	    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_BITSPERPEL;
	    long result = ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);
    }
	*/
}

bool WindowManagerDirectX::EnableOpenGL(int _colourDepth, int _zDepth)
{
	if(!Direct3DInit(m_hWnd, m_windowed, m_screenW, m_screenH, _colourDepth, _zDepth, m_waitVRT))
		return false;
	glClear(GL_COLOR_BUFFER_BIT);
	return true;
}

void WindowManagerDirectX::DisableOpenGL()
{
	Direct3DShutdown();
//	wglMakeCurrent( NULL, NULL );
//	wglDeleteContext( m_win32Specific->m_hRC );
//	ReleaseDC( m_win32Specific->m_hWnd, m_win32Specific->m_hDC );
}

void WindowManagerDirectX::ListAllDisplayModes()
{
	int i = 0;
	DEVMODE devMode;
	while (EnumDisplaySettings(NULL, i, &devMode) != 0)
	{
		if (devMode.dmBitsPerPel >= 15 &&
			devMode.dmPelsWidth >= 640 &&
			devMode.dmPelsHeight >= 480)
		{
			int resId = GetResolutionId(devMode.dmPelsWidth, devMode.dmPelsHeight);
			Resolution *res;
			if (resId == -1)
			{
				res = new Resolution(devMode.dmPelsWidth, devMode.dmPelsHeight);
				m_resolutions.PutDataAtEnd(res);
			}
			else
			{
				res = m_resolutions[resId];
			}

			if (res->m_refreshRates.FindData(devMode.dmDisplayFrequency) == -1)
			{
				res->m_refreshRates.PutDataAtEnd(devMode.dmDisplayFrequency);
			}
		}
		++i;
	}
}


bool WindowManagerDirectX::CreateWin(int _width, int _height, bool _windowed, 
	           				     int _colourDepth, int _refreshRate, int _zDepth, bool _waitVRT,
								 bool _antiAlias, const char *_title)
{
	m_screenW = _width;
	m_screenH = _height;
    m_windowed = _windowed;
	m_waitVRT = _waitVRT;

	// Register window class
	WNDCLASS wc;
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hInstance;
	wc.hIcon = LoadIcon( GetModuleHandle(NULL), MAKEINTRESOURCE(101) );
	wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = _title;
	RegisterClass( &wc );

	int posX, posY;
	unsigned int windowStyle = WS_VISIBLE;

	if (_windowed)
	{
        RestoreDesktop();

        windowStyle |= WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		RECT windowRect = { 0, 0, _width, _height };
		AdjustWindowRect(&windowRect, windowStyle, false);
		m_borderWidth = ((windowRect.right - windowRect.left) - _width) / 2;
		m_titleHeight = ((windowRect.bottom - windowRect.top) - _height) - m_borderWidth * 2;
		

		HWND desktopWindow = GetDesktopWindow();
		RECT desktopRect;
		GetWindowRect(desktopWindow, &desktopRect);
		int desktopWidth = desktopRect.right - desktopRect.left;
		int desktopHeight = desktopRect.bottom - desktopRect.top;

		if(_width > desktopWidth || _height > desktopHeight )
        {
            return false;
        }

        _width += m_borderWidth * 2;
		_height += m_borderWidth * 2 + m_titleHeight;

		posX = (desktopRect.right - _width) / 2;
		posY = (desktopRect.bottom - _height) / 2;
	}
	else
	{
		windowStyle |= WS_POPUP;//|WS_MAXIMIZE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;

		/*
		DEVMODE devmode;
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmBitsPerPel = _colourDepth;
		devmode.dmPelsWidth = _width;
		devmode.dmPelsHeight = _height;
		devmode.dmDisplayFrequency = _refreshRate;
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		long result = ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);
        if( result != DISP_CHANGE_SUCCESSFUL ) return false;
		*/
        
//		AppReleaseAssert(result == DISP_CHANGE_SUCCESSFUL, "Couldn't set full screen mode of %dx%d", 
//														_width, _height);
//      This assert goes off on many systems, regardless of success

        posX = 0;
		posY = 0;
		m_borderWidth = 1;
		m_titleHeight = 0;
	}

	// Create main window
	m_hWnd = CreateWindow( 
		wc.lpszClassName, wc.lpszClassName, 
		windowStyle,
		posX, posY, _width, _height,
		NULL, NULL, g_hInstance, NULL );

    if( !m_hWnd ) return false;

	m_mouseOffsetX = INT_MAX;

	// Enable OpenGL for the window
	if(!EnableOpenGL(_colourDepth, _zDepth))
	{
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
		return false;
	}

    return true;
}


void WindowManagerDirectX::DestroyWin()
{
	DisableOpenGL();
	DestroyWindow(m_hWnd);
	m_hWnd = NULL;
}


PlatformWindow *WindowManagerDirectX::Window()
{
	return (PlatformWindow *)m_hWnd;
}


void WindowManagerDirectX::Flip()
{
	Direct3DSwapBuffers();
}


void WindowManagerDirectX::NastyPollForMessages()
{
	MSG msg;
	while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) 
	{
		// handle or dispatch messages
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	} 
}


void WindowManagerDirectX::NastySetMousePos(int x, int y)
{
	if (m_mouseOffsetX == INT_MAX)
	{
		RECT rect1;
		GetWindowRect(m_hWnd, &rect1);

		RECT rect2;
		GetClientRect(m_hWnd, &rect2);

		//int borderWidth = (m_screenW - rect2.right) / 2;
		//int menuHeight = (m_screenH - rect2.bottom) - (borderWidth * 2);
		if (m_windowed) {
			m_mouseOffsetX = rect1.left + m_borderWidth;
			m_mouseOffsetY = rect1.top + m_borderWidth + m_titleHeight;
		}
		else {
			m_mouseOffsetX = 0;
			m_mouseOffsetY = 0;
		}
	}

	SetCursorPos(x + m_mouseOffsetX, y + m_mouseOffsetY);

	extern W32InputDriver *g_win32InputDriver;
	g_win32InputDriver->SetMousePosNoVelocity(x, y);
}


void WindowManagerDirectX::NastyMoveMouse(int x, int y)
{
	POINT pos;
	GetCursorPos( &pos );
	SetCursorPos( x + pos.x, y + pos.y );

	extern W32InputDriver *g_win32InputDriver;
	g_win32InputDriver->SetMousePosNoVelocity(x, y);
}

void WindowManagerDirectX::WindowMoved()
{
	m_mouseOffsetX = INT_MAX;
}

bool WindowManagerDirectX::Captured()
{
	return m_mouseCaptured;
}

void WindowManagerDirectX::CaptureMouse()
{
	SetCapture(m_hWnd);
	m_mouseCaptured = true;
}

void WindowManagerDirectX::EnsureMouseCaptured()
{
	// Called from camera.cpp Camera::Advance
	//
	// Might not need to do anything, the intention is
	// to have the mouse/keyboard captured whenever
	// the mouse is being warped (i.e. not in 
	// menu mode)
	// 
	// Look carefully at input.cpp if implementing this
	// code, since that calls CaptureMouse / UncaptureMouse
	// on various input events

// 	if (!m_mouseCaptured)
// 		CaptureMouse();
}

void WindowManagerDirectX::UncaptureMouse()
{
	ReleaseCapture();
	m_mouseCaptured = false;
}

void WindowManagerDirectX::EnsureMouseUncaptured()
{
	// Called from camera.cpp Camera::Advance
	//
	// Might not need to do anything, the intention is
	// to have the mouse/keyboard captured whenever
	// the mouse is being warped (i.e. not in 
	// menu mode)
	// 
	// Look carefully at input.cpp if implementing this
	// code, since that calls CaptureMouse / UncaptureMouse
	// on various input events

// 	if (m_mouseCaptured)
// 		UncaptureMouse();
}

void WindowManagerDirectX::HideMousePointer()
{
	// TODO: Delete me
//	if (m_mousePointerVisible)
//		ShowCursor(false);
	m_mousePointerVisible = false;
}


void WindowManagerDirectX::UnhideMousePointer()
{
	// TODO: Delete me
//	if (!m_mousePointerVisible)
//		ShowCursor(true);
	m_mousePointerVisible = true;
}


void WindowManagerDirectX::OpenWebsite( const char *_url )
{
    ShellExecute(NULL, "open", _url, NULL, NULL, SW_SHOWNORMAL); 
}


int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE _hPrevInstance, 
				   LPSTR _cmdLine, int _iCmdShow)
{
	// uncomment to see new/malloc leaks in debug output
	//_CrtSetDbgFlag( (_CrtSetDbgFlag( _CRTDBG_REPORT_FLAG )|_CRTDBG_LEAK_CHECK_DF)&~_CRTDBG_CHECK_CRT_DF );
	// uncomment and set to catch leak with given alloc id
	//_crtBreakAlloc = 110;

	g_hInstance = _hInstance;

	g_windowManager	= new WindowManagerDirectX();

#ifdef TARGET_OS_VISTA
    if( strlen( _cmdLine ) > 0 )
    {
		if( strcmp( _cmdLine, "-mediacenter" ) == 0 )
		{
			g_mediaCenter = true;
			Sleep(7500);
		}
		else
		{
			char *fileName;
			fileName = strrchr( _cmdLine, '\\' ) + 1;

			if( fileName )
			{
				strtok( fileName, "." );
				strcpy( g_saveFile, fileName );
			}
			char appPath[_MAX_PATH];
			if( GetModuleFileName(NULL, appPath, _MAX_PATH ) > 0 )
			{
				char *path = strrchr( appPath, '\\' );
				appPath[strlen(appPath)-strlen(path)] = '\0';
				SetCurrentDirectory(appPath);
			}
		}
    }
#endif

	AppMain( (char *) _cmdLine );
	
	return WM_QUIT;
}
#endif // USE_DIRECT3D
