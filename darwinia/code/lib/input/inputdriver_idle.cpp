#include "lib/universal_include.h"

#include <string>
#include <sstream>
//#include <fstream>

#include "lib/input/input.h"
#include "lib/input/control_bindings.h"
#include "lib/input/inputdriver_idle.h"
#include "lib/hi_res_time.h"


// Must be in the same order as s_controls (below)
enum {
	IdleAtLeast,
	IdleExactly,
	IdleReset,

	IdleNumControls
};


// Must be in the same order as the above enum
static ControlAction s_controls[] = {
	"atleast", INPUT_TYPE_1D,
	"reaches", INPUT_TYPE_1D,
	"no",      INPUT_TYPE_BOOL
};


IdleInputDriver::IdleInputDriver()
: m_idleTime( 0 ),
  m_oldIdleTime( 0 )
{
	setName( "Idle" );
	m_lastChecked = GetHighResTime();
}


bool IdleInputDriver::getInput( InputSpec const &spec, InputDetails &details )
{
	details.x = static_cast<int>( m_idleTime );
//	details.y = static_cast<int>( m_oldIdleTime );

	bool ans;
	switch ( spec.control_id ) {

		case IdleAtLeast:
			details.type = INPUT_TYPE_1D;
			ans = details.x >= spec.condition;
			break;

		case IdleExactly:
			details.type = INPUT_TYPE_1D;
			ans = ( m_oldIdleTime < spec.condition &&
			        spec.condition < m_idleTime );
			break;

		case IdleReset:
			details.type = INPUT_TYPE_BOOL;
			ans = ( 0 == m_idleTime );
			break;

		default:
			ans = false; // We should never get here!

	}

	//std::ofstream derr( "idle.log", std::ios::app );
	//double timeNow = GetHighResTime();
	//derr << timeNow << ": " << "idl = " << details.x << ", "
	//     << "cond = " << spec.condition << ", "
	//	 << "ans = " << ( ans ? "true" : "false" ) << std::endl;

	return ans;
}


void IdleInputDriver::Advance()
{
	double timeNow = GetHighResTime();
	m_oldIdleTime = m_idleTime;

	if ( g_inputManager->isIdle() ) {
		double timeDelta = timeNow - m_lastChecked;
		m_idleTime += timeDelta;
	} else {
		m_idleTime = 0;
	}

	m_lastChecked = timeNow;
}


bool IdleInputDriver::acceptDriver( std::string const &name )
{
	return stricmp( name.c_str(), "idle" ) == 0;
}


control_id_t IdleInputDriver::getControlID( std::string const &name )
{
	int i = 0;
	const char *cmd = s_controls[ i ].name;
	while ( cmd && name != cmd )
		cmd = s_controls[ ++i ].name;
	if ( i < IdleNumControls )
		return i;
	else
		return -1;
}


inputtype_t IdleInputDriver::getControlType( control_id_t control_id )
{
	return s_controls[ control_id ].type;
}


condition_t IdleInputDriver::getConditionID( std::string const &name, inputtype_t &type )
{
	if ( INPUT_TYPE_BOOL == type ) {
		return ( stricmp( name.c_str(), "longer" ) == 0 ) ? 0 : -1;
	} else if ( INPUT_TYPE_1D == type ) {
		std::istringstream str( name );
		int num;
		if ( ( str >> num ) && num > 0 )
			return num;
	}
	return -1;
}


bool IdleInputDriver::getInputDescription( InputSpec const &spec, InputDescription &desc )
{
	// TODO: This
	return false;
}
