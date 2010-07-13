#ifndef INCLUDED_INPUTTRANSFORM_H
#define INCLUDED_INPUTTRANSFORM_H

#include <memory>
#include <string>

#include "lib/input/input.h"


class InputTransform {

public:
	virtual bool operator()() = 0;

};


// ==================== class ControlEventFunctor ====================

class ControlEventFunctor : public InputTransform {

private:
	ControlType type;

public:
	ControlEventFunctor( ControlType _type );

	bool operator()();

	bool operator()( InputDetails &details );

	std::string noun();

};


// ==================== class ToggleInputTransform ====================

class ToggleInputTransform : public InputTransform {

private:
	std::auto_ptr<InputTransform> m_on, m_off;
	bool m_state, m_change;

	void Advance();

public:
	ToggleInputTransform( ControlType _on, ControlType _off );

	ToggleInputTransform( std::auto_ptr<InputTransform> _on,
	                      std::auto_ptr<InputTransform> _off );

	bool operator()();

	bool on();

	bool changed();

};


#endif // INCLUDED_INPUTTRANSFORM_H
