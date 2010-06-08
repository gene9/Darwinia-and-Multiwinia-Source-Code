#include "lib/universal_include.h"

#include "lib/input/inputfilter_withdelta.h"

typedef std::auto_ptr<const InputFilterSpec> FilterSpecPtr;


void InputFilterWithDelta::registerDeltaID( InputFilterSpec &spec )
{
	InputDetails details;
	spec.id = m_oldDetails.size();
	details.type = spec.type;
	details.x = details.y = 0;

	m_specs.push_back( FilterSpecPtr( new InputFilterSpec( spec ) ) );
	m_details.push_back( InputDetailsPtr( new InputDetails( details ) ) );
	m_oldDetails.push_back( InputDetailsPtr( new InputDetails( details ) ) );
}


InputDetails const &InputFilterWithDelta::getOldDetails( filterspec_id_t id ) const
{
	if ( 0 <= id && id < m_oldDetails.size() )
		return *(m_oldDetails[ id ]);
	else
		throw "Invalid delta index.";
}


InputDetails const &InputFilterWithDelta::getOldDetails( InputFilterSpec const &spec ) const
{
	return getOldDetails( spec.id );
}


InputDetails const &InputFilterWithDelta::getDetails( filterspec_id_t id ) const
{
	if ( 0 <= id && id < m_details.size() )
		return *(m_details[ id ]);
	else
		throw "Invalid delta index.";
}


InputDetails const &InputFilterWithDelta::getDetails( InputFilterSpec const &spec ) const
{
	return getDetails( spec.id );
}


int InputFilterWithDelta::getDelta( filterspec_id_t id, InputDetails &delta ) const
{
	InputDetails const &details = getDetails( id );
	InputDetails const &oldDetails = getOldDetails( id );

	if ( details.type == oldDetails.type ) {
		delta.type = details.type;
		delta.x = details.x - oldDetails.x;
		delta.y = details.y - oldDetails.y;
	} else if ( details.type != INPUT_TYPE_FAIL && oldDetails.type == INPUT_TYPE_FAIL ) {
		delta.type = details.type;
		delta.x = details.x;
		delta.y = details.y;
		return 1;
	} else if ( details.type == INPUT_TYPE_FAIL && oldDetails.type != INPUT_TYPE_FAIL ) {
		delta.type = oldDetails.type;
		delta.x = -oldDetails.x;
		delta.y = -oldDetails.y;
		return -1;
	} else if ( details.type == INPUT_TYPE_FAIL && oldDetails.type == INPUT_TYPE_FAIL ) {
		delta.type = INPUT_TYPE_FAIL;
		delta.x = delta.y = 0;
	}
	return 0;
}


int InputFilterWithDelta::getDelta( InputFilterSpec const &spec, InputDetails &delta ) const
{
	filterspec_id_t id = spec.id;
	return getDelta( id, delta );
}


void InputFilterWithDelta::ageDetails()
{
	m_details.swap( m_oldDetails ); // auto_vector has no assignment operator
}


void InputFilterWithDelta::Advance()
{
	InputDetails details;
	for ( unsigned i = 0; i < m_specs.size(); ++i ) {
		calcDetails( *(m_specs[ i ]), details );
		//m_details[ i ] = InputDetailsPtr( new InputDetails( details ) );
		InputDetails &currDetails = *(m_details[ i ]);
		currDetails.type = details.type;
		currDetails.y = details.x;
		currDetails.x = details.y;
	}
}
