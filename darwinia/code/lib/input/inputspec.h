#ifndef INCLUDED_INPUTSPEC_H
#define INCLUDED_INPUTSPEC_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>

typedef int control_id_t;
typedef int inputtype_t;
typedef int condition_t;
typedef int handler_id_t;

struct InputSpec {
	unsigned driver;         // ID of InputDriver which handles this input
	inputtype_t type;        // Type of input details to expect
	control_id_t control_id; // Keycode, button number, etc.
	handler_id_t handler_id; // Maybe the driver contains several input handling functions
	condition_t condition;   // Condition upon which this triggers (down, up, held, clicked, etc.)
};


// Class to tokenise a prefs string
class InputSpecTokens {

private:
	std::vector<std::string> m_tokens;
	InputSpecTokens( std::vector<std::string> _tokens );

public:
	InputSpecTokens( std::string _string );
	~InputSpecTokens();
	unsigned length() const;
	const std::string &operator[]( unsigned _index ) const;
	std::auto_ptr<InputSpecTokens> operator()( int _start, int _end ) const;

};


std::ostream &operator<<( std::ostream &stream, InputSpecTokens const &tokens );


#endif // INCLUDED_INPUTSPEC_H
