#ifndef INCLUDED_CONTROL_BINDINGS_H
#define INCLUDED_CONTROL_BINDINGS_H

#include <string>

#include "lib/input/inputspec.h"
#include "lib/input/inputspeclist.h"
#include "lib/input/control_types.h"


struct ControlAction {
	const char *name; // How this action appears in a control preferences file.
	inputtype_t type; // The types of input that can be handled (bit flags of InputType)
};


// This is basically a collection of mappings of ControlType -> vector<InputSpec>
class ControlBindings {

private:
	// Holds the actual bindings
	InputSpecList bindings[NumControlTypes];

	// Hold icon file paths
	std::string icons[NumControlTypes];

	// Allows us to a control event for the rest of the frame
	unsigned char suppressed[NumControlTypes];

public:
	ControlBindings();

	// Returns the Control Type with the given name, or -1 with a bad name
	static controltype_t getControlID( std::string const &name );

	static void getControlString( ControlType type, std::string &name );

	// Returns true if a particular input type can be used to feed a
	// particular control type
	static bool isAcceptibleInputType( controltype_t binding, inputtype_t type );

	// Grab the list of InputSpec associated with a particular control type
	const InputSpecList &operator[]( ControlType id ) const;
	const InputSpecList &operator[]( controltype_t id ) const;

	// Grab the icon path associated with a particular control type
	const std::string &getIcon( ControlType id ) const;
	const std::string &getIcon( controltype_t id ) const;

	void setIcon( controltype_t id, std::string const &iconfile );

	// Associate an InputSpec with a control type, returning true on success
	bool bind( controltype_t type, InputSpec const &spec, bool replace = false );

	//bool replacePrimaryBinding( controltype_t type, InputSpec const &spec );

	// Signals a frame change
	void Advance();

	// Suppress an event until the end of the frame
	void suppress( ControlType id );

	// Check if an event is suppressed
	bool isActive( ControlType id ) const;

	// Remove all bindings
	void Clear();

};


#endif // INCLUDED_CONTROL_BINDINGS_H
