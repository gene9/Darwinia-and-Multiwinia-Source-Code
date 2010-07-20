#ifndef _BUILDINGS_EDIT_WINDOW
#define _BUILDINGS_EDIT_WINDOW

#ifdef LOCATION_EDITOR

#include "interface/darwinia_window.h"


// ****************************************************************************
// Class BuildingEditWindow
// ****************************************************************************

// This window is displayed when a building is selected for editing
class BuildingEditWindow : public DarwiniaWindow
{
public:
    BuildingEditWindow( char *name );
	~BuildingEditWindow();

	void Create();
    void Render( bool hasFocus );
};


// ****************************************************************************
// Class BuildingsCreateWindow
// ****************************************************************************

// Top level window that contains a "create" button for every type of builing
class BuildingsCreateWindow : public DarwiniaWindow
{
public:
    int m_buildingType;

public:
    BuildingsCreateWindow( char *_name );
	~BuildingsCreateWindow();

    void Create();
};


#endif // LOCATION_EDITOR


#endif
