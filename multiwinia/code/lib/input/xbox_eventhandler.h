#ifndef INCLUDED_XBOX_EVENTHANDLER_H
#define INCLUDED_XBOX_EVENTHANDLER_H

#include <vector>
#include "lib/input/eventhandler.h"


class XboxEventHandler : public EventHandler 
{
public:
	XboxEventHandler();

	bool WindowHasFocus();
};


XboxEventHandler *getXboxEventHandler();


#endif // INCLUDED_W32_EVENTHANDLER_H
