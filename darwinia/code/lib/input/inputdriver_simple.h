#ifndef INCLUDED_INPUTDRIVER_SIMPLE_H
#define INCLUDED_INPUTDRIVER_SIMPLE_H

#include <string>

#include "lib/input/inputdriver.h"


class SimpleInputDriver : public InputDriver {

private:

protected:
	virtual bool acceptToken( InputParserState &state, std::string const &token,
	                          InputSpec &spec );

	virtual bool acceptDriver( std::string const &name ) = 0;
	
	virtual control_id_t getControlID( std::string const &name ) = 0;

	virtual inputtype_t getControlType( control_id_t control_id ) = 0;

	virtual InputParserState writeExtraSpecInfo( InputSpec &spec );

	virtual InputParserState parseExtraToken( std::string const &token, InputSpec &spec );
	
	virtual condition_t getConditionID( std::string const &name, inputtype_t &type );

public:
	// Return STATE_DONE if we managed to parse these tokens, and put the parsed information
	// into spec. Anything else means we failed.
	virtual InputParserState parseInputSpecification( InputSpecTokens const &tokens,
		                                              InputSpec &spec );

	// Return a helpful error string when there's a problem
	virtual const std::string &getLastParseError( InputParserState state );

};


#endif // INCLUDED_INPUTDRIVER_SIMPLE_H
