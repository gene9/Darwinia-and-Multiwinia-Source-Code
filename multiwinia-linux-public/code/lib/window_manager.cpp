#include "lib/universal_include.h"

#include <limits.h>

#include "lib/debug_utils.h"

#include "lib/input/input.h"
#include "lib/window_manager.h"
#include "lib/work_queue.h"


WindowManager *g_windowManager = NULL;


WindowManager::WindowManager()
:	m_mousePointerVisible(true),
	m_mouseOffsetX(INT_MAX),
	m_mouseCaptured(false)
{
	AbstractListAllDisplayModes();
}


void WindowManager::AbstractListAllDisplayModes()
{
	ListAllDisplayModes();
}


void WindowManager::ListAllDisplayModes()
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


bool WindowManager::Captured()
{
	return m_mouseCaptured;
}

void WindowManager::WindowMoved()
{
	m_mouseOffsetX = INT_MAX;
}

void WindowManager::EnsureMouseCaptured()
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

 	if (!m_mouseCaptured)
 		CaptureMouse();
}

void WindowManager::EnsureMouseUncaptured()
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

 	if (m_mouseCaptured)
 		UncaptureMouse();
}

static int WindowManagerGetClosestRefreshRate( Resolution *res, int _refresh )
{
    int closestRefresh = -1;
    for (int j = 0; j < res->m_refreshRates.Size(); ++j)
    {
        int ref = res->m_refreshRates[j];
        if (ref == _refresh)
        {
            return ref;
        }
        else if (closestRefresh == -1 || abs(ref - _refresh) < abs(closestRefresh - _refresh) )
        {
            closestRefresh = ref;
        }
    }

    if (closestRefresh != -1)
        return closestRefresh;
    else
        return _refresh;
}

void WindowManager::GetClosestResolution( int *_width, int *_height, int *_refresh )
{
    Resolution *closestRes = NULL;
    for (int i = 0; i < m_resolutions.Size(); ++i)
    {
        Resolution *res = m_resolutions[i];
        if (res->m_width == *_width && res->m_height == *_height)
        {
            *_refresh = WindowManagerGetClosestRefreshRate( res, *_refresh );
            return;
        }
        else if (closestRes == NULL ||
                 ( abs(res->m_height - *_height) + abs(res->m_width - *_width) ) <
                 ( abs(closestRes->m_height - *_height) + abs(closestRes->m_width - *_width) ) )
        {
            closestRes = res;
        }
    }

    if (closestRes != NULL)
    {
        *_width = closestRes->m_width;
        *_height = closestRes->m_height;
        *_refresh = WindowManagerGetClosestRefreshRate( closestRes, *_refresh );
    }
}
