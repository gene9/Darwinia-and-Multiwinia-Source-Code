#include "lib/universal_include.h"

#include <windows.h>
#include <string>

#include <eclipse.h>

#include "lib/input/input_types.h"
#include "lib/input/control_bindings.h"
#include "lib/input/win32_eventhandler.h"
#include "lib/input/inputdriver_win32.h"
#include "lib/input/keynames.h"

#include "lib/window_manager.h"
#include "lib/window_manager_win32.h"
#include "lib/language_table.h"
#include "app.h"
#include "lib/debug_utils.h"

using namespace std;

#define X 0
#define Y 1
#define Z 2

#define L 0
#define R 1
#define M 2


enum W32Drivers {
	NO_DRIVER,
	KEY_DRIVER,
	MOUSE_DRIVER
};


enum MouseControl {
	MOUSE_LEFTBUTTON,
	MOUSE_RIGHTBUTTON,
	MOUSE_MIDBUTTON,
	MOUSE_WHEEL,
	MOUSE_MOVEMENT,
	MOUSE_ANY
};


signed char g_keyDeltas[KEY_MAX];
signed char g_keys[KEY_MAX];

W32InputDriver *g_win32InputDriver = NULL;

W32InputDriver::W32InputDriver()
{
	setName( "W32" );

	memset(m_mb, 0, sizeof(bool) * NUM_MB);
	memset(m_mbOld, 0, sizeof(bool) * NUM_MB);
	memset(m_mbDeltas, 0, sizeof(int) * NUM_MB);
	
	memset(m_mousePos, 0, sizeof(int) * NUM_AXES);
	memset(m_mousePosOld, 0, sizeof(int) * NUM_AXES);
	memset(m_mouseVel, 0, sizeof(int) * NUM_AXES);

	memset(g_keys, 0, KEY_MAX);
	memset(g_keyDeltas, 0, KEY_MAX);
	memset(m_keyNewDeltas, 0, KEY_MAX);

	getW32EventHandler()->AddEventProcessor( this );

	g_win32InputDriver = this;
}


W32InputDriver::~W32InputDriver()
{
	getW32EventHandler()->RemoveEventProcessor( this );
}


bool W32InputDriver::getInput( InputSpec const &spec, InputDetails &details )
{

	switch ( spec.handler_id ) {
		case KEY_DRIVER:   return getKeyInput( spec, details );
		case MOUSE_DRIVER: return getMouseInput( spec, details );
		default:           return false; // Should never get here!
	}

}


bool W32InputDriver::getKeyInput( InputSpec const &spec, InputDetails &details )
{

	int button = spec.control_id;
	DarwiniaDebugAssert( 0 <= button && button < KEY_MAX );
	details.type = INPUT_TYPE_BOOL;

	switch ( spec.condition ) {
		case COND_DOWN:    return ( g_keyDeltas[button] ==  1 );
		case COND_UP:      return ( g_keyDeltas[button] == -1 );
		case COND_PRESSED: return ( g_keys[button] == 1 );
		default:           return false; // We should never get here!
	}

}


