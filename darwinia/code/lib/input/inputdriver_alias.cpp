#include "lib/universal_include.h"

#include "lib/input/inputdriver_alias.h"
#include "lib/input/input.h"

using std::string;


AliasInputDriver::AliasInputDriver()
{
	setName( "Alias" );
}


InputParserState AliasInputDriver::parseInputSpecification( InputSpecTokens const &tokens,
                                                            InputSpec &spec )
{
	InputParserState state = STATE_WANT_DRIVER;
	int idx = 0;
	if ( ( idx >= tokens.length() ) ||
	     ( tokens[idx++] != "alias" ) ) return state;

	state = STATE_WANT_CONTROL;
	if ( idx >= tokens.length() ) return state;

	spec.control_id = g_inputManager->getControlID( tokens[idx++] );
	if ( spec.control_id < 0 ) {
		return state;
	}

	spec.type = INPUT_TYPE_BOOL;

	return ( idx < tokens.length() ) ? STATE_OVERSTEP : STATE_DONE;
}


bool AliasInputDriver::getInput( InputSpec const &spec, InputDetails &details )
{
	if ( 0 <= spec.control_id && spec.control_id < NumControlTypes )
		return g_inputManager->controlEvent( static_cast<ControlType>(spec.control_id), details );
	else
		return false; // We should never get here!
}


void AliasInputDriver::Advance()
{
}


// In the same order as enum InputParserState (see inputdriver.h)
static string errors[] = {
	"An unknown error occurred.",
	"The driver type was not recognised.",
	"The control alias was not recognised.",
	"",
	"",
	"",
	"There are too many tokens in the input description.",
	"",
	"There was no parsing error."
};

const string &AliasInputDriver::getLastParseError( InputParserState state )
{
	return errors[ state ];
}


bool AliasInputDriver::getInputDescription( InputSpec const &spec, InputDescription &desc )
{
	// TODO: This isn't quite right.
	// May break translations.
	if ( 0 <= spec.control_id && spec.control_id < NumControlTypes )
		return g_inputManager->getBoundInputDescription( static_cast<ControlType>(spec.control_id), desc );
	else
		return false; // We should never get here!
}
