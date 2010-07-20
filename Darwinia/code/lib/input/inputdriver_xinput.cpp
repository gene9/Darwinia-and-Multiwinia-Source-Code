#include "lib/universal_include.h"

#include <math.h>
#include <sstream>

#include "lib/input/input_types.h"
#include "lib/input/control_bindings.h"
#include "lib/input/inputdriver_xinput.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"

#define XINPUT_GAMEPAD_THUMB_MAX   32768
#define XINPUT_GAMEPAD_TRIGGER_MAX   255

const float XInputDriver::SCALE_JOYSTICK = 3.4f;
const float XInputDriver::SCALE_TRIGGER  = 6.3f;
const double XInputDriver::REPEAT_FREQ   = 0.3;

inline int round(double x)
{
	return int(x > 0.0 ? x + 0.5 : x - 0.5);
}


// Must be in the same order as s_controls (below)
enum {
	XInputLeftTrigger,
	XInputRightTrigger,
	XInputLeftBumper,
	XInputRightBumper,
	XInputLeftThumbstick,
	XInputRightThumbstick,
	XInputDPadLeft,
	XInputDPadRight,
	XInputDPadUp,
	XInputDPadDown,
	XInputButtonA,
	XInputButtonB,
	XInputButtonX,
	XInputButtonY,
	XInputButtonStart,
	XInputButtonBack,

	XInputPlug,

	XInputLeftThumbstickLeft,
	XInputLeftThumbstickRight,
	XInputLeftThumbstickUp,
	XInputLeftThumbstickDown,

	XInputRightThumbstickLeft,
	XInputRightThumbstickRight,
	XInputRightThumbstickUp,
	XInputRightThumbstickDown,

	XInputNumControls
};


// Must be in the same order as the above enum
static ControlAction s_controls[] = {
	// Name                  Type
	"lefttrigger",           INPUT_TYPE_1D,
	"righttrigger",          INPUT_TYPE_1D,
	"leftbumper",            INPUT_TYPE_BOOL,
	"rightbumper",           INPUT_TYPE_BOOL,
	"leftthumbstick",        INPUT_TYPE_2D   | INPUT_TYPE_BOOL,
	"rightthumbstick",       INPUT_TYPE_2D   | INPUT_TYPE_BOOL,
	"dpadleft",              INPUT_TYPE_BOOL,
	"dpadright",             INPUT_TYPE_BOOL,
	"dpadup",                INPUT_TYPE_BOOL,
	"dpaddown",              INPUT_TYPE_BOOL,
	"a",                     INPUT_TYPE_BOOL,
	"b",                     INPUT_TYPE_BOOL,
	"x",                     INPUT_TYPE_BOOL,
	"y",                     INPUT_TYPE_BOOL,
	"start",                 INPUT_TYPE_BOOL,
	"back",                  INPUT_TYPE_BOOL,

	"plug",                  INPUT_TYPE_BOOL,

	"leftthumbstickleft",    INPUT_TYPE_BOOL,
	"leftthumbstickright",   INPUT_TYPE_BOOL,
	"leftthumbstickup",      INPUT_TYPE_BOOL,
	"leftthumbstickdown",    INPUT_TYPE_BOOL,

	"rightthumbstickleft",   INPUT_TYPE_BOOL,
	"rightthumbstickright",  INPUT_TYPE_BOOL,
	"rightthumbstickup",     INPUT_TYPE_BOOL,
	"rightthumbstickdown",   INPUT_TYPE_BOOL,

	NULL,                    NULL
};

enum Direction {
	North,
	South,
	East,
	West
};

static double s_nextrepeat[XInputNumControls];


XInputDriver::XInputDriver()
{
	setName( "XInput" );
	memset( &m_state, 0, sizeof(ControllerState) );
	memset( &m_oldState, 0, sizeof(ControllerState) );
}


