#ifndef INCLUDED_INPUTMANAGER_WIN32_H
#define INCLUDED_INPUTMANAGER_WIN32_H

#include "input.h"

class InputManagerWin32 : public InputManager
{
public:
    InputManagerWin32		();

	void	ResetWindowHandle();
    void    BindAltTab();
    void    UnbindAltTab();
    
	void	WaitEvent		();
	void	SetMousePos		(int x, int y);
	int 	EventHandler	(unsigned int _eventId, unsigned int wParam, int lParam, bool _isAReplayedEvent = false);
};

#endif // INCLUDED_INPUTMANAGER_WIN32_H
