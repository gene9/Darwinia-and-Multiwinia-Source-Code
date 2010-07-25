#ifndef _included_snowbox_h
#define _included_snowbox_h

#include "location.h"

#include "worldobject/entity.h"


class SnowBox : public Entity
{
public:
	SnowBox();
	void ChangeHealth ( int _amount );
};


#endif
