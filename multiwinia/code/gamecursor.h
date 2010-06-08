#ifndef _included_gamecursor_h
#define _included_gamecursor_h

/*
    Renders all mouse cursors of any kind in-game.
    Responsible for figuring out which mouse cursor to render.
    Also renders selection arrows to inform the player which units are selected

 */


#include "lib/rgb_colour.h"
#include "worldobject/worldobject.h"

class MouseCursor;
class MouseCursorMarker;
class Entity;
class Task;


// ============================================================================


class GameCursor
{
protected:
    MouseCursor *m_cursorStandard;
    MouseCursor *m_cursorHighlight;
    MouseCursor *m_cursorSelection;
    MouseCursor *m_cursorMoveHere;
    MouseCursor *m_cursorDisabled;
    MouseCursor *m_cursorMissile;

    char        m_selectionArrowFilename[256];
    char        m_selectionArrowShadowFilename[256];
	float		m_selectionArrowBoost;

	bool		m_highlightingSomething;
	bool		m_validPlacementOpportunity; 
	bool		m_moveableEntitySelected;

    LList<MouseCursorMarker *>  m_markers;

public:
    WorldObjectId   m_objUnderMouse;
    MouseCursor     *m_cursorEnter;
    MouseCursor     *m_cursorPlacement;
    MouseCursor     *m_cursorTurretTarget;

protected:
    void RenderMarkers          ();
    void FindScreenEdge         ( Vector2 const &_line, float *_posX, float *_posY );

    void RenderSelectedIcon     ( char *filename );
    void RenderSelectedUnitIcon ( Entity *entity );    
    void RenderSelectedTaskIcon ( Task *task );
    void RenderSelectedDGsIcon  ( int count );
    void RenderTryingToEnterEffect();
	    
public:
    GameCursor();
	~GameCursor();

    void CreateMarker           ( Vector3 const &_pos );
	void BoostSelectionArrows	( float _seconds );	// Show selection arrows for a specified time
    
    void Render();
    void RenderBasicCursorAlwaysOnTop();
    void RenderSelectedObjectNearCursor();
	bool RenderCrosshair();
    void RenderMultiwinia();
    void RenderSelectionCircle();
    void RenderFormation( Vector3 const &pos, Vector3 const &front );
    void RenderUnitHighlightEffect( Vector3 const &pos, float radius, RGBAColour colour );
    void RenderRing     ( Vector3 const &centrePos, float innerRadius, float outerRadius, int numSteps, bool forceAttachToGround=false );
    void RenderCyliner  ( Vector3 const &centrePos, float radius, float height, int numSteps, RGBAColour colour, bool forceAttachToGround=false );

    void RenderStandardCursor   ( float _screenX, float _screenY );
    void RenderWeaponMarker     ( Vector3 _pos, Vector3 _front, Vector3 _up );

	bool AdviseHighlightingSomething() { return m_highlightingSomething; };
	bool AdvisePlacementOpportunity()  { return m_validPlacementOpportunity; };
	bool AdviseMoveableEntitySelected() { return m_moveableEntitySelected; };

    void RenderSelectionArrows  ( WorldObjectId _id, Vector3 &_pos, bool darwinians = false );
    void RenderSelectionArrow   ( float _screenX, float _screenY, float _screenDX, float _screenDY, float _size, float _alpha );
    void RenderOffscreenArrow   ( Vector3 &fromPos, Vector3 &toPos, float alpha );

	bool GetSelectedObject      ( WorldObjectId &_id, Vector3 &_pos, float &_radius );
	bool GetHighlightedObject   ( WorldObjectId &_id, Vector3 &_pos, float &_radius );        
};


// ============================================================================

class MouseCursorMarker
{
public:
    Vector3     m_pos;
    Vector3     m_front;
    Vector3     m_up;
    double      m_startTime;
};


// ============================================================================

class MouseCursor
{
protected:
	char		*m_mainFilename;
	char		*m_shadowFilename;

	float	    m_size;
	bool	    m_animating;
    bool        m_shadowed;
	float	    m_hotspotX;		// 0,0 means hotspot is in top left of bitmap
	float	    m_hotspotY;		// 1,1 means hotspot is in bottom right of bitmap
    RGBAColour  m_colour;
    
public:
    bool        m_additive;

public:    
    MouseCursor         (char const *_filename);
	~MouseCursor        ();

	void SetSize        (float _size);
    float GetSize       ();
	void SetAnimation   (bool _onOrOff);
    void SetShadowed    (bool _onOrOff);
	void SetHotspot     (float x, float y);
    void SetColour      (const RGBAColour &_col );
	
	void Render         (float _x, float _y, float _rotateAngle=0.0f);
    void Render3D       (Vector3 const &_pos, bool _cameraScale=true );
    void Render3D       (Vector3 const &_pos, Vector3 const &_front, Vector3 const &_up, bool _cameraScale=true);
};


#endif
