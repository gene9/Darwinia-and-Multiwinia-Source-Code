#ifndef INCLUDED_ROUTING_SYSTEM_H
#define INCLUDED_ROUTING_SYSTEM_H


#include "lib/llist.h"
#include "lib/vector3.h"


// ****************************************************************************
// Class WayPoint
// ****************************************************************************

class WayPoint
{
protected:
	Vector3				m_pos;

public:
	enum
	{
		Type3DPos,
		TypeGroundPos,
		TypeBuilding
	};

	int					m_type;
	int					m_buildingId;

	WayPoint			(int _type, Vector3 const &_pos);
	~WayPoint			();

	Vector3				GetPos();
	void				SetPos(Vector3 const &_pos);
};



// ****************************************************************************
// Class Route
// ****************************************************************************

class Route
{
public:
	int			m_id;
	LList       <WayPoint *>m_wayPoints;

	Route		(int _id);
	~Route		();

    void        AddWayPoint             (Vector3 const &_pos);
    void        AddWayPoint             (int _buildingId);
    WayPoint    *GetWayPoint            (int _id);

	int			GetIdOfNearestWayPoint  (Vector3 const &_pos);
	int			GetIdOfNearestEdge      (Vector3 const &_pos, float *_dist);

    void        Render();
};


#endif
