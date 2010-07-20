#include "lib/universal_include.h"

#include "lib/input/input.h"
#include "lib/input/transform.h"

using namespace std;


// ==================== class ControlEventFunctor ====================

ControlEventFunctor::ControlEventFunctor( ControlType _type )
: type( _type ) {}


bool ControlEventFunctor::operator()()
{
	if ( g_inputManager )
		return g_inputManager->controlEvent( type );
	else return false;
}


bool ControlEventFunctor::operator()( InputDetails &details )
{
	if ( g_inputManager )
		return g_inputManager->controlEvent( type, details );
	else return false;
}


string ControlEventFunctor::noun()
{
	InputDescription desc;
	g_inputManager->getBoundInputDescription( type, desc );
	return desc.noun;
}


// ==================== class ToggleInputTransform ====================

ToggleInputTransform::ToggleInputTransform( ControlType _on, ControlType _off )
: m_on( new ControlEventFunctor( _on ) ), m_off( new ControlEventFunctor( _off ) ),
  m_state( false ), m_change( false ) {}


ToggleInputTransform::ToggleInputTransform( auto_ptr<InputTransform> _on,
                                            auto_ptr<InputTransform> _off )
: m_on( _on ), m_off( _off ), m_state( false ), m_change( false ) {}


void ToggleInputTransform::Advance()
{
	bool On = (*m_on)(), Off = (*m_off)();
	m_state = ( m_state && !Off ) || ( !m_state && On );
	m_change = On || Off;
}


bool ToggleInputTransform::operator()()
{
	return on();
}


bool ToggleInputTransform::on()
{
	Advance();
	return m_state;
}


bool ToggleInputTransform::changed()
{
	Advance();
	return m_change;
}
