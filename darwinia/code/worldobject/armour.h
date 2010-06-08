
#ifndef _included_tank_h
#define _included_tank_h

#include "worldobject/entity.h"
#include "worldobject/flag.h"

class ShapeMarker;
class Shape;

#define ARMOUR_UNLOADPERIOD   0.1f


class Armour: public Entity
{
protected:    
    ShapeMarker     *m_markerEntrance;    
    ShapeMarker     *m_markerFlag;
    Vector3         m_up;
    Vector3         m_conversionPoint;
    float           m_speed;
    
    enum
    {
        StateIdle,
        StateUnloading,
        StateLoading,
    };
    int             m_numPassengers;
    float           m_previousUnloadTimer;
    float           m_newOrdersTimer;

    Flag            m_flag;
    Flag            m_deployFlag;

public:
    Vector3         m_wayPoint;    
    int             m_state;

public:
    Armour();
    ~Armour();

    void Begin              ();
    bool Advance            ( Unit *_unit );
    void Render             ( float _predictionTime );

    void ChangeHealth       ( int _amount );
    
    void SetOrders          ( Vector3 const &_orders );
    void SetWayPoint        ( Vector3 const &_wayPoint );
    void SetConversionPoint ( Vector3 const &_conversionPoint );
    void AdvanceToTargetPos ();
    void DetectCollisions   ();
    void ConvertToGunTurret ();

    void SetDirectOrders    (); // orders while in direct control mode - removes gun turret mode

    bool IsLoading          ();
    bool IsUnloading        ();
    void ToggleLoading      ();
    void AddPassenger       ();
    void RemovePassenger    ();    

    int Capacity();

    void ListSoundEvents    ( LList<char *> *_list );

    void GetEntrance( Vector3 &_exitPos, Vector3 &_exitDir ); 
};


#endif