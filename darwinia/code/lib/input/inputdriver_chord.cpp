#include "lib/universal_include.h"

#include <algorithm>

#include "lib/input/inputdriver_chord.h"
#include "lib/input/input.h"

using namespace std;


static string nullErr = "";


ChordInputDriver::ChordInputDriver()
: m_specs(),
  lastError( nullErr )
{
	setName( "Chord" );
}


InputParserState ChordInputDriver::parseInputSpecification( InputSpecTokens const &tokens,
                                                            InputSpec &spec )
{
	string s;

	auto_ptr<InputSpecList> speclist( new InputSpecList() );
	vector<string> strings;
	bool hasParts = false;

	spec.type = INPUT_TYPE_BOOL;

	for ( unsigned i = 0; i <= tokens.length(); ++i ) {
		if ( i < tokens.length() && tokens[ i ] != "++" ) s += " " + tokens[ i ];
		else {
			if ( !hasParts && tokens.length() == i ) return STATE_ERROR; // Not a chord
			hasParts = true;
			strings.push_back( s );
			s.clear(); // Ready for another spec
		}
	}

	for ( unsigned nDown = 0; nDown < strings.size(); ++nDown ) {
		for ( unsigned i = 0; i < strings.size(); ++i ) {
			if ( !i ) s = "";
			else s.append( " && " );
			s.append( strings[ i ] );
			s.append( (nDown == i) ? " down" : " pressed" );
		}

		InputSpec partspec;
		partspec.type = INPUT_TYPE_BOOL;

		InputParserState pState = g_inputManager->parseInputSpecString( s, partspec, lastError );
		if ( PARSE_SUCCESS( pState ) ) {
			if ( partspec.type > INPUT_TYPE_BOOL ) {
				static string repError = "Complex inputs are not allowed in chords.";
				lastError = repError;
				return STATE_CONJ_ERROR;
			}
			speclist->push_back( InputSpecPtr( new InputSpec( partspec ) ) );
		} else {
			return STATE_CONJ_ERROR;
		}
	}

	// Parsing went OK. Save this.
	m_specs.push_back( speclist );
	spec.control_id = m_specs.size() - 1;
	return STATE_DONE;
}


// Actually implements a disjunction
bool ChordInputDriver::getInput( InputSpec const &spec, InputDetails &details )
{
	if ( 0 <= spec.control_id && spec.control_id < m_specs.size() ) {
		const InputSpecList &specs = *(m_specs[ spec.control_id ]);
		for ( InputSpecIt i = specs.begin(); i != specs.end(); ++i )
			if ( g_inputManager->checkInput( **i, details ) )
				return true;
	}
	return false;
}


void ChordInputDriver::Advance()
{
}


const string &ChordInputDriver::getLastParseError( InputParserState state )
{
	return lastError;
}


bool ChordInputDriver::getInputDescription( InputSpec const &spec, InputDescription &desc )
{
	// IF _any_ return false, we don't return a description at all.

	bool firstPart = true;

	if ( 0 <= spec.control_id && spec.control_id < m_specs.size() ) {
		const InputSpecList &specs = *(m_specs[ spec.control_id ]);

		for ( InputSpecIt i = specs.begin(); i != specs.end(); ++i ) {
			InputDescription d;
			const InputSpec &spec = **i;
			if ( g_inputManager->getInputDescription( spec, d ) ) {
				d.translate();
				if ( firstPart ) {
					desc.noun = d.noun;
					desc.verb = d.verb;
					desc.translated = true;
					firstPart = false;
				} else
					desc.noun.append( "-" ).append( d.noun );
			} else
				return false;
		}

		return !firstPart;
	} else
		return false; // We should never get here!
}