bool XInputDriver::getInput( InputSpec const &spec, InputDetails &details )
{
	if ( !m_state.isConnected && !( XInputPlug == spec.control_id ) )
		return false; // The controller is not plugged in

	// No changes
	if ( m_state.state.dwPacketNumber == m_oldState.state.dwPacketNumber ) {
		switch ( spec.condition ) {
			case COND_DOWN:
			case COND_UP:
			case COND_RELEASED:
			case COND_MOVED:
				return false;
		}
	}

	switch ( spec.control_id ) {

		case XInputLeftTrigger:
		case XInputRightTrigger:
			return getTriggerInput( spec, details );

		case XInputLeftThumbstick:
			if ( COND_UP == spec.condition ||
			     COND_DOWN == spec.condition ||
			     COND_PRESSED == spec.condition ||
			     COND_REPEAT == spec.condition ) {
				return getButtonInput( XINPUT_GAMEPAD_LEFT_THUMB, spec, details );
			} else {
				return getThumbInput( spec, details );
			}
		case XInputRightThumbstick:
			if ( COND_UP == spec.condition ||
			     COND_DOWN == spec.condition ||
			     COND_PRESSED == spec.condition ||
			     COND_REPEAT == spec.condition ) {
				return getButtonInput( XINPUT_GAMEPAD_RIGHT_THUMB, spec, details );
			} else {
				return getThumbInput( spec, details );
			}

		case XInputLeftBumper:
			return getButtonInput( XINPUT_GAMEPAD_LEFT_SHOULDER, spec, details );
		case XInputRightBumper:
			return getButtonInput( XINPUT_GAMEPAD_RIGHT_SHOULDER, spec, details );
		case XInputDPadLeft:
			return getButtonInput( XINPUT_GAMEPAD_DPAD_LEFT, spec, details );
		case XInputDPadRight:
			return getButtonInput( XINPUT_GAMEPAD_DPAD_RIGHT, spec, details );
		case XInputDPadUp:
			return getButtonInput( XINPUT_GAMEPAD_DPAD_UP, spec, details );
		case XInputDPadDown:
			return getButtonInput( XINPUT_GAMEPAD_DPAD_DOWN, spec, details );
		case XInputButtonA:
			return getButtonInput( XINPUT_GAMEPAD_A, spec, details );
		case XInputButtonB:
			return getButtonInput( XINPUT_GAMEPAD_B, spec, details );
		case XInputButtonX:
			return getButtonInput( XINPUT_GAMEPAD_X, spec, details );
		case XInputButtonY:
			return getButtonInput( XINPUT_GAMEPAD_Y, spec, details );
		case XInputButtonStart:
			return getButtonInput( XINPUT_GAMEPAD_START, spec, details );
		case XInputButtonBack:
			return getButtonInput( XINPUT_GAMEPAD_BACK, spec, details );

		case XInputLeftThumbstickLeft:
			return getThumbDirection( XInputLeftThumbstick, spec, West, details );
		case XInputLeftThumbstickRight:
			return getThumbDirection( XInputLeftThumbstick, spec, East, details );
		case XInputLeftThumbstickUp:
			return getThumbDirection( XInputLeftThumbstick, spec, North, details );
		case XInputLeftThumbstickDown:
			return getThumbDirection( XInputLeftThumbstick, spec, South, details );

		case XInputRightThumbstickLeft:
			return getThumbDirection( XInputRightThumbstick, spec, West, details );
		case XInputRightThumbstickRight:
			return getThumbDirection( XInputRightThumbstick, spec, East, details );
		case XInputRightThumbstickUp:
			return getThumbDirection( XInputRightThumbstick, spec, North, details );
		case XInputRightThumbstickDown:
			return getThumbDirection( XInputRightThumbstick, spec, South, details );

		case XInputPlug:
			details.type = INPUT_TYPE_BOOL;
			if ( COND_UP == spec.condition )
				return ( !m_state.isConnected && m_oldState.isConnected );
			else if ( COND_DOWN == spec.condition )
				return ( m_state.isConnected && !m_oldState.isConnected );
			else if ( COND_PRESSED == spec.condition )
				return m_state.isConnected;

		default:
			return false; // We should never get here!

	}

} // End of getInput


bool XInputDriver::isIdle()
{
	if ( !m_state.isConnected ) return true;
	XINPUT_GAMEPAD gpZero;
	memset( &gpZero, 0, sizeof(XINPUT_GAMEPAD) );
	return memcmp( &m_state.state.Gamepad, &gpZero, sizeof(XINPUT_GAMEPAD) ) == 0;
}


InputMode XInputDriver::getInputMode()
{
	return INPUT_MODE_GAMEPAD;
}


bool XInputDriver::getButtonInput( WORD button, InputSpec const &spec, InputDetails &details )
{

	bool val, oldVal, rep;
	XINPUT_GAMEPAD const &gp = m_state.state.Gamepad;
	XINPUT_GAMEPAD const &oldGP = m_oldState.state.Gamepad;
	double now;

	val = ( ( gp.wButtons & button ) == button );
	oldVal = ( ( oldGP.wButtons & button ) == button );

	details.type = INPUT_TYPE_BOOL;

	switch ( spec.condition ) {

		case COND_DOWN:
			return ( val && !oldVal );

		case COND_PRESSED:
			return val;

		case COND_UP:
			return ( !val && oldVal );

		case COND_REPEAT:
			now = GetHighResTime();
			rep = val &&
			      ( !oldVal
			      || ( s_nextrepeat[ spec.control_id ] <= now ) );
			if ( rep )
				s_nextrepeat[ spec.control_id ] = now + REPEAT_FREQ;
			return rep;

		default:
			return false; // We should never get here!

	}

} // End of getButtonInput


