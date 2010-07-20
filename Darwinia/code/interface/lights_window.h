#ifndef _INCLUDED_LIGHTS_WINDOW_H
#define _INCLUDED_LIGHTS_WINDOW_H

#ifdef LOCATION_EDITOR

#include "interface/darwinia_window.h"


// ****************************************************************************
// Class LightsEditWindow
// ****************************************************************************

class LightsEditWindow: public DarwiniaWindow
{
public:
    LightsEditWindow( char *name );
	~LightsEditWindow();

	void Create();
};

#endif // LOCATION_EDITOR

#endif
