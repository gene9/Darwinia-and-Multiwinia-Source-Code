#ifndef INCLUDED_SDL_EVENTHANDLER_H
#define INCLUDED_SDL_EVENTHANDLER_H

#include "lib/universal_include.h"

#include "lib/input/eventhandler.h"
#include "lib/input/sdl_eventproc.h"
#include <vector>

class SDLEventHandler : public EventHandler, public SDLEventProcessor
{
private:
	std::vector<SDLEventProcessor *> eventProcessors;
	
public:
	 bool WindowHasFocus();
	 int HandleSDLEvent(const SDL_Event & event);
	 
	// Register driver for SDL callbacks
	void AddEventProcessor( SDLEventProcessor *_driver );

	// Unregister driver (if it is still the registered one)
	void RemoveEventProcessor( SDLEventProcessor *_driver );
};

SDLEventHandler *getSDLEventHandler();

#endif