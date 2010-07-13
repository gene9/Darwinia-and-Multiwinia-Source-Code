#include "lib/universal_include.h"

#include "lib/input/inputdriver_invert.h"
#include "lib/input/input.h"
#include <memory>
//#include <fstream>

using namespace std;


static string nullErr = "";


InvertInputDriver::InvertInputDriver()
: m_specs(),
  lastError( nullErr )
{
	setName( "Invert" );
}


bool InvertInputDriver::getInput( InputSpec const &spec, InputDetails &details )
{
	if ( 0 <= spec.control_id && spec.control_id < m_specs.size() ) {
		const InputSpec &invspec = *(m_specs[ spec.control_id ]);
		bool ans = !( g_inputManager->checkInput( invspec, details ) );
		return ans;
	}
	return false;
}


void InvertInputDriver::Advance()
{
}


const string &InvertInputDriver::getLastParseError( InputParserState state )
{
	return lastError;
}


InputParserState InvertInputDriver::parseInputSpecification( InputSpecTokens const &tokens,
                                                             InputSpec &spec )
{
	//ofstream derr( "inputinvert_debug.txt", ios::app );
	//derr << "Full: " << tokens << endl;

	if ( tokens.length() < 1 ) return STATE_ERROR;
	if ( ( stricmp( tokens[0].c_str(), "not" ) == 0 ) ||
	     tokens[0] == "!" ) {
		auto_ptr<InputSpecTokens> newtokens = tokens( 1, -1 );
		//derr << "Part: " << *newtokens << endl;
		InputSpec invspec;
		InputParserState state = g_inputManager->parseInputSpecTokens( *newtokens, invspec, lastError );
		if ( PARSE_SUCCESS( state ) ) {
			if ( invspec.type != INPUT_TYPE_BOOL ) {
				static string complexErr = "Complex input types cannot be negated.";
				lastError = complexErr;
				return STATE_CONJ_ERROR; // This check may be too restrictive
			}
			m_specs.push_back( auto_ptr<const InputSpec>( new InputSpec( invspec ) ) );
			spec.type = INPUT_TYPE_BOOL;
			spec.control_id = m_specs.size() - 1;
			return STATE_DONE;
		}
		return state;
	} else
		return STATE_ERROR;

}


bool InvertInputDriver::getInputDescription( InputSpec const &spec, InputDescription &desc )
{
	if ( 0 <= spec.control_id && spec.control_id < m_specs.size() ) {
		const InputSpec &invspec = *(m_specs[ spec.control_id ]);
		if ( g_inputManager->getInputDescription( invspec, desc ) ) {
			desc.verb = "not " + desc.verb;
			return true;
		}
	}
	return false;
}
