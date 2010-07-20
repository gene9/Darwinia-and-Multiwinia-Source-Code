#ifndef _INCLUDED_CAMERA_MOUNT_WINDOW_H
#define _INCLUDED_CAMERA_MOUNT_WINDOW_H


#ifdef LOCATION_EDITOR

#include "interface/darwinia_window.h"



// ****************************************************************************
// Class CameraMountEditWindow
// ****************************************************************************

class CameraMountEditWindow: public DarwiniaWindow
{
public:
    CameraMountEditWindow( char *name );
	~CameraMountEditWindow();

	void Create();
};


#endif // LOCATION_EDITOR

#endif