bool XInputDriver::getThumbInput( InputSpec const &spec, InputDetails &details )
{

	SHORT x, y, oldX, oldY;
	XINPUT_GAMEPAD const &gp = m_state.state.Gamepad;
	XINPUT_GAMEPAD const &oldGP = m_oldState.state.Gamepad;
	double sensitivity = spec.handler_id / 100.0;

	switch ( spec.control_id ) {
		case XInputLeftThumbstick:
			x = gp.sThumbLX; y = gp.sThumbLY;
			oldX = oldGP.sThumbLX; oldY = oldGP.sThumbLY;
			break;

		case XInputRightThumbstick:
			x = gp.sThumbRX; y = gp.sThumbRY;
			oldX = oldGP.sThumbRX; oldY = oldGP.sThumbRY;
			break;

		default:
			return false; // We should never get here!
	}

	switch ( spec.condition ) {

		case COND_MOVED:
			details.type = INPUT_TYPE_2D;
			details.x = round((x - oldX) * sensitivity);
			details.y = round((y - oldY) * sensitivity);
			return ( abs( details.x ) > SCALE_JOYSTICK || abs( details.y ) > SCALE_JOYSTICK );

		case COND_ZERO:
			details.type = INPUT_TYPE_BOOL;
			return ( x == 0 && y == 0 );

		case COND_NONZERO:
			details.type = INPUT_TYPE_2D;
			details.x = round(x * sensitivity);
			details.y = round(y * sensitivity);
			return ( x != 0 || y != 0 );

		case COND_RELEASED:
			details.type = INPUT_TYPE_BOOL;
			return ( ( x == 0 && y == 0 ) &&
			         ( oldX != 0 || oldY != 0 ) );

		case COND_READ:
			details.type = INPUT_TYPE_2D;
			details.x = round(x * sensitivity);
			details.y = round(y * sensitivity);
			return true;

		default:
			return false; // We should never get here!

	}

} // End of getThumbInput


bool XInputDriver::getTriggerInput( InputSpec const &spec, InputDetails &details )
{

	BYTE val, oldVal;
	XINPUT_GAMEPAD const &gp = m_state.state.Gamepad;
	XINPUT_GAMEPAD const &oldGP = m_oldState.state.Gamepad;
	double sensitivity = spec.handler_id / 100.0;

	switch ( spec.control_id ) {
		case XInputLeftTrigger:
			val = gp.bLeftTrigger; oldVal = oldGP.bLeftTrigger;
			break;
		case XInputRightTrigger:
			val = gp.bRightTrigger; oldVal = oldGP.bRightTrigger;
			break;
		default:
			return false; // We should never get here!
	}

	val = round(val * sensitivity);
	oldVal = round(oldVal * sensitivity);

	switch ( spec.condition ) {

		case COND_NONZERO:
			details.type = INPUT_TYPE_1D;
			details.x = val;
			return ( val != 0 );

		case COND_ZERO:
			details.type = INPUT_TYPE_BOOL;
			return ( val == 0 );

		case COND_RELEASED:
			details.type = INPUT_TYPE_BOOL;
			return ( val == 0 && oldVal != 0 );

		case COND_MOVED:
			details.type = INPUT_TYPE_1D;
			details.x = val - oldVal;
			return ( details.x != 0 );

        case COND_DOWN:
            details.type = INPUT_TYPE_BOOL;
            return ( val != 0 && oldVal == 0 );

		case COND_READ:
			details.type = INPUT_TYPE_1D;
			details.x = val;
			return true;

		default:
			return false; // We should never get here!

	}

} // End of getTriggerInput


