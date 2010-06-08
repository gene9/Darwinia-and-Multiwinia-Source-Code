#ifndef _included_reallyquit_window_h
#define _included_reallyquit_window_h

#include "interface/darwinia_window.h"

#define REALLYQUIT_WINDOWNAME "Really Quit?"

class ReallyQuitWindow : public DarwiniaWindow {
public:
	ReallyQuitWindow();
	void Create();
};

#endif // _included_reallyquit_window_h