#ifndef INPUTDRIVER_SDL_KEY
#define INPUTDRIVER_SDL_KEY

#include "lib/input/inputdriver_simple.h"
#include "lib/input/sdl_eventproc.h"
#include "lib/input/keydefs.h"
#include <string>

class SDLKeyboardInputDriver : public SimpleInputDriver, SDLEventProcessor
{
	public:
		SDLKeyboardInputDriver();
		~SDLKeyboardInputDriver();
		
		void Advance();
		void PollForEvents();
		int HandleSDLEvent(const SDL_Event & event);
		bool acceptDriver( std::string const &name );
		control_id_t getControlID( std::string const &name );
		bool getInput( InputSpec const &spec, InputDetails &details );
		inputtype_t getControlType( control_id_t control_id );
		bool getInputDescription( InputSpec const &spec, InputDescription &desc );
		bool getFirstActiveInput( InputSpec &spec, bool instant );
		
	protected:
		bool m_keys[KEY_MAX];
		int m_keyDeltas[KEY_MAX];
		int m_keyNewDeltas[KEY_MAX];
};

#endif