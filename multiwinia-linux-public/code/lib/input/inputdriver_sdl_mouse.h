#ifndef INPUTDRIVER_SDL_MOUSE
#define INPUTDRIVER_SDL_MOUSE

#include "lib/input/inputdriver_simple.h"
#include "lib/input/sdl_eventproc.h"
#include <string>
#include <SDL/SDL.h>

static const int NUM_MB = 3;
static const int NUM_AXES = 3;

class SDLMouseInputDriver : public SimpleInputDriver, SDLEventProcessor
{
	public:
		SDLMouseInputDriver();
		~SDLMouseInputDriver();
		
		void Advance();
		void PollForEvents();
		int HandleSDLEvent(const SDL_Event & event);
		bool acceptDriver( std::string const &name );
		control_id_t getControlID( std::string const &name );
		bool getInput( InputSpec const &spec, InputDetails &details );
		inputtype_t getControlType( control_id_t control_id );
		bool getInputDescription( InputSpec const &spec, InputDescription &desc );
		void SetMousePosNoVelocity( int _x, int _y );
		
	protected:
		bool        m_mb[NUM_MB];            // Mouse button states from this frame
		bool        m_mbOld[NUM_MB];         // Mouse button states from last frame
		int         m_mbDeltas[NUM_MB];      // Mouse button diffs

		int         m_mousePos[NUM_AXES];    // X Y Z
		int         m_mousePosOld[NUM_AXES]; // X Y Z
		int         m_mouseVel[NUM_AXES];    // X Y Z
};

#endif