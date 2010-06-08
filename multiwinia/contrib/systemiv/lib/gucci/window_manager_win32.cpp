#include "lib/universal_include.h"

#include <limits.h>
#include <shellapi.h>

#include "lib/debug_utils.h"

#include "input.h"
#include "window_manager_win32.h"


static HINSTANCE g_hInstance;

#define WH_KEYBOARD_LL 13
#define LLKHF_ALTDOWN 0x20


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    SecondaryEventHandler secondaryHandler = g_windowManager->GetSecondaryMessageHandler();

	if (!g_inputManager ||
  		 g_inputManager->EventHandler(message, wParam, lParam) == -1 )
	{
        if( !secondaryHandler ||
            secondaryHandler(message, wParam, lParam) == -1 )
        {
			if( message == WM_CLOSE )
			{
				AppShutdown();
				PostQuitMessage(0);
				return 0;
			}

		    return DefWindowProc( hWnd, message, wParam, lParam );
        }
        return 0;
	}

	return 0;
}


WindowManagerWin32::WindowManagerWin32()
:	m_hWnd(NULL),
	m_hDC(NULL),
	m_hRC(NULL)

{
	AppAssert(g_windowManager == NULL);

	ListAllDisplayModes();

    SaveDesktop();
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


void WindowManagerWin32::EnableOpenGL(int _colourDepth, int _zDepth)
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
	wglMakeCurrent( m_hDC, m_hRC );

	glClear(GL_COLOR_BUFFER_BIT);
}


void WindowManagerWin32::DisableOpenGL()
{
	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( m_hRC );
	ReleaseDC( m_hWnd, m_hDC );
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
			WindowResolution *res;
			if (resId == -1)
			{
				res = new WindowResolution(devMode.dmPelsWidth, devMode.dmPelsHeight);
				
                bool added = false;
                for( int i = 0; i < m_resolutions.Size(); ++i )
                {
                    if( devMode.dmPelsWidth < m_resolutions[i]->m_width )
                    {
                        m_resolutions.PutDataAtIndex( res, i );
                        added = true;
                        break;
                    }
                }

                if( !added )
                {
                    m_resolutions.PutDataAtEnd( res );
                }
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


bool WindowManagerWin32::CreateWin(int _width, int _height, bool _border, bool _windowed,
	           				     int _colourDepth, int _refreshRate, int _zDepth,
                                 int antiAlias, const char *_title )
{
	m_screenW = _width;
	m_screenH = _height;
    m_windowed = _windowed;

	// Register window class
	WNDCLASS wc;
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hInstance;
	wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = strdup( _title );
	RegisterClass( &wc );

	int posX, posY;
	unsigned int windowStyle = 
		_border ? (WS_POPUPWINDOW | WS_VISIBLE) :
				  (WS_POPUP | WS_VISIBLE);

	if (_windowed)
	{
        RestoreDesktop();

		if( _border )
			windowStyle |= WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

		RECT windowRect = { 0, 0, _width, _height };
		AdjustWindowRect(&windowRect, windowStyle, false);
		m_borderWidth = ((windowRect.right - windowRect.left) - _width) / 2;
		m_titleHeight = ((windowRect.bottom - windowRect.top) - _height) - m_borderWidth * 2;
		_width += m_borderWidth * 2;
		_height += m_borderWidth * 2 + m_titleHeight;

		HWND desktopWindow = GetDesktopWindow();
		RECT desktopRect;
		GetWindowRect(desktopWindow, &desktopRect);
		int desktopWidth = desktopRect.right - desktopRect.left;
		int desktopHeight = desktopRect.bottom - desktopRect.top;

		posX = (desktopRect.right - _width) / 2;
		posY = (desktopRect.bottom - _height) / 2;
	}
	else
	{
        windowStyle |= WS_MAXIMIZE;
        
		DEVMODE devmode;
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmBitsPerPel = _colourDepth;
		devmode.dmPelsWidth = _width;
		devmode.dmPelsHeight = _height;
		devmode.dmDisplayFrequency = _refreshRate;
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		long result = ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);
        if( result != DISP_CHANGE_SUCCESSFUL ) return false;
        
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
	EnableOpenGL(_colourDepth, _zDepth);

    return true;
}

void WindowManagerWin32::HideWin()
{
	ShowWindow(GetWindowManagerWin32()->m_hWnd, SW_MINIMIZE);
	ShowWindow(GetWindowManagerWin32()->m_hWnd, SW_HIDE);
}

void WindowManagerWin32::DestroyWin()
{
	DisableOpenGL();
	DestroyWindow(m_hWnd);
}


void WindowManagerWin32::Flip()
{
    PollForMessages();
	SwapBuffers(m_hDC);
}


void WindowManagerWin32::PollForMessages()
{
	MSG msg;
	while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) 
	{
		// handle or dispatch messages
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	} 
}


void WindowManagerWin32::SetMousePos(int x, int y)
{
	if (m_mouseOffsetX == INT_MAX)
	{
		RECT rect1;
		GetWindowRect(m_hWnd, &rect1);

		RECT rect2;
		GetClientRect(m_hWnd, &rect2);

		//int borderWidth = (m_screenW - rect2.right) / 2;
		//int menuHeight = (m_screenH - rect2.bottom) - (borderWidth * 2);
		m_mouseOffsetX = rect1.left + m_borderWidth;
		m_mouseOffsetY = rect1.top + m_borderWidth + m_titleHeight;
	}

	SetCursorPos(x + m_mouseOffsetX, y + m_mouseOffsetY);
}

HINSTANCE WindowManagerWin32::GethInstance()
{
    return g_hInstance;
}

void WindowManagerWin32::CaptureMouse()
{
	SetCapture(m_hWnd);
	m_mouseCaptured = true;
}


void WindowManagerWin32::UncaptureMouse()
{
	ReleaseCapture();
	m_mouseCaptured = true;
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


#ifdef USE_CRASHREPORTING
#include "contrib/XCrashReport/client/exception_handler.h"
#endif

int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE _hPrevInstance, 
				   LPSTR _cmdLine, int _iCmdShow)
{
	g_hInstance = _hInstance;

#ifdef USE_CRASHREPORTING
    __try
#endif
    {
		AppMain();
    }
#ifdef USE_CRASHREPORTING
	__except( RecordExceptionInfo( GetExceptionInformation(), "WinMain", APP_NAME, APP_VERSION ) ) {}
#endif
	
	return WM_QUIT;
}

