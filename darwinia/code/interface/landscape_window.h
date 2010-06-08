#ifndef _INCLUDED_LANDSCAPE_WINDOW_H
#define _INCLUDED_LANDSCAPE_WINDOW_H

#ifdef LOCATION_EDITOR

#include "interface/darwinia_window.h"


class LandscapeFlattenArea;
class LandscapeTile;


// ****************************************************************************
// Class LandscapeTileEditWindow
// ****************************************************************************

class LandscapeTileEditWindow: public DarwiniaWindow
{
public:
	LandscapeTile        	*m_tileDef;
    int						m_tileId;

    LandscapeTileEditWindow( char *name, int tileId );
    ~LandscapeTileEditWindow();

	void					Create();
};


// ****************************************************************************
// Class LandscapeFlatAreaEditWindow
// ****************************************************************************

class LandscapeFlattenAreaEditWindow: public DarwiniaWindow
{
public:
	LandscapeFlattenArea	*m_areaDef;
    int						m_areaId;

    LandscapeFlattenAreaEditWindow(char *_name, int areaId);
    ~LandscapeFlattenAreaEditWindow();

	void					Create();
};


// ****************************************************************************
// Class LandscapeEditWindow
// ****************************************************************************

class LandscapeEditWindow: public DarwiniaWindow
{
public:
    LandscapeEditWindow( char *name );
    ~LandscapeEditWindow();

	void					Create();
};


// ****************************************************************************
// Class LandscapeGuideGridWindow
// ****************************************************************************

class LandscapeGuideGridWindow : public DarwiniaWindow
{
public:
	LandscapeTile       	*m_tileDef;
    int						m_tileId;
    int                     m_guideGridPower;
    int                     m_pixelSizePerSample;

    enum
    {
        GuideGridToolFreehand,
        GuideGridToolFlatten,
        GuideGridToolBinary
    };
    int                     m_tool;
    float                   m_toolSize;

    LandscapeGuideGridWindow( char *name, int tileId );
    ~LandscapeGuideGridWindow();

    void                    Create();
};


#endif // LOCATION_EDITOR


#endif
