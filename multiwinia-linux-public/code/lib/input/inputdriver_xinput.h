#ifndef INCLUDED_INPUT_XINPUT_H
#define INCLUDED_INPUT_XINPUT_H

#include <string>
#include <XInput.h>
#include "lib/input/inputdriver_simple.h"


class XInputDriver : public SimpleInputDriver {

private:
	struct ControllerState {
		bool isConnected;
		XINPUT_STATE state;
	};

	enum OptionalTokenState {
		BEFORE_OPTIONAL_TOKENS,
		AFTER_TOKEN_WITH,
		AFTER_TOKEN_SENSITIVITY
	};

	static const float SCALE_JOYSTICK_L;
    static const float SCALE_JOYSTICK_R;
	static const float SCALE_TRIGGER;
	static const double REPEAT_FREQ;

	// At the moment this only supports the first controller
	
	class Controller {
	public:
		Controller();

		void Initialise( int _id );

		void Advance();
		bool getInput( InputSpec const &spec, InputDetails &details );
		bool isIdle();

	private:
		bool getButtonInput( WORD button, InputSpec const &spec, InputDetails &details );

		bool getThumbInput( InputSpec const &spec, InputDetails &details );

		bool getTriggerInput( InputSpec const &spec, InputDetails &details );

		void correctDeadZones( XINPUT_STATE &state );

		void scaleInputs( XINPUT_STATE &state );

		bool getThumbDirection( control_id_t control, InputSpec const &spec,
								unsigned direction, InputDetails &details );

	private:
		int m_id;
		ControllerState m_state;
		ControllerState m_oldState;
	};

	static const int MaxControllers = 4;
	Controller m_controllers[MaxControllers];
	unsigned m_controllerMask;

	OptionalTokenState m_optTokenState;

	bool acceptDriver( std::string const &name );
	
	control_id_t getControlID( std::string const &name );

	inputtype_t getControlType( control_id_t control_id );

	InputParserState writeExtraSpecInfo( InputSpec &spec );

	InputParserState parseExtraToken( std::string const &token, InputSpec &spec );

protected:
	virtual condition_t getConditionID( std::string const &name, inputtype_t &type );

public:
	XInputDriver();

	void setControllerMask( unsigned controllerMask );

	// Get input state. True if the input was triggered (input condition met). If true,
	// details are placed in details.
	bool getInput( InputSpec const &spec, InputDetails &details );

	// Returns true if the InputDriver is receiving no user input. Used to access screensaver
	// type modes.
	bool isIdle();

	// Returns the input mode associated with the InputDriver (keyboard or gamepad or none)
	InputMode getInputMode();

	// This triggers a read from the input hardware and does message polling
	void Advance();

	// Fill out a description of the input defined by spec
	bool getInputDescription( InputSpec const &spec, InputDescription &desc );

	// Get the name of the driver (debuggung purposes)
	const std::string &getName();

};


#endif //INCLUDED_INPUT_XINPUT_H
