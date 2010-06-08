#ifndef _included_carryablebuilding_h
#define _included_carryablebuilding_h

#include "worldobject/building.h"



class CarryableBuilding : public Building
{    
public:
    double   m_carryRadius;    
    double   m_scale;
    int     m_numLifters;
    int     m_numLiftersThisFrame;
    int     m_minLifters;
    int     m_maxLifters;
    bool    m_lifted;
    double  m_recountTimer;
    
    Vector3 m_waypoint;                                     
    int     m_routeId;
    int     m_routeWayPointId;

    double   m_speedScale;
    
    Vector3 m_requestedWaypoint;
    int     m_requestedRouteId;
    int     m_numRequests;

public:
    CarryableBuilding();

    void Initialise     ( Building *_template );
    void SetShape       ( Shape *_shape );
    
    bool Advance();
    bool IsInView();
    void Render( double _predictionTime );

    void FollowRoute();

    void CalculateOwnership();

    Vector3 GetCarryPosition        ( int _uniqueId );
    double   GetCarryPercentage      ();

    int *CalculateCollisions        ( Vector3 const &_pos, int &_numCollisions, double &_collisionFactor );         // with other carryable buildings

    virtual void SetWaypoint        ( Vector3 const &_waypoint, int routeId );

    virtual void HandleCollision    ( double _force );

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);

    void ListSoundEvents        ( LList<char *> *_list );

    double GetSpeedScale();

    static bool IsCarryableBuilding( int _type );
};


#endif