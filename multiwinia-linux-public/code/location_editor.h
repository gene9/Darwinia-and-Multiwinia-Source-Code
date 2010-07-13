#ifndef _included_location_editor_h
#define _included_location_editor_h

#ifdef LOCATION_EDITOR

#include "lib/vector3.h"


class InstantUnit;
class LandscapeTile;


class LocationEditor
{
public:
    int             m_landscapeGrabX;
    int             m_landscapeGrabZ;
    int             m_newLandscapeX;                            // Used for landscape dragging
    int             m_newLandscapeZ;
	int				m_mode;
        
    int             m_moveBuildingsWithLandscape;
    
	enum
	{
		ToolNone,
        ToolMove,
        ToolRotate,
        ToolLink,
		ToolCreate,
		ToolNumTypes			
	};

	enum
	{
		ModeNone,				// 0
		ModeLandTile,			// 1
		ModeLandFlat,			// 2
		ModeBuilding,			// 3
		ModeLight,				// 4
		ModeInstantUnit,		// 5
        ModeCameraMount,        // 6
        ModeMultiwinia,         // 7
        ModeInvalidCrates,
		ModeNumModes
	};

private:
	bool			m_waitingForRelease;

	void			CreateEditWindowForMode (int _mode);

	int				DoesRayHitBuilding      (Vector3 const &rayStart, Vector3 const &rayDir);
	int				DoesRayHitInstantUnit   (Vector3 const &rayStart, Vector3 const &rayDir);
	int				DoesRayHitCameraMount	(Vector3 const &rayStart, Vector3 const &rayDir);
	int				IsPosInLandTile         (Vector3 const &pos);
	int				IsPosInFlattenArea      (Vector3 const &pos);

	void			AdvanceModeNone();
	void			AdvanceModeLandTile();
	void			AdvanceModeLandFlat();
	void			AdvanceModeBuilding();
	void			AdvanceModeLight();
	void			AdvanceModeInstantUnit();
	void			AdvanceModeCameraMount();

	void			RenderUnit(InstantUnit *_iu);
	
	void			RenderModeLandTile();
	void			RenderModeLandFlat();
	void			RenderModeBuilding();
	void			RenderModeInstantUnit();
	void			RenderModeCameraMount();

    void            MoveBuildingsInTile( LandscapeTile *_tile, float _dX, float _dZ );

public:
	int             m_tool;			// == TypeNone if no object selected
	int				m_selectionId;	// == -1 if no object selected
	int				m_subSelectionId; // Some modes need a second selection ID, eg ModeRoute
									  // where one ID specifies the route, the other the way
									  // point within that route.

	LocationEditor();
	~LocationEditor();
       
	void RequestMode(int _mode);
	int GetMode();

	void Advance();
	void Render();
};


#endif // LOCATION_EDITOR


#endif
