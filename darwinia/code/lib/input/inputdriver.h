#ifndef INCLUDED_INPUTDRIVER_H
#define INCLUDED_INPUTDRIVER_H

#include <string>

#include "lib/input/input_types.h"
#include "lib/input/inputspec.h"

#define PARSE_SUCCESS(x) ( STATE_DONE == (x) )


// Specific drivers may or may not want to use this enum,
// but is is possibly helpful for maintainability
enum InputCondition {
	COND_DOWN,     // Button was just pushed down
	COND_UP,       // Button was just released
	COND_PRESSED,  // Button is still pressed
	COND_REPEAT,   // Button is held and generates key repeats
	COND_ACTION,   // Something happened (as in "key any action")
	COND_RELEASED, // Analog just entered dead zone
	COND_NONZERO,  // Analog is outside dead zone (return position)
	COND_MOVED,    // Analog is outside dead zone (return delta)
	COND_ZERO,     // Analog is still in dead zone
	COND_READ,     // Analog always triggers (return default device info)
	NumInputConditions
};


enum InputParserState {
	STATE_ERROR,         // We can't parse this correctly, reason unknown
	STATE_WANT_DRIVER,   // About to read the driver (eg. "XInput")
	STATE_WANT_CONTROL,  // About to read the control (eg. "start")
	STATE_WANT_COND,     // About to read a condition (eg. "down")
	STATE_WANT_MODIFIER, // About to read modifier
	STATE_WANT_OPTIONAL, // About to read optional modifier
	STATE_BAD_EXTRA,     // Error occurred whilst writing extra information
	STATE_OVERSTEP,      // Too many tokens
	STATE_CONJ_ERROR,    // Error in conjunction
	STATE_DONE           // Finished and OK
};


class InputDriver {

private:
	std::string m_name;

protected:
	// Give this a condition name and flags for the possible types of control
	// that we're allowing. Returns the ID of the found condition and specifies
	// the type to which it belongs (if there was more than one option).
	condition_t getDefaultConditionID( std::string const &name, inputtype_t &type );

	// Get the string that by default identifies the condition given for the type of
	// input given. For writing preference files
	bool getDefaultPrefsString( condition_t condition_id, inputtype_t type, std::string &prefsString );

	// Get the verb corresponding to the condition and type given, returning true
	// upon success, false when verb is set to some default filler text
	bool getDefaultVerb( condition_t condition_id, inputtype_t type, std::string &verb );

	void setName( std::string const &name );

public:
	// Return STATE_DONE if we managed to parse these tokens, and put the parsed information
	// into spec. Anything else means we failed.
	virtual InputParserState parseInputSpecification( InputSpecTokens const &tokens,
		                                              InputSpec &spec ) = 0;

	// Get input state. True if the input was triggered (input condition met). If true,
	// details are placed in details.
	virtual bool getInput( InputSpec const &spec, InputDetails &details ) = 0;

	// Returns true if the InputDriver is receiving no user input. Used to access screensaver
	// type modes.
	virtual bool isIdle();

	// Returns the input mode associated with the InputDriver (keyboard or gamepad or none)
	virtual InputMode getInputMode();

	// Returns true if there was an "active" input event this frame. Fills spec with
	// the details of the input. Active inputs are primarily things like button presses
	// but not buttons held down or released or 2D analog events.
	virtual bool getFirstActiveInput( InputSpec &spec, bool instant );

	// This triggers a read from the input hardware and does message polling
	virtual void Advance() = 0;

	// Poll for system events that may require immediate, hard-coded action
	// eg. Window minimise or XInput plug pulled may always be required to
	// pause the game. Window close or kill signals should also be dealt with
	// promptly. This should probably be called in Advance() as well as elsewhere.
	virtual void PollForEvents();

	// Return a helpful error string when there's a problem
	virtual const std::string &getLastParseError( InputParserState state ) = 0;

	// Fill out a description of the input defined by spec
	virtual bool getInputDescription( InputSpec const &spec, InputDescription &desc );

	// Get the name of the driver (debuggung purposes)
	virtual const std::string &getName() const;

};


std::ostream &operator<<( std::ostream &stream, InputDriver const &driver );


#endif // INCLUDED_INPUTDRIVER_H
