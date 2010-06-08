#include "lib/universal_include.h"

#include <limits.h>

#include "lib/debug_utils.h"

#include "input.h"
#include "window_manager.h"


WindowManager *g_windowManager = NULL;


WindowManager::WindowManager()
:	m_mousePointerVisible(true),
	m_mouseOffsetX(INT_MAX),
	m_mouseCaptured(false),
    m_secondaryMessageHandler(NULL)
{
}


WindowManager::~WindowManager()
{
}


void WindowManager::SuggestDefaultRes( int *_width, int *_height, int *_refresh, int *_depth )
{
    *_width = m_desktopScreenW;
    *_height = m_desktopScreenH;
    *_refresh = m_desktopRefresh;
    *_depth = m_desktopColourDepth;
}


// Returns an index into the list of already registered resolutions
int WindowManager::GetResolutionId(int _width, int _height)
{
	for (int i = 0; i < m_resolutions.Size(); ++i)
	{
		WindowResolution *res = m_resolutions[i];
		if (res->m_width == _width && res->m_height == _height)
		{
			return i;
		}
	}

	return -1;
}


WindowResolution *WindowManager::GetResolution( int _id )
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


bool WindowManager::Captured()
{
	return m_mouseCaptured;
}


void WindowManager::RegisterMessageHandler( SecondaryEventHandler _messageHandler )
{
    if( !m_secondaryMessageHandler )
    {
        m_secondaryMessageHandler = _messageHandler;
    }
}


SecondaryEventHandler WindowManager::GetSecondaryMessageHandler()
{
    return m_secondaryMessageHandler;
}

class DeleteWindowManagerOnExit {
	public:
	~DeleteWindowManagerOnExit()
	{
		if (g_windowManager)
			g_windowManager->DestroyWin();

		delete g_windowManager;
		g_windowManager = NULL;
	}
};


static DeleteWindowManagerOnExit please;

#if defined TARGET_OS_MACOSX || TARGET_OS_LINUX
#include "window_manager_sdl.h"
#else
#include "window_manager_win32.h"
#endif 

WindowManager *WindowManager::Create()
{
#if defined TARGET_OS_MACOSX || TARGET_OS_LINUX
	return new WindowManagerSDL();
#else
	return new WindowManagerWin32();
#endif 
}


