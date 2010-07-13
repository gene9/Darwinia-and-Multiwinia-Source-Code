#include "sdl_eventhandler.h"
#include <SDL/SDL.h>
#include "app.h"
#include <iostream>
#include <algorithm>

typedef std::vector<SDLEventProcessor *>::iterator ProcIt;

bool SDLEventHandler::WindowHasFocus()
{
	return (SDL_GetAppState() | SDL_APPACTIVE) != 0;
}

int SDLEventHandler::HandleSDLEvent(const SDL_Event & event)
{
	
	
	switch (event.type)
	{
		case SDL_QUIT:
			g_app->m_requestQuit = true;
			break;
	}
	
	int ans = -1;
	for ( ProcIt i = eventProcessors.begin(); i != eventProcessors.end(); ++i ) {
		ans = (*i)->HandleSDLEvent( event );
		if ( ans != -1 ) break;
	}
	return ans;
}

void SDLEventHandler::AddEventProcessor( SDLEventProcessor *_driver )
{
	if ( _driver )
		eventProcessors.push_back( _driver );
	else
		std::cerr << "AddEventProcessor: _driver is NULL\n" << std::endl;
}

void SDLEventHandler::RemoveEventProcessor( SDLEventProcessor *_driver )
{
	std::remove( eventProcessors.begin(), eventProcessors.end(), _driver );
}

SDLEventHandler *getSDLEventHandler()
{
	return dynamic_cast<SDLEventHandler *>(g_eventHandler);
}
