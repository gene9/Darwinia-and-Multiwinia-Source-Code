#ifndef __INPUT_MANAGER_SDL__
#define __INPUT_MANAGER_SDL__

#include "input.h"

class InputManagerSDL : public InputManager 
{
public:
	InputManagerSDL();

	void	ResetWindowHandle();
    void    BindAltTab();
    void    UnbindAltTab();
    
	void	WaitEvent		();
	int 	EventHandler	(unsigned int _eventId, unsigned int wParam, int lParam, bool _isAReplayedEvent = false);
	
protected:
	bool m_rawRmb;
	bool m_emulatedRmb;
	bool m_shouldEmulateRmb;

	void ChangeWindowHasFocus(bool _newWindowHasFocus);
	
private:
	bool m_xineramaOffsetHack;
}; 

#endif //  __INPUT_MANAGER_SDL__
