#include "lib/universal_include.h"

#include <string>

#include "lib/preferences.h"
#include "lib/input/inputdriver_value.h"

using namespace std;


enum {
	#define DEF_VALUE(x,y) Value##x,
	#include "lib/input/inputdriver_values.inc"
	#undef DEF_VALUE

	NumValues
};


static string s_values[] = {
	#define DEF_VALUE(x,y) #x,
	#include "lib/input/inputdriver_values.inc"
	#undef DEF_VALUE
	""
};


InputParserState ValueInputDriver::parseInputSpecification( InputSpecTokens const &tokens,
                                                            InputSpec &spec )
{
	InputParserState state = STATE_WANT_DRIVER;
	unsigned idx = 0;
	if ( ( tokens.length() < idx + 1 ) ||
	     ( stricmp( tokens[idx++].c_str(), "value" ) == 0 ) )
		return state;

	state = STATE_WANT_CONTROL;
	bool controlOK = false;
	if ( tokens.length() < idx + 1 ) return state;
	for ( unsigned i = 0; i < NumValues; ++i ) {
		if ( tokens[idx] == s_values[i] ) {
			spec.control_id = i;
			controlOK = true;
			break;
		}
	}

	if ( !controlOK ) return state;

	if ( tokens.length() > idx + 1 ) return STATE_OVERSTEP;

	return STATE_DONE;
}


bool ValueInputDriver::getInput( InputSpec const &spec, InputDetails &details )
{
	details.type = INPUT_TYPE_BOOL;
	bool ans = false;
	switch ( spec.control_id ) {
		#define DEF_VALUE(x,y) case Value##x:\
			ans = (y); break;
		#include "lib/input/inputdriver_values.inc"
		#undef DEF_VALUE

		default:
			ans = false;
	}

	return ans;
}


void ValueInputDriver::Advance()
{
}


// In the same order as enum InputParserState (see inputdriver.h)
static string errors[] = {
	"An unknown error occurred.",
	"The driver type was not recognised.",
	"The value type was not recognised.",
	"",
	"",
	"",
	"There are too many tokens in the input description.",
	"",
	"There was no parsing error."
};


const std::string &ValueInputDriver::getLastParseError( InputParserState state )
{
	return errors[ state ];
}
