
#ifndef _included_armour_h
#define _included_armour_h

#include "worldobject/entity.h"
#include "worldobject/flag.h"

class ShapeMarker;
class Shape;
class AI;
class Building;

#define ARMOUR_UNLOADPERIOD   0.1


class Armour: public Entity
{
private:
    double           m_lastOrdersSet;

protected:    
    ShapeMarker     *m_markerEntrance;
/*
    ShapeMarker     *m_markerFlag;
*/
    Vector3         m_pickupPoint;
    double           m_speed;
    
    int             m_numPassengers;
    int             m_registeredPassengers;
    int             m_passengerOverflow;
    double          m_previousUnloadTimer;
    double          m_newOrdersTimer;

    Flag            m_flag;
    Flag            m_deployFlag;

    int             m_suicideBuildingId;
    int             m_currentAIObjective;
    bool            m_turretRushing;

    double          m_recheckPathTimer;

    Vector3         m_smoothFront;
    Vector3         m_smoothUp;
    double           m_smoothYpos;

public:

    enum
    {
        StateIdle,
        StateUnloading,
        StateLoading,
    };

    Vector3         m_wayPoint;    
    int             m_state;
    Vector3         m_up;

	bool			m_awaitingDeployment;
    Vector3         m_conversionPoint;
    int             m_conversionOrder;
    int             m_conversionToggleDirection;

    static  LList<int>  s_armourObjectives;

protected:
    bool PreventLoadOverlap (); // make sure we arent loading while another armour nearby is already doing so; return true if we find an armour already loading
    void FindIdleDarwinians (); // scan for idle darwinians for loading into the rocket, and go pick them up
    void RRFindPickupPoint  (); // look for somewhere to collect darwinians from (trunkport or spawn point)
    bool RRValidPickupPoint ( Vector3 const &_pos ); // make sure there are no other armours on teh way to collect from this position
    void RRFindDropoffPoint ( AI *_ai ); // find somewhere to drop our darwinians (at objectives representing solar panels)
    void RunRocketRiotAI    ( AI *_ai );

    void RunAssaultAI       ( AI *_ai );
    void RunStandardAI      ( AI *_ai );
    void RunBlitzkriegAI    ( AI *_ai );
    void RunMobileBombAI    ( AI *_ai );    // for whatever reason (Assault mode generally), there is nothing for the armour to do, so find a good target and go explode on it

    int  CountApproachingDarwinians();
    void GetBlitzkriegDropPoint( Building *_b );

	bool IsRouteClear		( Vector3 const &_pos );
	void CreateRoute		( Vector3 const &_pos );

    void FindPickupPoint    ( AI *_ai);
    void AISetConversionPoint( Vector3 const &_pos );

public:
    Armour();
    ~Armour();

    void Begin              ();
    bool Advance            ( Unit *_unit );
    void Render             ( double _predictionTime );

    void RunAI              ( AI *_ai );

    bool ChangeHealth( int _amount, int _damageType = DamageTypeUnresistable );
    
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

    void ToggleConversionAction();

    int Capacity();
    int GetNumPassengers    ();
    double GetSpeed();

    bool IsSelectable       ();

    void ListSoundEvents    ( LList<char *> *_list );

    void GetEntrance( Vector3 &_exitPos, Vector3 &_exitDir ); 

    void RegisterPassenger();
    void FollowRoute();

    bool RayHit( Vector3 const &rayStart, Vector3 const &rayDir );

	static bool IsRouteClear( Vector3 const &_fromPos, Vector3 const &_toPos );
};


#endif
