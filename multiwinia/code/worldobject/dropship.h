#ifndef _included_dropship_h
#define _included_dropship_h

#include "worldobject/worldobject.h"
#include "worldobject/entity.h"

class DropShip : public Entity
{
public:
    enum
    {
        DropShipStateArriving,
        DropShipStateDropping,
        DropShipStateLeaving
    };

    Vector3     m_waypoint;
    int         m_state;
    int         m_numPassengers;
    double       m_scale;
    double       m_speed;
    WorldObjectId m_lastDrop;

    DropShip();

    bool Advance( Unit *_unit );
    void Render( double _predictionTime );
    void RenderBeam( double _predictionTime );

    void SetWaypoint( const Vector3 _waypoint );
    bool AdvanceToTargetPos();

    bool ChangeHealth( int _amount, int _damageType = DamageTypeUnresistable );
};

#endif