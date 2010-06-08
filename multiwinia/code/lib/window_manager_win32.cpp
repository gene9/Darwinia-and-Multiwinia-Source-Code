#include "lib/universal_include.h"

#if !defined USE_DIRECT3D
#include "lib/ogl_extensions.h"

#include <limits.h>
#include <windows.h>
#include <shellapi.h>

#ifdef TARGET_MSVC
#include "lib/input/win32_eventhandler.h"
#endif
#include "lib/debug_utils.h"
#include "lib/window_manager_win32.h"

#include "main.h"
#include "renderer.h"
#include "lib/work_queue.h"

static HINSTANCE g_hInstance;

#define WH_KEYBOARD_LL 13

#ifndef LLKHF_ALTDOWN
#define LLKHF_ALTDOWN 0x20
#endif


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	W32EventHandler *w = getW32EventHandler();

	if ( !w || ( w->WndProc( hWnd, message, wParam, lParam ) == -1 ) )
	{
		return DefWindowProcW( hWnd, message, wParam, lParam );
	}

	return 0;
}


WindowManagerWin32::WindowManagerWin32()
:	m_hWnd(NULL),
	m_hDC(NULL),
	m_hRC(NULL),
	m_hRC2(NULL)
{
	AppDebugAssert(g_windowManager == NULL);

	ListAllDisplayModes();

    SaveDesktop();
}


WindowManagerWin32::~WindowManagerWin32()
{
	DestroyWin();
}


void WindowManagerWin32::SaveDesktop()
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


void WindowManagerWin32::RestoreDesktop()
{
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
}


bool WindowManagerWin32::EnableOpenGL(int _colourDepth, int _zDepth)
{
	PIXELFORMATDESCRIPTOR pfd;
	int format;
	
	// Get the device context (DC)
	m_hDC = GetDC( m_hWnd );
	
	// Set the pixel format for the DC
	ZeroMemory( &pfd, sizeof( pfd ) );
	pfd.nSize = sizeof( pfd );
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = _colourDepth;
	pfd.cDepthBits = _zDepth;
	pfd.iLayerType = PFD_MAIN_PLANE;
	format = ChoosePixelFormat( m_hDC, &pfd );
	SetPixelFormat( m_hDC, format, &pfd );
	
	// Create and enable the render context (RC)
	m_hRC = wglCreateContext( m_hDC );

	// Create a secondary render context for loading thread.
	m_hRC2 = wglCreateContext( m_hDC );

	bool result = wglShareLists( m_hRC, m_hRC2 );
	if( !result )
	{
		LPVOID lpMsgBuf;
		DWORD dw = GetLastError(); 

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		AppDebugOut("Failed to Share Lists: %s.\n", lpMsgBuf );
	}

	wglMakeCurrent( m_hDC, m_hRC );

	glClear(GL_COLOR_BUFFER_BIT);
	return true;
}


void WindowManagerWin32::DisableOpenGL()
{
	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( m_hRC );

	ReleaseDC( m_hWnd, m_hDC );
	m_hRC = NULL;
}

void WindowManagerWin32::ListAllDisplayModes()
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

bool WindowManagerWin32::CreateWin(int _width, int _height, bool _windowed,
	           		   			   int _colourDepth, int _refreshRate, int _zDepth, bool _waitVRT,
	           		   			   bool _antiAlias, const wchar_t *_title)
{
	m_screenW = _width;
	m_screenH = _height;
    m_windowed = _windowed;

	// Register window class
	WNDCLASSW wc;
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hInstance;
	wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = _title;
	RegisterClassW( &wc );

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
		windowStyle |= WS_POPUP;

		DEVMODE devmode;
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmBitsPerPel = _colourDepth;
		devmode.dmPelsWidth = _width;
		devmode.dmPelsHeight = _height;
		devmode.dmDisplayFrequency = _refreshRate;
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		long result = ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);
        if( result != DISP_CHANGE_SUCCESSFUL ) return false;
        
//		AppReleaseAssert(result == DISP_CHANGE_SUCCESSFUL, "Couldn't set full screen mode of %dx%d", 
//														_width, _height);
//      This assert goes off on many systems, regardless of success

        posX = 0;
		posY = 0;
		m_borderWidth = 1;
		m_titleHeight = 0;
	}

	// Create main window
	m_hWnd = CreateWindowW( 
		wc.lpszClassName, wc.lpszClassName, 
		windowStyle,
		posX, posY, _width, _height,
		NULL, NULL, g_hInstance, NULL );

    if( !m_hWnd ) return false;

	m_mouseOffsetX = INT_MAX;

	// Enable OpenGL for the window
	EnableOpenGL(_colourDepth, _zDepth);

    return true;
}


void WindowManagerWin32::DestroyWin()
{
	DisableOpenGL();
	DestroyWindow(m_hWnd);
}


PlatformWindow *WindowManagerWin32::Window()
{
	return (PlatformWindow *)m_hWnd;
}


void WindowManagerWin32::Flip()
{
	SwapBuffers(m_hDC);
}


void WindowManagerWin32::NastyPollForMessages()
{
	MSG msg;
	while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) 
	{
		// handle or dispatch messages
		TranslateMessage( &msg );
		DispatchMessageW( &msg );
	} 
}


void WindowManagerWin32::NastySetMousePos(int x, int y)
{
	if (m_mouseOffsetX == INT_MAX)
	{
		RECT rect1;
		GetWindowRect(m_hWnd, &rect1);

		//RECT rect2;
		//GetClientRect(m_hWnd, &rect2);

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


void WindowManagerWin32::NastyMoveMouse(int x, int y)
{
	POINT pos;
	GetCursorPos( &pos );
	SetCursorPos( x + pos.x, y + pos.y );

	extern W32InputDriver *g_win32InputDriver;
	g_win32InputDriver->SetMousePosNoVelocity(x, y);
}


bool WindowManagerWin32::Captured()
{
	return m_mouseCaptured;
}

void WindowManagerWin32::CaptureMouse()
{
	SetCapture(m_hWnd);

	m_mouseCaptured = true;
}

void WindowManagerWin32::UncaptureMouse()
{
	ReleaseCapture();
	m_mouseCaptured = false;
}

void WindowManagerWin32::HideMousePointer()
{
	ShowCursor(false);
	m_mousePointerVisible = false;
}


void WindowManagerWin32::UnhideMousePointer()
{
	ShowCursor(true);
	m_mousePointerVisible = true;
}


void WindowManagerWin32::OpenWebsite( const char *_url )
{
    ShellExecute(NULL, "open", _url, NULL, NULL, SW_SHOWNORMAL); 
}


int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE _hPrevInstance, 
				   LPSTR _cmdLine, int _iCmdShow)
{
	//_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF /*| _CRTDBG_CHECK_ALWAYS_DF */| _CRTDBG_DELAY_FREE_MEM_DF);

	// uncomment to see new/malloc leaks in debug output
	//_CrtSetDbgFlag( (_CrtSetDbgFlag( _CRTDBG_REPORT_FLAG )|_CRTDBG_LEAK_CHECK_DF)&~_CRTDBG_CHECK_CRT_DF );
	// uncomment and set to catch leak with given alloc id
	//_crtBreakAlloc = 110;

	g_hInstance = _hInstance;

	g_windowManager	= new WindowManagerWin32();

	AppMain( (char *) _cmdLine );

	return WM_QUIT;
}
#endif // !defined USE_DIRECT3D
