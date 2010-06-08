#ifndef INCLUDED_W32_EVENTHANDLER_H
#define INCLUDED_W32_EVENTHANDLER_H

#include <vector>

#include "lib/input/eventhandler.h"
#include "lib/input/inputdriver_win32.h"
#include "lib/input/win32_eventproc.h"


class W32EventHandler : public EventHandler, public W32EventProcessor {

private:
	std::vector <W32EventProcessor *>w32eventprocs;

public:
	W32EventHandler();

	// Called by the WindowManager
	LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

	// Register driver for Windows callbacks
	void AddEventProcessor( W32EventProcessor *_driver );

	// Unregister driver (if it is still the registered one)
	void RemoveEventProcessor( W32EventProcessor *_driver );

	void ResetWindowHandle();

	void BindAltTab();

	void UnbindAltTab();

	bool WindowHasFocus();

};


W32EventHandler *getW32EventHandler();


#endif // INCLUDED_W32_EVENTHANDLER_H
