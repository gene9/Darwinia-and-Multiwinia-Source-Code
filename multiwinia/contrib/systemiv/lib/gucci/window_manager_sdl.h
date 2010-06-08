#ifndef INCLUDED_WINDOW_MANAGER_SDL_H
#define INCLUDED_WINDOW_MANAGER_SDL_H

#include "window_manager.h"

class WindowManagerSDL : public WindowManager
{
public:
	WindowManagerSDL();
	~WindowManagerSDL();

	bool        CreateWin           (int _width, int _height,		       // Set _colourDepth, _refreshRate and/or 
			                         bool _windowed, int _colourDepth,	   // _zDepth to -1 to get default values
			                         int _refreshRate, int _zDepth,
									 int _antiAlias,
		                             const char *_title );
    
	void		HideWin				();
	void        DestroyWin          ();
	void        Flip                ();
	void        PollForMessages     ();
	void        SetMousePos         (int x, int y);
    
	void        CaptureMouse        ();
	void        UncaptureMouse      ();
    
	void        HideMousePointer    ();
	void        UnhideMousePointer  ();
    
    void        SaveDesktop         ();
    void        RestoreDesktop      ();
    void        OpenWebsite( const char *_url );

protected:
	void        ListAllDisplayModes ();
	bool		m_tryingToCaptureMouse;
};

#endif
