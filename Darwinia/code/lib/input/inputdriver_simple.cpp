#include "lib/universal_include.h"

#include <string>

#include "lib/input/inputspec.h"
#include "lib/input/inputdriver_simple.h"

using namespace std;


// In the same order as enum InputParserState (see inputdriver.h)
static string errors[] = {
	"An unknown error occurred.",
	"The driver type was not recognised.",
	"The control type was not recognised.",
	"The control condition was not recognised.",
	"The condition modifier was not recognised.",
	"There was an error writing extra information.",
	"There are too many tokens in the input description.",
	"There was an error in the conjunction.",
	"There was no parsing error."
};


InputParserState SimpleInputDriver::parseInputSpecification( InputSpecTokens const &tokens,
                                                             InputSpec &spec ) {

	InputParserState state = STATE_WANT_DRIVER;
	for ( int i = 0; i < tokens.length(); ++i ) {
		if ( !acceptToken( state, tokens[ i ], spec ) ) break;
	}

	if ( STATE_WANT_OPTIONAL == state )
		state = STATE_DONE;

	return state;

} // End of parseInputSpecification


bool SimpleInputDriver::acceptToken( InputParserState &state, string const &token,
                                     InputSpec &spec ) {
	switch ( state ) {

		case STATE_WANT_DRIVER:
			if ( acceptDriver( token ) ) {
				state = STATE_WANT_CONTROL;
				return true;
			} else
				return false; // Don't continue parsing

		case STATE_WANT_CONTROL:
			spec.control_id = getControlID( token );
			if ( spec.control_id >= 0 ) {
				spec.type = getControlType( spec.control_id );
				state = STATE_WANT_COND;
				return true;
			} else
				return false; // Don't continue parsing

		case STATE_WANT_COND:
			spec.condition = getConditionID( token, spec.type );
			if ( spec.condition >= 0 ) {
				state = writeExtraSpecInfo( spec );
				return true;
			} else
				return false; // Don't continue parsing

		case STATE_WANT_MODIFIER:
		case STATE_WANT_OPTIONAL:
			state = parseExtraToken( token, spec );
			return true;

		case STATE_BAD_EXTRA:
			return false; // Don't continue parsing

		case STATE_DONE:
			state = STATE_OVERSTEP;
			return false; // Don't continue parsing

		default:
			state = STATE_ERROR; // We shouldn't be here
			return false; // Don't continue parsing

	}
}


condition_t SimpleInputDriver::getConditionID( string const &name, inputtype_t &type ) {
	return getDefaultConditionID( name, type );
}


InputParserState SimpleInputDriver::writeExtraSpecInfo( InputSpec &spec ) {
	return STATE_DONE; // No extra info to write. We're done.
}

InputParserState SimpleInputDriver::parseExtraToken( std::string const &token,
                                                     InputSpec &spec ) {
	return STATE_BAD_EXTRA; // Got into STATE_WANT_MODIFIER with no handler.
}

const string &SimpleInputDriver::getLastParseError( InputParserState state ) {
	return errors[ state ];
}
