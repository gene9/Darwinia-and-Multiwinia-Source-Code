#include "lib/universal_include.h"

#include "lib/input/inputfiltermanager.h"
#include "lib/input/inputfilterspec.h"
#include "lib/input/input.h"


InputFilterManager *g_inputFilterManager = NULL;


InputFilterManager::InputFilterManager()
: filters() {}


InputFilterManager::~InputFilterManager()
{
	for ( unsigned i = 0; i < filters.size(); ++i )
		if ( filters[ i ] ) delete filters[ i ];
}


void InputFilterManager::addFilter( InputFilter *filter )
{
	if ( filter )
		filters.push_back( filter );
}


bool InputFilterManager::parseFilterSpecString( std::string const &description,
                                                InputFilterSpec &spec,
	                                            std::string &err )
{
	InputSpecTokens tokens( description );
	return parseFilterSpecTokens( tokens, spec, err );
}


bool InputFilterManager::parseFilterSpecTokens( InputSpecTokens const &tokens,
                                                InputFilterSpec &spec,
	                                            std::string &err )
{
	err = "";
	if ( tokens.length() == 0 ) return false;

	for ( unsigned i = 0; i < filters.size(); ++i )
		if ( filters[ i ]->parseFilterSpecification( tokens, spec ) ) {
			spec.filter = i;
			return true;
		}

	return false;
}


bool InputFilterManager::filter( InputSpecList const &inSpecs,
                                 InputFilterSpec const &filterSpec,
	                             InputDetails &outDetails )
{
	InputDetailsList detailsList( inSpecs.size() );
	for ( unsigned i = 0; i < inSpecs.size(); ++i ) {
		InputDetails details;
		g_inputManager->checkInput( *(inSpecs[ i ]), details );
		detailsList.push_back( InputDetailsPtr( new InputDetails( details ) ) );
	}

	return filter( detailsList, filterSpec, outDetails );
}


bool InputFilterManager::filter( InputDetailsList const &inDetails,
                                 InputFilterSpec const &filterSpec,
	                             InputDetails &outDetails )
{
	InputFilter *filter = filters[ filterSpec.filter ];
	return filter->filter( inDetails, filterSpec, outDetails );
}


void InputFilterManager::Advance()
{
	//for ( unsigned i = 0; i < filters.size(); ++i )
	//	filters[ i ]->Advance();
}
