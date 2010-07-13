#include "lib/debug_utils.h"

#include "lib/input/inputdriver_sdl_mouse.h"
#include "lib/input/sdl_eventhandler.h"
#include "lib/window_manager_sdl.h"
#include <SDL/SDL.h>

using std::string;

// mouse axes and buttons
enum { X, Y, Z };
enum { LEFT, RIGHT, MIDDLE };

enum MouseControl {
	MOUSE_LEFTBUTTON,
	MOUSE_RIGHTBUTTON,
	MOUSE_MIDBUTTON,
	MOUSE_WHEEL,
	MOUSE_MOVEMENT,
	MOUSE_ANY
};


SDLMouseInputDriver::SDLMouseInputDriver()
{
	setName("SDLMouse");
	getSDLEventHandler()->AddEventProcessor(this);
	
	memset(m_mb, 0, sizeof(m_mb));
	memset(m_mbOld, 0, sizeof(m_mbOld));
	memset(m_mbDeltas, 0, sizeof(m_mbDeltas));
	
	memset(m_mousePos, 0, sizeof(m_mousePos));
	memset(m_mousePosOld, 0, sizeof(m_mousePosOld));
	memset(m_mouseVel, 0, sizeof(m_mouseVel));
}

int SDLMouseInputDriver::HandleSDLEvent(const SDL_Event & event)
{
	switch (event.type)
	{
		case SDL_MOUSEMOTION:
		{
			WindowManagerSDL *wm = static_cast<WindowManagerSDL*>(g_windowManager);
			
			m_mousePos[X] += event.motion.xrel;
			m_mousePos[Y] += event.motion.yrel;
			
			// We can't seem to uncapture the mouse in fullscreen mode once it has
			// been captured. So we must clip the mouse coords to the screen ourselves.
			if (!wm->m_mouseCaptured)
			{
				m_mousePos[X] = max(0, min(wm->m_screenW, m_mousePos[X]));
				m_mousePos[Y] = max(0, min(wm->m_screenH, m_mousePos[Y]));
			}
			
			return 0;
		}
		
		case SDL_MOUSEBUTTONDOWN:
		{
			switch (event.button.button)
			{
				case SDL_BUTTON_LEFT:
					m_mb[LEFT] = true;
					break;
					
				case SDL_BUTTON_MIDDLE:
					m_mb[MIDDLE] = true;
					break;
				
				case SDL_BUTTON_RIGHT:
					m_mb[RIGHT] = true;
					break;
					
				case SDL_BUTTON_WHEELUP:
					m_mousePos[Z]++;
					break;
					
				case SDL_BUTTON_WHEELDOWN:
					m_mousePos[Z]--;
					break;
			}
			return 0;
		}
			
		case SDL_MOUSEBUTTONUP:
		{
			switch (event.button.button)
			{
				case SDL_BUTTON_LEFT:
					m_mb[LEFT] = false;
					break;
					
				case SDL_BUTTON_MIDDLE:
					m_mb[MIDDLE] = false;
					break;
				
				case SDL_BUTTON_RIGHT:
					m_mb[RIGHT] = false;
					break;
			}
			return 0;
		}
	}
	
	return -1; // unhandled
}

void SDLMouseInputDriver::Advance()
{
	for ( unsigned i = 0; i < NUM_MB; ++i ) {
		m_mbDeltas[i] = ( m_mb[i] ? 1 : 0 ) - ( m_mbOld[i] ? 1 : 0 );
		m_mbOld[i] = m_mb[i];
	}

	for ( unsigned i = 0; i < NUM_AXES; ++i) {
		m_mouseVel[i] = m_mousePos[i] - m_mousePosOld[i];
		m_mousePosOld[i] = m_mousePos[i];
	}
}

void SDLMouseInputDriver::PollForEvents()
{
	// not implemented yet
}

bool SDLMouseInputDriver::acceptDriver( string const &name )
{
	return ( name == "mouse" );
}

control_id_t SDLMouseInputDriver::getControlID( string const &name )
{
	if ( name == "axes" )   return MOUSE_MOVEMENT;
	if ( name == "left" )   return MOUSE_LEFTBUTTON;
	if ( name == "right" )  return MOUSE_RIGHTBUTTON;
	if ( name == "middle" ) return MOUSE_MIDBUTTON;
	if ( name == "wheel" )  return MOUSE_WHEEL;
	if ( name == "any" )    return MOUSE_ANY;
	
	return -1; // error
}

