#ifndef INCLUDED_SDLEVENTPROC_H
#define INCLUDED_SDLEVENTPROC_H
#include <SDL/SDL.h>

class SDLEventProcessor
{
	public:
		virtual int HandleSDLEvent(const SDL_Event& event) = 0;
};

#endif

