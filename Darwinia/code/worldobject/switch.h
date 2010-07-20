#ifndef _included_fenceswitch_h
#define _included_fenceswitch_h

#include "worldobject/building.h"


class Shape;
class ShapeFragment;
class ShapeMarker;


class FenceSwitch : public Building
{
protected:
    int         m_linkedBuildingId;
    int         m_linkedBuildingId2;    // optional second link for fence toggling

    bool        m_switchable;

    float       m_timer;                // no fences changes will be made until this timer counts to 0 for the first time

    ShapeMarker *m_connectionLocation;

public:
    char        m_script[256];
    bool		m_locked;
    int			m_lockable;
    int         m_switchValue;

public:
    FenceSwitch 		();

	void Initialise		(Building *_template);
    void SetDetail      ( int _detail );

    bool Advance		();
    void Render			(float predictionTime);
    void RenderAlphas   (float predictionTime);
    void RenderLink     ();
    void RenderLights   ();
    void RenderConnection( Vector3 _targetPos, bool _active );

    void Switch         ();

    int  GetBuildingLink();
    void SetBuildingLink(int _buildingId);

	void Read( TextReader *_in, bool _dynamic );
	void Write( FileWriter *out );

    Vector3 GetConnectionLocation();

    bool IsInView();
};


#endif