bool W32InputDriver::getMouseInput( InputSpec const &spec, InputDetails &details )
{

	int button = -1;

	switch ( spec.control_id ) {
		case MOUSE_LEFTBUTTON:  button = L; details.type = INPUT_TYPE_BOOL; break;
		case MOUSE_RIGHTBUTTON: button = R; details.type = INPUT_TYPE_BOOL; break;
		case MOUSE_MIDBUTTON:   button = M; details.type = INPUT_TYPE_BOOL; break;
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


bool W32InputDriver::isIdle()
{
	signed char keysZero[KEY_MAX];
	bool mbZero[NUM_MB];
	int maZero[NUM_AXES];
	memset( keysZero, 0, KEY_MAX );
	memset( mbZero, 0, sizeof(bool) * NUM_MB );
	memset( maZero, 0, sizeof(int) * NUM_AXES );

	return memcmp( keysZero, g_keys, KEY_MAX ) == 0 &&
		memcmp( keysZero, g_keyDeltas, KEY_MAX ) == 0 &&
		memcmp( mbZero, m_mb, sizeof(bool) * NUM_MB ) == 0 &&
		memcmp( maZero, m_mouseVel, sizeof(int) * NUM_AXES ) == 0;
}


InputMode W32InputDriver::getInputMode()
{
	return INPUT_MODE_KEYBOARD;
}


void W32InputDriver::SetMousePosNoVelocity( int _x, int _y )
{
	// Warp Mouse without velocity
	m_mousePosOld[X] = m_mousePos[X] = _x;
	m_mousePosOld[Y] = m_mousePos[Y] = _y;
	m_mouseVel[X] = 0;
	m_mouseVel[Y] = 0;
}


bool W32InputDriver::getFirstActiveInput( InputSpec &spec, bool instant )
{
	// Check for pressed mouse buttons
	//for ( unsigned i = 0; i < NUM_MB; ++i ) {
	//	if ( 1 == m_mbDeltas[i] ) { // Mouse button pressed
	//		spec.handler_id = MOUSE_DRIVER;
	//		switch ( i ) {
	//			case L: spec.control_id = MOUSE_LEFTBUTTON; break;
	//			case R: spec.control_id = MOUSE_RIGHTBUTTON; break;
	//			case M: spec.control_id = MOUSE_MIDBUTTON; break;
	//			default: return false; // We should never get here!
	//		}
	//		spec.condition = COND_DOWN;
	//		spec.type = INPUT_TYPE_BOOL;
	//		return true;
	//	}
	//}

	// Check for pressed keys
	for ( unsigned i = 0; i < KEY_TILDE; ++i ) {
		if ( 1 == g_keyDeltas[ i ] && // key was pressed
		     strstr( getKeyNames()[ i ], " " ) == NULL ) { // Key is bindable
			spec.handler_id = KEY_DRIVER;
			spec.control_id = i;
			if ( instant )
				spec.condition = COND_DOWN;
			else
				spec.condition = COND_PRESSED;
			spec.type = INPUT_TYPE_BOOL;
			return true;
		}
	}

	// We found no active inputs
	return false;
	
} // End of getFirstActiveInput


void W32InputDriver::Advance()
{
	PollForEvents();
	memcpy(g_keyDeltas, m_keyNewDeltas, KEY_MAX);
	memset(m_keyNewDeltas, 0, KEY_MAX);

	for ( unsigned i = 0; i < NUM_MB; ++i ) {
		m_mbDeltas[i] = ( m_mb[i] ? 1 : 0 ) - ( m_mbOld[i] ? 1 : 0 );
		m_mbOld[i] = m_mb[i];
	}

	for ( unsigned i = 0; i < NUM_AXES; ++i) {
		m_mouseVel[i] = m_mousePos[i] - m_mousePosOld[i];
		m_mousePosOld[i] = m_mousePos[i];
	}

}


void W32InputDriver::PollForEvents()
{
	g_windowManager->NastyPollForMessages();
}


LRESULT CALLBACK W32InputDriver::WndProc( HWND hWnd, UINT message,
                                          WPARAM wParam, LPARAM lParam )
{

	switch (message) 
	{

		case WM_LBUTTONDOWN:
			m_mb[L] = true;
			g_windowManager->CaptureMouse();
			break;
		case WM_LBUTTONUP:
			m_mb[L] = false;
			g_windowManager->UncaptureMouse();
			break;
		case WM_MBUTTONDOWN:
			m_mb[M] = true;
			g_windowManager->CaptureMouse();
			break;
		case WM_MBUTTONUP:
			m_mb[M] = false;
			g_windowManager->UncaptureMouse();
			break;
		case WM_RBUTTONDOWN:
			m_mb[R] = true;
			g_windowManager->CaptureMouse();
			break;
		case WM_RBUTTONUP:
			m_mb[R] = false;
			g_windowManager->UncaptureMouse();
			break;

		case 0x020A: //case WM_MOUSEWHEEL:
		{
			int move = (short)HIWORD(wParam) / 120;
			m_mousePos[Z] += move;
			break;
		}
        
		case WM_MOUSEMOVE:
		{
			short newPosX = lParam & 0xFFFF;
			short newPosY = short(lParam >> 16);
			m_mousePos[X] = newPosX;
			m_mousePos[Y] = newPosY;
			break;
		}

		case WM_HOTKEY:
			if (wParam == 0)
			{
				g_keys[KEY_TAB] = 1;
				m_keyNewDeltas[KEY_TAB] = 1;
			}
			break;
			
		case WM_SYSKEYUP:
			if (wParam == KEY_ALT)
			{
				// ALT pressed
				g_keys[wParam] = 0;
			}
			else
			{
				g_keys[wParam] = 0;
			}
			break;
			
		case WM_SYSKEYDOWN:
		{
			int flags = (short)HIWORD(lParam);
			if (wParam == KEY_ALT)
			{
				g_keys[wParam] = 1;
			}
			else if (flags & KF_ALTDOWN)
			{
				m_keyNewDeltas[wParam] = 1;
				g_keys[wParam] = 1;
				if (wParam == KEY_F4)
				{
                    g_app->m_requestQuit = true;
				}
			}
			break;
		}

		case WM_KEYUP:
		{
			DarwiniaDebugAssert(wParam >= 0 && wParam < KEY_MAX);
			if (g_keys[wParam] != 0)
			{
				m_keyNewDeltas[wParam] = -1;
				g_keys[wParam] = 0;
			}
			break;
		}

		case WM_KEYDOWN:
		{
			DarwiniaDebugAssert(wParam >= 0 && wParam < KEY_MAX);

			if (g_keys[wParam] != 1)
			{
				m_keyNewDeltas[wParam] = 1;
				g_keys[wParam] = 1;
			}                
			EclUpdateKeyboard( wParam, g_keys[KEY_SHIFT] == 1, g_keys[KEY_CONTROL] == 1, g_keys[KEY_ALT] == 1 );
			break;
		}

		default:
			return -1;	// Not processed
	}

	return 0;	// Processed

} // End of WndProc


bool W32InputDriver::acceptDriver( string const &name )
{
	if ( name == "key" ) lastAcceptedDriver = KEY_DRIVER;
	else if ( name == "mouse" ) lastAcceptedDriver = MOUSE_DRIVER;
	else lastAcceptedDriver = NO_DRIVER;
	return ( lastAcceptedDriver != NO_DRIVER );
}


control_id_t W32InputDriver::getControlID( string const &name )
{
	
	switch ( lastAcceptedDriver ) {

		case KEY_DRIVER:
			return getKeyId( name );

		case MOUSE_DRIVER:
			if ( name == "axes" )   return MOUSE_MOVEMENT;
			if ( name == "left" )   return MOUSE_LEFTBUTTON;
			if ( name == "right" )  return MOUSE_RIGHTBUTTON;
			if ( name == "middle" ) return MOUSE_MIDBUTTON;
			if ( name == "wheel" )  return MOUSE_WHEEL;
			if ( name == "any" )    return MOUSE_ANY;

		default:
			return -1; // Error

	}

}


InputParserState W32InputDriver::writeExtraSpecInfo( InputSpec &spec )
{
	spec.handler_id = lastAcceptedDriver;
	return STATE_DONE;
}


inputtype_t W32InputDriver::getControlType( control_id_t control_id )
{

	switch ( lastAcceptedDriver ) {

		case KEY_DRIVER:
			return INPUT_TYPE_BOOL;

		case MOUSE_DRIVER:
			return getMouseControlType( control_id );

		default:
			return -1; // Should never get here!

	}

}


inputtype_t W32InputDriver::getMouseControlType( control_id_t control_id )
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


control_id_t W32InputDriver::getKeyId( string const &keyName )
{
	for ( control_id_t i = 0; i <= KEY_TILDE; ++i ) {
		if ( stricmp( keyName.c_str() , getKeyNames()[i] ) == 0 )
			return i;
	}

	if ( stricmp( keyName.c_str(), "any" ) == 0 ) return KEY_ANY;

	return -1;
}


bool W32InputDriver::getInputDescription( InputSpec const &spec, InputDescription &desc )
{
	if ( KEY_DRIVER == spec.handler_id ) {

		bool keyInRange = ( 0 <= spec.control_id && spec.control_id <= KEY_TILDE );
		if ( keyInRange ) {
			desc.noun = "control_";
			desc.noun += getKeyNames()[ spec.control_id ];
			if ( ISLANGUAGEPHRASE( desc.noun.c_str() ) )
				desc.noun = RAWLANGUAGEPHRASE( desc.noun.c_str() );
			else
				desc.noun = getKeyNames()[ spec.control_id ];
		} else
			desc.noun = "control_key_unknown";

		std::string prefCond;
		if ( getDefaultVerb( spec.condition, INPUT_TYPE_BOOL, desc.verb ) &&
		     getDefaultPrefsString( spec.condition, INPUT_TYPE_BOOL, prefCond ) ) {
			desc.pref = "key ";
			desc.pref += desc.noun + " " + prefCond;
			return keyInRange;
		} else {
			desc.pref = "unknown_key_pref_string";
			return false;
		}

	
	} else if ( MOUSE_DRIVER == spec.handler_id ) {
	
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

	} else { // Shouldn't be here!
		desc.noun = "control_driver_unknown";
		desc.verb = "control_driver_unknown";
		desc.pref = "control_driver_unknown";
		return false;
	}
}