void XInputDriver::correctDeadZones( XINPUT_STATE &state )
{

	if( (state.Gamepad.sThumbLX < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
	     state.Gamepad.sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) &&
	    (state.Gamepad.sThumbLY < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
	     state.Gamepad.sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) )
	{
		state.Gamepad.sThumbLX = 0;
		state.Gamepad.sThumbLY = 0;
	}

	if( (state.Gamepad.sThumbRX < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
	     state.Gamepad.sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) &&
	    (state.Gamepad.sThumbRY < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
	     state.Gamepad.sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) )
	{
		state.Gamepad.sThumbRX = 0;
		state.Gamepad.sThumbRY = 0;
	}

	if ( state.Gamepad.bLeftTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD )
		state.Gamepad.bLeftTrigger = 0;

	if ( state.Gamepad.bRightTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD )
		state.Gamepad.bRightTrigger = 0;

} // End of correctDeadZones


void XInputDriver::scaleInputs( XINPUT_STATE &state )
{
	XINPUT_GAMEPAD &gp = state.Gamepad;

	double temp;

	temp = gp.sThumbLX * SCALE_JOYSTICK / XINPUT_GAMEPAD_THUMB_MAX;
	gp.sThumbLX = floor( temp * temp * temp );

	temp = gp.sThumbLY * SCALE_JOYSTICK / XINPUT_GAMEPAD_THUMB_MAX;
	gp.sThumbLY = -floor( temp * temp * temp );

	temp = gp.sThumbRX * SCALE_JOYSTICK / XINPUT_GAMEPAD_THUMB_MAX;
	gp.sThumbRX = floor( temp * temp * temp );

	temp = gp.sThumbRY * SCALE_JOYSTICK / XINPUT_GAMEPAD_THUMB_MAX;
	gp.sThumbRY = -floor( temp * temp * temp );


	//gp.sThumbLX = floor( gp.sThumbLX * SCALE_JOYSTICK );
	//gp.sThumbLY = -floor( gp.sThumbLY * SCALE_JOYSTICK );
	//gp.sThumbRX = floor( gp.sThumbRX * SCALE_JOYSTICK );
	//gp.sThumbRY = -floor( gp.sThumbRY * SCALE_JOYSTICK );

	gp.bLeftTrigger = floor( gp.bLeftTrigger * SCALE_TRIGGER );
	gp.bRightTrigger = floor( gp.bRightTrigger * SCALE_TRIGGER );

	//gp.sThumbLX *= gp.sThumbLX * gp.sThumbLX;
	//gp.sThumbLY *= gp.sThumbLY * gp.sThumbLY;
	//gp.sThumbRX *= gp.sThumbRX * gp.sThumbRX;
	//gp.sThumbRY *= gp.sThumbRY * gp.sThumbRY;

	//gp.bLeftTrigger *= gp.bLeftTrigger * gp.bLeftTrigger;
	//gp.bRightTrigger *= gp.bRightTrigger * gp.bRightTrigger;
}


bool XInputDriver::getThumbDirection( control_id_t control, InputSpec const &spec,
                                      unsigned direction, InputDetails &details )
{
	SHORT x, y, oldX, oldY, val, oldVal;
	XINPUT_GAMEPAD const &gp = m_state.state.Gamepad;
	XINPUT_GAMEPAD const &oldGP = m_oldState.state.Gamepad;
	double sensitivity = spec.handler_id / 100.0;

	switch ( control ) {
		case XInputLeftThumbstick:
			x = gp.sThumbLX; y = gp.sThumbLY;
			oldX = oldGP.sThumbLX; oldY = oldGP.sThumbLY;
			break;

		case XInputRightThumbstick:
			x = gp.sThumbRX; y = gp.sThumbRY;
			oldX = oldGP.sThumbRX; oldY = oldGP.sThumbRY;
			break;

		default:
			return false; // We should never get here!
	}

	switch ( direction ) {
		case North:
			val = -y; oldVal = -oldY; break;
		case South:
			val = y; oldVal = oldY; break;
		case East:
			val = x; oldVal = oldX; break;
		case West:
			val = -x; oldVal = -oldX; break;
		default:
			return false; // We should never get here!
	}

	details.type = INPUT_TYPE_BOOL;
	details.x = round( val * sensitivity );
	details.y = round( oldVal * sensitivity );

	if ( val || oldVal )
		details.type = INPUT_TYPE_BOOL;

	bool isPressed = ( details.x > SCALE_JOYSTICK );
	bool wasPressed = ( details.y > SCALE_JOYSTICK );
	double now = 0.0;
	bool rep = false;

	switch ( spec.condition ) {

		case COND_DOWN:
			return isPressed && !wasPressed;

		case COND_PRESSED:
			return isPressed;

		case COND_UP:
			return wasPressed && !isPressed;

		case COND_REPEAT:
			now = GetHighResTime();
			rep = isPressed &&
			      ( !wasPressed
			      || ( s_nextrepeat[ control ] <= now ) );
			if ( rep )
				s_nextrepeat[ control ] = now + REPEAT_FREQ;
			return rep;

		default:
			return false; // We should never get here!

	}

} // End of getThumbDirection


