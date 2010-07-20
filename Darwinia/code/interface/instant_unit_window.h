#ifndef _INCLUDED_INSTANT_UNIT_WINDOW_H
#define _INCLUDED_INSTANT_UNIT_WINDOW_H

#ifdef LOCATION_EDITOR

#include "interface/darwinia_window.h"


// ****************************************************************************
// Class InstantUnitEditWindow
// ****************************************************************************

class InstantUnitEditWindow: public DarwiniaWindow
{
public:
	InstantUnitEditWindow(char *name);
	~InstantUnitEditWindow();

	void Create();
};


// ****************************************************************************
// Class InstantUnitCreateWindow
// ****************************************************************************

class InstantUnitCreateWindow: public DarwiniaWindow
{
public:
    InstantUnitCreateWindow( char *name );
	~InstantUnitCreateWindow();

	void Create();
};


#endif // LOCATION_EDITOR

#endif
