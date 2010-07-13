#ifndef _included_markersystem_h
#define _included_markersystem_h

#include "lib/tosser/llist.h"
#include "lib/vector2.h"
#include "lib/vector3.h"
#include "worldobject/worldobject.h"

class LocationMarker;




class MarkerSystem
{
protected:
    LList<LocationMarker *> m_markers;
    WorldObjectId   m_newMarkerUnderMouse;

protected:
    void    FindScreenEdge          ( Vector2 const &line, float *posX, float *posY );
    void    CalculateMarkerPosition ( float screenX, float screenY, float dx, float dy,
                                      float &posX, float &posY );                                

    void    SortMarkers();

public:
    void    RenderShadow            ( float screenX, float screenY, float screenDX, float screenDY, float size, float alpha );
    void    RenderCompass           ( float screenX, float screenY, float screenDX, float screenDY, float size, float alpha );
    void    RenderCompassEmpty      ( float screenX, float screenY, float screenDX, float screenDY, float size, float alpha );
    void    RenderColourCircle      ( float screenX, float screenY, float screenDX, float screenDY, float size, RGBAColour colour, float percent = 1.0 );
    void    RenderIcon              ( float screenX, float screenY, float screenDX, float screenDY, float size, char *iconFilename, RGBAColour colour, float rotation=0.0f );

    void    CalculateScreenBox      ( float screenX, float screenY, float screenDX, float screenDY, float size,
                                      float &minX, float &minY, float &maxX, float &maxY );
    bool    IsMOuseInside           ( float screenX, float screenY, float screenDX, float screenDY, float size );

    bool    CalculateScreenPosition ( Vector3 const &worldPos, float &screenX, float &screenY );             // returns true if onscreen

public:    
    WorldObjectId   m_markerUnderMouse;
    bool            m_isOrderMarkerUnderMouse;

public:
    MarkerSystem();

    void RegisterMarker_Crate   ( WorldObjectId id );
    void RegisterMarker_Statue  ( WorldObjectId id );
	void RegisterMarker_Research( WorldObjectId id );
    void RegisterMarker_Officer ( WorldObjectId id );
    void RegisterMarker_Task    ( WorldObjectId id, bool fromCrate=true );
    void RegisterMarker_Orders  ( WorldObjectId id );
    void RegisterMarker_Fixed   ( int teamId, Vector3 const &pos, char *icon, bool fromCrate=true );

    void Advance();
    void Render();

    void ClearAllMarkers        ();
};



// ============================================================================


class LocationMarker 
{
public:
    enum
    {
        TypeUnknown,
        TypeFixed,
        TypeAttachedToCrate,
        TypeAttachedToTask,
        TypeAttachedToStatue,
		TypeAttachedToResearch,
        TypeAttachedToOfficer,
        TypeAttachedToOrders
    };

    int             m_type;    
    int             m_teamId;
    Vector3         m_pos;
    WorldObjectId   m_objId;
    char            m_icon[256];
    
    bool        m_fromCrate;
    bool        m_active;
    bool        m_hasBeenSeen;
    double      m_onscreenTimer;
    double      m_birthTime;
    float       m_rotation;
    
public:
    LocationMarker();

    bool IsPersistant();            // If true, i will never fade out
    bool ShowOffscreen();           // If true, I will appear at the screen edges
    bool IsSelectable();            
    bool ShouldFlashAlert();        // Means I demand the player's attention, so flash for a bit
    bool AlwaysAtFullAlpha();       
};




#endif
