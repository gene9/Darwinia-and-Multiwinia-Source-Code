#include "lib/universal_include.h"

#include <algorithm>
#include <sstream>

#include "lib/input/inputdriver_pipe.h"
#include "lib/input/input.h"
#include "lib/input/inputfiltermanager.h"

using namespace std;


struct InputFilterWithArgs {
	InputFilterSpec filter;
	InputSpecList args;
};


static string nullErr = "";


PipeInputDriver::PipeInputDriver()
: m_specs(),
  lastError( nullErr )
{
	setName( "Pipe" );
}


InputParserState PipeInputDriver::parseInputSpec( ostringstream &stream,
                                                  InputSpecList &speclist )
{
	if ( stream.str() == "" ) {
		static string emptyError = "An argument to the input converter is empty.";
		lastError = emptyError;
		return STATE_CONJ_ERROR;
	}

	InputSpec spec;
	InputParserState pState = g_inputManager->parseInputSpecString( stream.str(), spec, lastError );
	if ( PARSE_SUCCESS( pState ) ) {
		speclist.push_back( InputSpecPtr( new InputSpec( spec ) ) );
		stream.str("");
		return pState;
	} else
		return STATE_CONJ_ERROR;
}


InputParserState PipeInputDriver::parseInputSpecification( InputSpecTokens const &tokens,
                                                           InputSpec &spec )
{
	string s = "";
	auto_ptr<InputFilterWithArgs> filterWithArgs( new InputFilterWithArgs() );

	spec.type = INPUT_TYPE_BOOL;

	// Get the last position of an arrow token
	unsigned arrowPos = -1;
	for ( unsigned i = tokens.length(); i >= 0; --i )
		if ( tokens[ i ] == "->" ) {
			arrowPos = i;
			break;
		}

	// Discard if there is no arrow
	if ( 0 >= arrowPos || arrowPos >= tokens.length() )
		return STATE_ERROR;

	// Get everything up to arrow
	for ( unsigned j = 0; j < arrowPos; ++j )
		s += " " + tokens[ j ];

	// Cut off outside square brackets, if any,
	// and split at commas inside said brackets
	// then parse the bits
	int brackets = 0;
	int parsingBrackets = 0;
	ostringstream stream;

	stream.clear();
	for ( string::const_iterator it = s.begin(); it != s.end(); ++it ) {
		bool split = false;
		switch ( *it ) {
			case '[':
				if ( parsingBrackets == 0 )
					parsingBrackets = 1;
				brackets++; break;
			case ']':
				brackets--; break;
			case ',':
				split = (brackets == 1 && parsingBrackets == 1); break;
			default:
				if ( parsingBrackets == 0 && *it != ' ' )
					parsingBrackets = -1;
				stream.put( *it );
		}

		if ( split ) {
			InputParserState pState = parseInputSpec( stream, filterWithArgs->args );
			if ( !PARSE_SUCCESS( pState ) )
				return pState;
		}

	} // End for loop

	// Make sure we process the last portion read
	InputParserState pState = parseInputSpec( stream, filterWithArgs->args );
	if ( !PARSE_SUCCESS( pState ) )
		return pState;

	// Check for unmatched brackets
	if ( brackets != 0 ) {
		static string bracketError = "Unmatched square brackets.";
		lastError = bracketError;
		return STATE_CONJ_ERROR;
	}

	// We're done with the left hand side. Now for the filter.
	s = "";
	for ( unsigned k = arrowPos + 1; k < tokens.length(); ++k )
		s += " " + tokens[ k ];

	if ( !g_inputFilterManager->parseFilterSpecString( s, filterWithArgs->filter, lastError ) )
		return STATE_CONJ_ERROR;

	// Parsing went OK. Save this.
	m_specs.push_back( filterWithArgs );
	spec.control_id = m_specs.size() - 1;
	return STATE_DONE;
}


bool PipeInputDriver::getInput( InputSpec const &spec, InputDetails &details )
{
	if ( 0 <= spec.control_id && spec.control_id < m_specs.size() ) {
		const InputFilterWithArgs &filterWithArgs = *(m_specs[ spec.control_id ]);
		return g_inputFilterManager->filter( filterWithArgs.args, filterWithArgs.filter, details );
	} else
		return false; // We should never get here!
}


void PipeInputDriver::Advance()
{
}


const string &PipeInputDriver::getLastParseError( InputParserState state )
{
	return lastError;
}

