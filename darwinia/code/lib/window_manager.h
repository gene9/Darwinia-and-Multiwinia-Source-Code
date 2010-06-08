#ifndef INCLUDED_WINDOW_MANAGER_H
#define INCLUDED_WINDOW_MANAGER_H


#include <lib/llist.h>


class WindowManagerWin32;


// ****************************************************************************
// Class Resolution
// ****************************************************************************

class Resolution
{
public:
	int			m_width;
	int			m_height;
	LList <int>	m_refreshRates;

	Resolution(int _width, int _height): m_width(_width), m_height(_height) {}
};


// ****************************************************************************
// Class WindowManager
// ****************************************************************************

class WindowManager
{
public:
	LList		<Resolution *> m_resolutions;
	WindowManagerWin32 *m_win32Specific;
	bool		m_mousePointerVisible;
	bool		m_invertY;	// Whether the Y coordinate needs to be inverted or not.

protected:
	int			m_screenW;	// Cached values. Use Renderer::ScreenW() if you
	int			m_screenH;	// want a value to inspect.
    bool        m_windowed; //
	bool        m_mouseCaptured;
	bool		m_waitVRT;

	int			m_mouseOffsetX;
	int			m_mouseOffsetY;

	int			m_borderWidth;
	int			m_titleHeight;

    int         m_desktopScreenW;           // Original starting values
    int         m_desktopScreenH;           // Original starting values
    int         m_desktopColourDepth;       // Original starting values
    int         m_desktopRefresh;           // Original starting values

	void ListAllDisplayModes();
	bool EnableOpenGL(int _colourDepth, int _zDepth);
	void DisableOpenGL();
	
public:
	WindowManager();
	~WindowManager();

	int GetResolutionId(int _width, int _height); // Returns -1 if resolution doesn't exist
    Resolution *GetResolution( int _id );

	bool CreateWin(int _width, int _height,		                // Set _colourDepth, _refreshRate and/or 
		           bool _windowed, int _colourDepth,		    // _zDepth to -1 to get default values
		           int _refreshRate, int _zDepth, 
				   bool _waitVRT);

	void DestroyWin();
	void Flip();
	void NastyPollForMessages();
	void NastySetMousePos(int x, int y);
	void NastyMoveMouse(int x, int y);

	void EnsureMouseCaptured();
	void EnsureMouseUncaptured();

	void CaptureMouse();
	void UncaptureMouse();

	void HideMousePointer();
	void UnhideMousePointer();

    bool Windowed();
	bool Captured();
	bool MouseVisible();

    void SaveDesktop();
    void RestoreDesktop();

	void WindowMoved();

    void SuggestDefaultRes( int *_width, int *_height, int *_refresh, int *_depth );

    static void OpenWebsite( char *_url );
};


void AppMain();

extern WindowManager *g_windowManager;


#endif