bool SDLMouseInputDriver::getInput( InputSpec const &spec, InputDetails &details )
{
	int button = -1;
	
	switch ( spec.control_id ) {
		case MOUSE_LEFTBUTTON:  button = LEFT; details.type = INPUT_TYPE_BOOL; break;
		case MOUSE_RIGHTBUTTON: button = RIGHT; details.type = INPUT_TYPE_BOOL; break;
		case MOUSE_MIDBUTTON:   button = MIDDLE; details.type = INPUT_TYPE_BOOL; break;
		case MOUSE_WHEEL:       details.type = INPUT_TYPE_1D; break;
		case MOUSE_MOVEMENT:    details.type = INPUT_TYPE_2D; break;
		default:                return false; // We should never get here!
	}

	if ( button >= 0 ) {
		switch ( spec.condition ) {
			case COND_DOWN:    return ( m_mbDeltas[button] ==  1 );
			case COND_UP:      return ( m_mbDeltas[button] == -1 );
			case COND_PRESSED: return ( m_mb[button] );
			default:           return false; // We should never get here!
		}
	}

	bool reading = false;
	switch ( spec.condition ) {
		case COND_READ:
			reading = true;
		case COND_MOVED:
			if ( MOUSE_MOVEMENT == spec.control_id ) {
				details.x = m_mouseVel[X];
				details.y = m_mouseVel[Y];
				return reading || ( details.x != 0 || details.y != 0 );
			} else if ( MOUSE_WHEEL == spec.control_id ) {
				details.x = m_mouseVel[Z];
				return reading || ( details.x != 0 );
			} else
				return false; // We should never get here!

		default:
			return false; // We should never get here!
	}
}

bool SDLMouseInputDriver::getInputDescription( InputSpec const &spec, InputDescription &desc )
{
	bool ret = true;
	inputtype_t type = INPUT_TYPE_BOOL;
	desc.pref = "mouse ";
	switch ( spec.control_id ) {
		case MOUSE_LEFTBUTTON:
			desc.noun = "control_mouse_left_button";
			desc.pref += "left "; break;
		case MOUSE_RIGHTBUTTON:
			desc.noun = "control_mouse_right_button";
			desc.pref += "right "; break;
		case MOUSE_MIDBUTTON:
			desc.noun = "control_mouse_mid_button";
			desc.pref += "middle "; break;
		case MOUSE_WHEEL:
			desc.noun = "control_mouse_wheel";
			desc.pref += "wheel ";
			type = INPUT_TYPE_1D; break;
		case MOUSE_MOVEMENT:
			desc.noun = "control_mouse";
			desc.pref += "axes ";
			type = INPUT_TYPE_2D; break;
		default:
			desc.noun = "control_mouse_unknown";
			desc.pref += "unknown ";
			ret = false;
	}

	std::string mouseCondition;
	if ( MOUSE_WHEEL == spec.control_id ) {
		if ( COND_MOVED == spec.condition ) {
			desc.verb = "control_verb_scroll";
			getDefaultPrefsString( spec.condition, type, mouseCondition );
			desc.pref += mouseCondition;
			return true;
		} else {
			desc.verb = "control_verb_unknown";
			desc.pref += "unknown";
			return false;
		}
	} else {
		if ( getDefaultVerb( spec.condition, type, desc.verb ) &&
			 getDefaultPrefsString( spec.condition, type, mouseCondition ) ) {
			desc.pref += mouseCondition;
			return ret;
		} else
			return false;
	}
}

inputtype_t SDLMouseInputDriver::getControlType( control_id_t control_id )
{
	switch ( control_id ) {
		case MOUSE_MOVEMENT:    return INPUT_TYPE_2D;
		case MOUSE_LEFTBUTTON:  return INPUT_TYPE_BOOL;
		case MOUSE_RIGHTBUTTON: return INPUT_TYPE_BOOL;
		case MOUSE_MIDBUTTON:   return INPUT_TYPE_BOOL;
		case MOUSE_WHEEL:       return INPUT_TYPE_1D;
		case MOUSE_ANY:         return INPUT_TYPE_BOOL;
		default:                return -1; // Should never get here!
	}
}

void SDLMouseInputDriver::SetMousePosNoVelocity( int _x, int _y )
{
	// Warp Mouse without velocity
	m_mousePosOld[X] = m_mousePos[X] = _x;
	m_mousePosOld[Y] = m_mousePos[Y] = _y;
	m_mouseVel[X] = 0;
	m_mouseVel[Y] = 0;
}


SDLMouseInputDriver::~SDLMouseInputDriver()
{
	getSDLEventHandler()->RemoveEventProcessor(this);
}
