#include "lib/universal_include.h"
#include <float.h>

#include "lib/math_utils.h"
#include "lib/debug_utils.h"
#include "lib/vector2.h"
#include "lib/debug_render.h"

#include "app.h"
#include "landscape.h"
#include "routing_system.h"
#include "location.h"

#include "worldobject/radardish.h"


// ****************************************************************************
// Class WayPoint
// ****************************************************************************

// *** Constructor
WayPoint::WayPoint(int _type, Vector3 const &_pos)
:	m_type(_type),
	m_pos(_pos),
	m_buildingId(-1)
{
}


// *** Destructor
WayPoint::~WayPoint()
{
}


// *** GetPos
// I could have made it so that the Y values for GroundPoses were set once at
// level load time, but that would require an init phases after level file
// parsing and landscape terrain generation.
Vector3 WayPoint::GetPos()
{
	Vector3 rv(m_pos);

	if (m_type == TypeGroundPos)
	{
		Landscape *land = &g_app->m_location->m_landscape;
		rv.y = land->m_heightMap->GetValue(rv.x, rv.z);
		if (rv.y < 0.0f)
		{
			rv.y = 0.0f;
		}
		rv.y += 1.0f;
	}
	else if (m_type == TypeBuilding)
	{
		Building *building = g_app->m_location->GetBuilding(m_buildingId);
        if( building )
        {
            DarwiniaDebugAssert( building->m_type == Building::TypeRadarDish ||
                         building->m_type == Building::TypeBridge );
            Teleport *teleport = (Teleport *) building;
            Vector3 pos, front;
            teleport->GetEntrance( pos, front );
		    return pos;
        }
	}

	return rv;
}


// *** SetPos
void WayPoint::SetPos(Vector3 const &_pos)
{
	m_pos = _pos;
}


// ****************************************************************************
// Class Route
// ****************************************************************************

// *** Constructor
Route::Route(int _id)
:	m_id(_id)
{
}


// *** Destructor
Route::~Route()
{
	m_wayPoints.EmptyAndDelete();
}


void Route::AddWayPoint(Vector3 const &_pos)
{
    WayPoint *wayPoint = new WayPoint( WayPoint::Type3DPos, _pos );
    m_wayPoints.PutDataAtEnd( wayPoint );
}


void Route::AddWayPoint(int _buildingId)
{
    WayPoint *wayPoint = new WayPoint( WayPoint::TypeBuilding, g_zeroVector );
    wayPoint->m_buildingId = _buildingId;
    m_wayPoints.PutDataAtEnd( wayPoint );
}


WayPoint *Route::GetWayPoint(int _id)
{
    if( m_wayPoints.ValidIndex(_id) )
    {
        WayPoint *wayPoint = m_wayPoints[_id];
        return wayPoint;
    }

    return NULL;
}


int	Route::GetIdOfNearestWayPoint(Vector3 const &_pos)
{
	int idOfNearest = -1;
	float distToNearestSqrd = FLT_MAX;

	int size = m_wayPoints.Size();
	for (int i = 0; i < size; ++i)
	{
		WayPoint *wp = m_wayPoints.GetData(i);
		Vector3 delta = _pos - wp->GetPos();
		float distSqrd = delta.MagSquared();
		if (distSqrd < distToNearestSqrd)
		{
			distToNearestSqrd = distSqrd;
			idOfNearest = i;
		}
	}

	return idOfNearest;
}


// Returns the id of the first waypoint of the nearest edge
int	Route::GetIdOfNearestEdge(Vector3 const &_pos, float *_dist)
{
	int idOfNearest = 0;
	float distToNearest = FLT_MAX;

	Vector2 pos(_pos.x, _pos.z);
	WayPoint *wp = m_wayPoints.GetData(0);
	Vector3 newPos = wp->GetPos();
	Vector2 oldPos( newPos.x, newPos.z );

	int size = m_wayPoints.Size();
	for (int i = 1; i < size; ++i)
	{
		wp = m_wayPoints.GetData(i);
		newPos = wp->GetPos();
		Vector2 temp(newPos.x, newPos.z);
		float dist = PointSegDist2D(pos, oldPos, temp);
		oldPos = temp;

		if (dist < distToNearest)
		{
			distToNearest = dist;
			idOfNearest = i;
		}
	}

	return idOfNearest - 1;
}


void Route::Render()
{
    if( !g_app->m_location ) return;

#ifdef DEBUG_RENDER_ENABLED
    Vector3 lastPos;

    glDisable( GL_DEPTH_TEST );

    for( int i = 0; i < m_wayPoints.Size(); ++i )
    {
        WayPoint *wayPoint = m_wayPoints[i];
        Vector3 thisPos = wayPoint->GetPos();
        if( i > 0 )
        {
            RenderArrow( lastPos, thisPos, 5.0f, RGBAColour(255,100,100,255) );
        }
        lastPos = thisPos;
    }

    glEnable( GL_DEPTH_TEST );
#endif
}

