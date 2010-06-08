#ifndef INCLUDED_INPUT_TYPES_H
#define INCLUDED_INPUT_TYPES_H

#include <string>
#include <memory>
#include "lib/auto_vector.h"


enum InputType {
	INPUT_TYPE_FAIL = 0,   // The InputDetails are invalid
	INPUT_TYPE_BOOL = 1,   // Buttons, etc.
	INPUT_TYPE_ANALOG = 2, // Contains axis information
	INPUT_TYPE_1D = 6,     // XInput trigger buttons (implies ANALOG)
	INPUT_TYPE_2D = 10,    // Mouse or joystick axes (implies ANALOG)
	INPUT_TYPE_ANY = 15    // Not concerned (just want to know if it triggered)
};


struct InputDetails {
	InputType type;  // Determines what we can expect in x and y
	int x;           // Only meaningful if type is INPUT_TYPE_1D or INPUT_TYPE_2D
	int y;           // Only meaningful if type is INPUT_TYPE_2D
};

typedef auto_vector<const InputDetails> InputDetailsList;
typedef std::auto_ptr<InputDetails> InputDetailsPtr;

class InputDescription {

public:
	InputDescription();

	InputDescription( InputDescription const &_desc );

	InputDescription( std::string const &_noun, std::string const &_verb,
	                  std::string const &_pref, bool _translated = false );

	void translate();

	std::string noun;

	std::string verb;

	std::string pref;

	bool translated;

};


enum InputMode {
	INPUT_MODE_NONE,       // Describes a driver not associated with a specific input mode
	INPUT_MODE_KEYBOARD,   // Describes a driver that accepts input from Keyboard or Mouse
	INPUT_MODE_GAMEPAD     // Describes a driver that accepts input from a Gamepad
};


#endif INCLUDED_INPUT_TYPES_H
