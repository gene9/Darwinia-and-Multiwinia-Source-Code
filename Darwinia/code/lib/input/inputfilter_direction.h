#ifndef INCLUDED_INPUTFILTER_DIRECTION_H
#define INCLUDED_INPUTFILTER_DIRECTION_H

#include "lib/input/inputfilter_withdelta.h"

// This class takes one 2D analogue input and converts it
// into a set of four queriable buttons, which correspond
// to up, down, left and right buttons

class DirectionInputFilter : public InputFilterWithDelta  {

public:
	// Apply the filter to a set of InputDetails to get another InputDetails
	virtual bool filter( InputDetailsList const &inDetails, InputFilterSpec const &filterSpec,
	                     InputDetails &outDetails );

	// Return true if the tokens were successfully parsed into an InputFilterSpec
	virtual bool parseFilterSpecification( InputSpecTokens const &tokens,
	                                       InputFilterSpec &spec );

	virtual void calcDetails( InputFilterSpec const &spec, InputDetails &details );

};


#endif // INCLUDED_INPUTFILTER_DIRECTION_H
