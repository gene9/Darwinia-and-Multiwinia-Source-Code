#ifndef INCLUDED_WINDOW_MANAGER_H
#define INCLUDED_WINDOW_MANAGER_H

#include "lib/tosser/llist.h"


class WindowResolution;

typedef long (*SecondaryEventHandler )(unsigned int,unsigned int,long);

// ****************************************************************************
// Class WindowManager
// ****************************************************************************

class WindowManager
{
public:
	static WindowManager *Create();

	WindowManager();	
	virtual ~WindowManager();

	virtual bool        CreateWin           (int _width, int _height,			// Set _colourDepth, _refreshRate and/or 
				                             bool _border,						// Whether window has a border or not
											 bool _windowed, int _colourDepth,	// _zDepth to -1 to get default values
				                             int _refreshRate, int _zDepth,
											 int _antiAlias,
		                                     const char *_title ) = 0;

	virtual void		HideWin				() = 0;
	virtual void        DestroyWin          () = 0;
	virtual void        Flip                () = 0;
	virtual void        PollForMessages     () = 0;
	virtual void        SetMousePos         (int x, int y) = 0;

	virtual void        CaptureMouse        () = 0;
	virtual void        UncaptureMouse      () = 0;

	virtual void        HideMousePointer    () = 0;
	virtual void        UnhideMousePointer  () = 0;

    virtual void        OpenWebsite			( const char *_url ) = 0;

    int         		WindowW				() { return m_screenW; }
    int         		WindowH				() { return m_screenH; }    

    bool        		Windowed            ();
	bool        		Captured            ();
	bool        		MouseVisible        ();

    void        		SuggestDefaultRes   ( int *_width, int *_height, int *_refresh, int *_depth );

	int                 GetResolutionId (int _width, int _height);      // Returns -1 if resolution doesn't exist
    WindowResolution    *GetResolution  (int _id);


    void        		RegisterMessageHandler (SecondaryEventHandler _messageHandler );
    SecondaryEventHandler GetSecondaryMessageHandler();

public:
    LList		<WindowResolution *> m_resolutions;

protected:
	int			m_screenW;	
	int			m_screenH;	
	bool        m_windowed; 
	bool        m_mouseCaptured;
	bool		m_mousePointerVisible;

	int			m_mouseOffsetX;
	int			m_mouseOffsetY;

	int			m_borderWidth;
	int			m_titleHeight;

    int         m_desktopScreenW;           // Original starting values
    int         m_desktopScreenH;           // Original starting values
    int         m_desktopColourDepth;       // Original starting values
    int         m_desktopRefresh;           // Original starting values

    SecondaryEventHandler m_secondaryMessageHandler;
};


// ****************************************************************************
// Class WindowResolution
// ****************************************************************************

class WindowResolution
{
public:
	int			m_width;
	int			m_height;
	LList <int>	m_refreshRates;

	WindowResolution(int _width, int _height): 
                        m_width(_width), 
                        m_height(_height) {}
};


void AppMain();

void AppShutdown();

extern WindowManager *g_windowManager;


#endif
