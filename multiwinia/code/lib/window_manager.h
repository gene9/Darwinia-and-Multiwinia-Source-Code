#ifndef INCLUDED_WINDOW_MANAGER_H
#define INCLUDED_WINDOW_MANAGER_H


#include <lib/tosser/llist.h>

typedef void* PlatformWindow;

class WorkQueue;

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
	bool		m_mousePointerVisible;
	bool		m_invertY;	// Whether the Y coordinate needs to be inverted or not.

protected:
	int			m_mouseOffsetX;
	int			m_mouseOffsetY;

    bool        m_windowed;
	bool        m_mouseCaptured;
	int			m_screenW;
	int			m_screenH;
	
	int         m_desktopScreenW;           // Original starting values
    int         m_desktopScreenH;           // Original starting values
    int         m_desktopColourDepth;       // Original starting values
    int         m_desktopRefresh;           // Original starting values

	virtual void ListAllDisplayModes();
	
private:
	void AbstractListAllDisplayModes();

public:
	WindowManager();
	~WindowManager();

	int GetResolutionId(int _width, int _height); // Returns -1 if resolution doesn't exist
    Resolution *GetResolution( int _id );
	virtual void SuggestDefaultRes( int *_width, int *_height, int *_refresh, int *_depth );

	virtual bool CreateWin(int _width, int _height,		                // Set _colourDepth, _refreshRate and/or 
						   bool _windowed, int _colourDepth,		    // _zDepth to -1 to get default values
						   int _refreshRate, int _zDepth,  bool _waitVRT,
						   bool _antiAlias, const wchar_t *_title) = 0;

	virtual void DestroyWin() = 0;
	virtual PlatformWindow *Window() = 0;
	virtual void Flip() = 0;
	virtual void NastySetMousePos(int x, int y) = 0;
	virtual void NastyMoveMouse(int x, int y) = 0;
	virtual void NastyPollForMessages() = 0;

	virtual void EnsureMouseCaptured();
	virtual void EnsureMouseUncaptured();

	virtual void CaptureMouse() = 0;
	virtual void UncaptureMouse() = 0;

	virtual void HideMousePointer() = 0;
	virtual void UnhideMousePointer() = 0;

    bool Windowed();
	bool Captured();
    virtual void OpenWebsite( const char *_url ) = 0;

	void WindowMoved();
    virtual void GetClosestResolution( int *_width, int *_height, int *_refresh );
};

extern WindowManager *g_windowManager;


#endif
