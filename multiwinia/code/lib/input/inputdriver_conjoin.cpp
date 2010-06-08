#include "lib/universal_include.h"

#include <algorithm>

#include "lib/input/inputdriver_conjoin.h"
#include "lib/input/input.h"

using namespace std;


static string nullErr = "";


ConjoinInputDriver::ConjoinInputDriver()
: m_specs(),
  lastError( nullErr )
{
	setName( "Conjoin" );
}


InputParserState ConjoinInputDriver::parseInputSpecification( InputSpecTokens const &tokens,
                                                              InputSpec &spec )
{
	string s = "";
	auto_ptr<InputSpecList> speclist( new InputSpecList() );
	bool haveComplexInput = false;
	bool hasParts = false;

	spec.type = INPUT_TYPE_BOOL;

	for ( int i = 0; i <= tokens.length(); ++i ) {
		if ( i < tokens.length() && tokens[ i ] != "&&" ) s += " " + tokens[ i ];
		else {
			if ( !hasParts &&  tokens.length() == i ) return STATE_ERROR; // Not a conjunction
			hasParts = true;
			InputSpec partspec;
			partspec.type = INPUT_TYPE_BOOL;
			InputParserState pState = g_inputManager->parseInputSpecString( s, partspec, lastError );
			if ( PARSE_SUCCESS( pState ) ) {
				if ( partspec.type > INPUT_TYPE_BOOL ) {
					if ( haveComplexInput ) {
						static string repError = "Too many complex inputs in conjunction.";
						lastError = repError;
						return STATE_CONJ_ERROR;
					} else {
						haveComplexInput = true;
						spec.type = partspec.type;
					}
				}
				speclist->push_back( auto_ptr<const InputSpec>( new InputSpec( partspec ) ) );
				s.clear(); // Ready for another spec
			} else {
				return STATE_CONJ_ERROR;
			}
		}
	}

	// Parsing went OK. Save this.
	m_specs.push_back( speclist );
	spec.control_id = m_specs.size() - 1;
	return STATE_DONE;
}


bool ConjoinInputDriver::getInput( InputSpec const &spec, InputDetails &details )
{
	if ( 0 <= spec.control_id && spec.control_id < m_specs.size() ) {
		bool detailsSet = false;
		const InputSpecList &specs = *(m_specs[ spec.control_id ]);
		for ( InputSpecIt i = specs.begin(); i != specs.end(); ++i ) {
			InputDetails d;
			if ( g_inputManager->checkInput( **i, d ) ) {
				if ( !detailsSet || d.type > INPUT_TYPE_BOOL ) { // This is our actual return value
					details.type = d.type;
					details.x = d.x;
					details.y = d.y;
					detailsSet = true;
				}
			} else
				return false;
		}
		return detailsSet;
	} else
		return false; // We should never get here!
}


void ConjoinInputDriver::Advance()
{
}


const string &ConjoinInputDriver::getLastParseError( InputParserState state )
{
	return lastError;
}


bool ConjoinInputDriver::getInputDescription( InputSpec const &spec, InputDescription &desc )
{
	// We want to find the primary input, if possible.
	// IF _any_ return false, we don't return a description at all.

	if ( 0 <= spec.control_id && spec.control_id < m_specs.size() ) {
		bool descSet = false;
		inputtype_t curr_type = INPUT_TYPE_BOOL;
		const InputSpecList &specs = *(m_specs[ spec.control_id ]);

		for ( InputSpecIt i = specs.begin(); i != specs.end(); ++i ) {
			InputDescription d;
			const InputSpec &spec = **i;
			if ( g_inputManager->getInputDescription( spec, d ) ) {
				if ( !descSet || spec.type >= curr_type ) { // This is our actual return value
					desc.noun = d.noun;
					desc.verb = d.verb;
					curr_type = spec.type;
					descSet = true;
				}
			} else
				return false;
		}

		return descSet;
	} else
		return false; // We should never get here!
}
