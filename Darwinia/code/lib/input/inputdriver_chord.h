#ifndef INCLUDED_INPUTDRIVER_CHORD_H
#define INCLUDED_INPUTDRIVER_CHORD_H

#include "lib/input/inputdriver.h"
#include "lib/input/inputspeclist.h"


// Enables partial input specifications to be combined using "++" in the
// preferences file. Only allows boolean inputs that can handle "down"
// and "pressed" actions.

class ChordInputDriver : public InputDriver {

private:
	// List of lists of InputSpec
	auto_vector<InputSpecList> m_specs;
	std::string &lastError;

public:
	ChordInputDriver();

	// Return STATE_DONE if we managed to parse these tokens, and put the parsed information
	// into spec. Anything else means we failed.
	InputParserState parseInputSpecification( InputSpecTokens const &tokens,
	                                          InputSpec &spec );

	// Get input state. True if the input was triggered (input condition met). If true,
	// details are placed in details.
	bool getInput( InputSpec const &spec, InputDetails &details );

	// This triggers a read from the input hardware and does message polling
	void Advance();

	// Return a helpful error string when there's a problem
	const std::string &getLastParseError( InputParserState state );

	// Fill out a description of the input defined by spec
	bool getInputDescription( InputSpec const &spec, InputDescription &desc );

	// Get the name of the driver (debuggung purposes)
	const std::string &getName();

};


#endif // INCLUDED_INPUTDRIVER_CHORD_H
