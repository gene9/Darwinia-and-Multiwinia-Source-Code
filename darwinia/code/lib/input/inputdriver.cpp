#include "lib/universal_include.h"

#include <iostream>

#include "lib/input/inputdriver.h"


struct ConditionInfo {
	inputtype_t type;     // InputType
	char *name;
	condition_t cond;     // InputCondition
};

static ConditionInfo s_conditions[] = {
	// Type            Name        Condition
	INPUT_TYPE_BOOL,   "down",     COND_DOWN,     // Button was just pushed down
	INPUT_TYPE_BOOL,   "up",       COND_UP,       // Button was just released
	INPUT_TYPE_BOOL,   "pressed",  COND_PRESSED,  // Button is still pressed
//	INPUT_TYPE_BOOL,   "action",   COND_ACTION,   // Something happened (as in "key any action")
	INPUT_TYPE_ANALOG, "release",  COND_RELEASED, // Analog just entered dead zone
	INPUT_TYPE_ANALOG, "position", COND_NONZERO,  // Analog is outside dead zone (return position)
	INPUT_TYPE_ANALOG, "move",     COND_MOVED,    // Analog is outside dead zone (return delta)
	INPUT_TYPE_ANALOG, "zero",     COND_ZERO,     // Analog is still in dead zone
	INPUT_TYPE_ANALOG, "read",     COND_READ,     // Analog always triggers (return default device info)
	NULL,              NULL,       NULL
};

condition_t InputDriver::getDefaultConditionID( std::string const &name, inputtype_t &type )
{
	int i = 0;
	ConditionInfo info = s_conditions[ i ];
	while ( info.name && ( ( type & info.type ) != info.type
	                       || name != info.name ) )
		info = s_conditions[ ++i ];
	if ( i < NumInputConditions ) {
		type = info.type;
		return info.cond;
	} else
		return -1;
}


bool InputDriver::getDefaultPrefsString( condition_t condition_id, inputtype_t type, std::string &prefsString )
{
	int i = 0;
	ConditionInfo info = s_conditions[ i ];
	while ( info.name && ( ( type & info.type ) != info.type
	                       || condition_id != info.cond ) )
		info = s_conditions[ ++i ];
	if ( i < NumInputConditions ) {
		prefsString = info.name;
		return true;
	} else {
		prefsString = "unknown";
		return false;
	}
}


bool InputDriver::getDefaultVerb( condition_t condition_id, inputtype_t type, std::string &verb )
{
	std::string base;
	bool ans = getDefaultPrefsString( condition_id, type, base );
	verb = "control_verb_";
	verb.append( base );
	return ans;
}


void InputDriver::PollForEvents() {}


bool InputDriver::isIdle()
{
	return true;
}


InputMode InputDriver::getInputMode()
{
	return INPUT_MODE_NONE;
}


bool InputDriver::getInputDescription( InputSpec const &spec, InputDescription &desc )
{
	return false;
}


bool InputDriver::getFirstActiveInput( InputSpec &spec, bool instant )
{
	return false;
}


void InputDriver::setName( std::string const &name )
{
	m_name = name;
}


const std::string &InputDriver::getName() const
{
	return m_name;
}


std::ostream &operator<<( std::ostream &stream, InputDriver const &driver )
{
	return stream << "InputDriver (" << driver.getName() << ")";
}
