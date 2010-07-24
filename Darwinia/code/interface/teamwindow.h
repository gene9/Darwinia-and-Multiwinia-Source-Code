#ifndef _TEAM_EDIT_WINDOW
#define _TEAM_EDIT_WINDOW

#ifdef LOCATION_EDITOR

#include "interface/darwinia_window.h"


// ****************************************************************************
// Class BuildingEditWindow
// ****************************************************************************

// This window is displayed when a building is selected for editing
class TeamEditWindow : public DarwiniaWindow
{
public:
    TeamEditWindow( char *name );
	~TeamEditWindow();

	void Create();
};

#endif // LOCATION_EDITOR


#endif