void XInputDriver::Advance()
{
	DWORD dwResult;
	XINPUT_STATE &state = m_state.state;
	// This could be faster if we switched pointers around instead
	// but it would lead to extra indirection elsewhere
	memcpy( &m_oldState, &m_state, sizeof(ControllerState) );
	memset( &m_state, 0, sizeof(ControllerState) );

	dwResult = XInputGetState( 0, &state );
	if ( m_state.isConnected = ( ERROR_SUCCESS == dwResult ) ) {
		correctDeadZones( state );
		scaleInputs( state );
	}
}


bool XInputDriver::acceptDriver( std::string const &name )
{
	m_optTokenState = BEFORE_OPTIONAL_TOKENS;
	return ( name == "XInput" );
}


control_id_t XInputDriver::getControlID( std::string const &name )
{
	int i = 0;
	const char *cmd = s_controls[ i ].name;
	while ( cmd && name != cmd )
		cmd = s_controls[ ++i ].name;
	if ( i < XInputNumControls )
		return i;
	else
		return -1;
}


inputtype_t XInputDriver::getControlType( control_id_t control_id )
{
	return s_controls[ control_id ].type;
}


condition_t XInputDriver::getConditionID( std::string const &name, inputtype_t &type )
{
	if ( INPUT_TYPE_BOOL == type && name == "repeat" )
		return COND_REPEAT;
	return getDefaultConditionID( name, type );
}


InputParserState XInputDriver::writeExtraSpecInfo( InputSpec &spec )
{
	switch ( spec.control_id ) {
		case XInputLeftThumbstick:
		case XInputRightThumbstick:
		case XInputLeftTrigger:
		case XInputRightTrigger:
			spec.handler_id = 100;  // Set default sensitivity
			return STATE_WANT_OPTIONAL;
		case XInputLeftThumbstickLeft:
		case XInputLeftThumbstickRight:
		case XInputLeftThumbstickUp:
		case XInputLeftThumbstickDown:
		case XInputRightThumbstickLeft:
		case XInputRightThumbstickRight:
		case XInputRightThumbstickUp:
		case XInputRightThumbstickDown:
			spec.handler_id = 100;  // Set default sensitivity
		default:
			return STATE_DONE;
	}
}


InputParserState XInputDriver::parseExtraToken( std::string const &token, InputSpec &spec )
{
	InputParserState state = STATE_BAD_EXTRA;

	switch ( m_optTokenState ) {
		case BEFORE_OPTIONAL_TOKENS:
			if ( token == "with" ) {
				m_optTokenState = AFTER_TOKEN_WITH;
				state = STATE_WANT_MODIFIER;
			}
			break;

		case AFTER_TOKEN_WITH:
			if ( token == "sensitivity" ) {
				m_optTokenState = AFTER_TOKEN_SENSITIVITY;
				state = STATE_WANT_MODIFIER;
			}
			break;

		case AFTER_TOKEN_SENSITIVITY:
			std::istringstream str( token );
			int num;
			if ( ( str >> num ) && num > 0 && num <= 200 ) {
				spec.handler_id = num;
				state = STATE_DONE;
			}
			break;
	}

	return state;
}


bool XInputDriver::getInputDescription( InputSpec const &spec, InputDescription &desc )
{

	bool controlInRange = ( 0 <= spec.control_id && spec.control_id < XInputNumControls );
	bool verbOK;
	inputtype_t type;

	desc.noun = "control_gamepad_";
	if ( controlInRange ) {
		desc.noun.append( s_controls[ spec.control_id ].name );
		type = s_controls[ spec.control_id ].type;
	} else
		desc.noun.append( "unknown" );

	if ( INPUT_TYPE_1D == type ) {

		switch ( spec.condition ) {
			case COND_NONZERO:
			case COND_MOVED:
			case COND_READ:
				desc.verb = "control_verb_squeeze";
				verbOK = true;
				break;
			case COND_RELEASED:
			case COND_ZERO:
				verbOK = getDefaultVerb( spec.condition, INPUT_TYPE_1D, desc.verb );
				break;
			default:
				desc.verb = "control_verb_unknown";
				return false;
		}

	} else
		verbOK = getDefaultVerb( spec.condition, type, desc.verb );

	return controlInRange && verbOK;

} // End of getInputDescription
