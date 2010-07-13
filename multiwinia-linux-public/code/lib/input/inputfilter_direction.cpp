#include "lib/universal_include.h"

#include "lib/input/inputfilter_direction.h"


enum CompassDirection {
	DIR_NORTH,
	DIR_SOUTH,
	DIR_WEST,
	DIR_EAST
};


// Apply the filter to a set of InputDetails to get another InputDetails
bool DirectionInputFilter::filter( InputDetailsList const &inDetails,
                                   InputFilterSpec const &filterSpec,
                                   InputDetails &outDetails )
{
	return false;
}


// Return true if the tokens were successfully parsed into an InputFilterSpec
bool DirectionInputFilter::parseFilterSpecification( InputSpecTokens const &tokens,
                                                     InputFilterSpec &spec )
{
	if ( tokens.length() != 3 ) return false;
	if ( tokens[ 0 ] != "direction" ) return false;

	if ( tokens[ 1 ] == "up" )
		spec.mode = DIR_NORTH;
	else if ( tokens[ 1 ] == "down" )
		spec.mode = DIR_SOUTH;
	else if ( tokens[ 1 ] == "left" )
		spec.mode = DIR_WEST;
	else if ( tokens[ 1 ] == "right" )
		spec.mode = DIR_EAST;
	else
		return false;

	if ( tokens[ 2 ] == "down" )
		registerDeltaID( spec );
	else if ( tokens[ 2 ] != "pressed" )
		return false;

	return true;

}


void DirectionInputFilter::calcDetails( InputFilterSpec const &spec, InputDetails &details )
{
	// Um.
}
