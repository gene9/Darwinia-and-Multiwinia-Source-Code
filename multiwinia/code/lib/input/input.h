#ifndef INCLUDED_INPUT_H
#define INCLUDED_INPUT_H

#include <vector>
#include <string>

#include "lib/filesys/text_stream_readers.h"
#include "lib/input/input_types.h"
#include "lib/input/inputspec.h"
#include "lib/input/inputdriver.h"
#include "lib/input/control_types.h"
#include "lib/input/control_bindings.h"


class InputManager {

private:
	std::vector<InputDriver *> drivers;

	ControlBindings bindings;

	bool m_idle;

	InputMode m_inputMode;

	// Does the work for controlEvent, below
	bool controlEventA( ControlType type, InputDetails &details );

public:
	InputManager();

	~InputManager();

	// Parse an input prefs file
	void parseInputPrefs( TextReader &reader, bool replace = false );

	// Return true if a particular control action was triggered this time step
	// and place any extra information into details.
	bool controlEvent( ControlType type, InputDetails &details );
	bool controlEvent( ControlType type ); // Not interested in details

	const std::string &controlIcon( ControlType type ) const;

	// Suppress an event for this frame
	void suppressEvent( ControlType type );

	// This triggers a read from the input hardware and does message polling
	void Advance();

	// Poll for system events that may require immediate, hard-coded action
	// since this will be called more often than Advance()
	void PollForEvents();

	// Add a new driver to the drivers collection. The driver will be deleted
	// at the end of the life of this InputManager, so must have been created
	// with "new". Never do this twice for the same driver.
	void addDriver( InputDriver *driver );

	// Returns a description of the first bound input of a control event
	bool getBoundInputDescription( ControlType type, InputDescription &desc );

	// Returns a description of the first bound input of a control event
	bool getInputDescription( InputSpec const &spec, InputDescription &desc );

	// Get the ID corresponding to a control name
	controltype_t getControlID( std::string const &control );

	// Inverse of getControlID
	void getControlString( ControlType type, std::string &name );

	// Check to see if this input spec has been triggered this time step
	// Not for general use outside the input system
	bool checkInput( InputSpec const &spec, InputDetails &details );

	// Returns true if there was an "active" input event this frame. Fills spec with
	// the details of the input. Active inputs are primarily things like button presses
	// but not buttons held down or released or 2D analog events.
	bool getFirstActiveInput( InputSpec &spec, bool instant = false );

	// Turn a prefs file string (right hand side) into an InputSpec, if
	// one of the drivers can handle it. Not for general use outside
	// of the input system
	InputParserState parseInputSpecString( std::string const &description, InputSpec &spec,
	                                       std::string &err );

	// Turn a set of tokens into an InputSpec. Not for general use outside of the input system
	InputParserState parseInputSpecTokens( InputSpecTokens const &tokens, InputSpec &spec,
	                                       std::string &err );

	// Replace primary binding with this one. Used when overriding default bindings
	// with user specified ones
	void replacePrimaryBinding( ControlType type, std::string const &prefString );

	// Remove all bindings
	void Clear();

	// Returns true if all of the InputDrivers report being idle
	bool isIdle();

	// Returns the input mode (keyboad or gamepad) currently in use
	InputMode getInputMode();

	void printNumBindings();

};


extern InputManager *g_inputManager;


#endif
