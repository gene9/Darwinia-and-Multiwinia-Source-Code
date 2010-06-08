#ifndef _included_teleport_h
#define _included_teleport_h

#include "lib/vector3.h"

#include "worldobject/building.h"

class Entity;
     
struct TeleportMap
{
    int m_teamId;
    int m_fromUnitId;
    int m_toUnitId;
};


class Teleport : public Building
{   
protected:

    float       m_timeSync;
    float       m_sendPeriod;                                   // Minimum time between sends
    static      LList<TeleportMap> m_teleportMap;               // Maps units going in onto units comingout

    ShapeMarker *m_entrance;

protected:
    
    void RenderSpirit   ( Vector3 const &_pos, int _teamId );

public:
    LList <WorldObjectId> m_inTransit;                         // Entities on the move

public:
    Teleport();

    void SetShape       ( Shape *_shape );

    bool Advance        ();
    void RenderAlphas   ( float predictionTime );
    
    void EnterTeleport  ( WorldObjectId _id, bool _relay=false );      // Relay=true means i've entered directly from another teleport

    bool IsInView       ();

    virtual bool        Connected();
    virtual bool        ReadyToSend ();
    
    virtual bool        GetEntrance ( Vector3 &_pos, Vector3 &_front );
    virtual bool        GetExit     ( Vector3 &_pos, Vector3 &_front );
    
    virtual Vector3     GetStartPoint();
    virtual Vector3     GetEndPoint();
    
    virtual bool        UpdateEntityInTransit( Entity *_entity );      // Returns true (remove me) or false (still inside)

};


#endif
