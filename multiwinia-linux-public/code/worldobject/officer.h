
#ifndef _included_officer_h
#define _included_officer_h

#include "worldobject/entity.h"
#include "worldobject/flag.h"

#include "lib/tosser/darray.h"

#define OFFICER_ATTACKRANGE     10.0
#define OFFICER_ABSORBRANGE     10.0


class OfficerFormation
{
public:
    int   m_entityUniqueId;
    double m_timer;
};


class Officer : public Entity
{
public:
    enum
    {
        StateIdle,
        StateToWaypoint,
        StateGivingOrders
    };

    enum
    {
        OrderNone,
        OrderPrepareGoto,
        OrderGoto,
        OrderFollow,
        NumOrderTypes
    };

    int         m_state;
    Vector3     m_wayPoint;
    int         m_wayPointTeleportId;           // Id of teleport we wish to walk into

    Vector3     m_targetFront;                  // used by the ai in formation mode
    bool        m_turnToTarget;
    
    int         m_shield;
    bool        m_demoted;
    bool        m_absorb;
    double       m_absorbTimer;
    bool        m_noFormations;
    
    int         m_orders;
    Vector3     m_orderPosition;                // Position in the world
    int         m_ordersBuildingId;             // Id of target building eg Teleport
    int         m_orderRouteId;
    double       m_lastOrdersSet;
    double       m_lastOrderCreated;

    bool        m_formation;
    DArray      <OfficerFormation> m_formationEntities;
    bool        m_formationAngleSet;

    ShapeMarker *m_flagMarker;
    Flag        m_flag;
    
protected:
    bool AdvanceIdle                ();
    bool AdvanceToWaypoint          ();
    bool AdvanceGivingOrders        ();
    bool AdvanceToTargetPosition    ();        
    bool AdvanceToTargetPositionInFormation();
    bool SearchForRandomPosition    ();
    
    void Absorb();

    void RenderShield       ( double _predictionTime );
    void RenderSpirit       ( Vector3 const &_pos );

public:
    Officer();
    ~Officer();

    void Begin              ();
    void Render		        ( double _predictionTime );
    bool RenderShape        ( double _predictionTime );
    bool RenderPixelEffect  ( double _predictionTime );
    void RenderFlag         ( double _predictionTime );    

    bool Advance            ( Unit *_unit );

    void RunAI( AI *_ai );

    bool ChangeHealth( int _amount, int _damageType = DamageTypeUnresistable );

    void SetWaypoint    ( Vector3 const &_wayPoint );
    void SetOrders      ( Vector3 const &_orders, bool directRoute=false );
    //void SetDirectOrders( Vector3 const &_orders ); // orders while in direct control mode - removes Goto command

    bool IsFormationToggle( Vector3 const &mousePos );
    void SetFormation   ( Vector3 const &targetPos );

	int GetNextMode		 ();
	int GetPreviousMode	 ();

    void SetNextMode     ();
    void SetPreviousMode ();

	void CancelOrders	 ();
	void SetFollowMode	 ();

    bool IsSelectable       ();

    int     GetFormationIndex( int _uniqueId );
    Vector3 GetFormationPosition( int _uniqueId );
    static Vector3 GetFormationPositionFromIndex( Vector3 const &pos, int positionIndex, Vector3 const &front, int numEntities );

    bool    FormationFull();
    bool    IsInFormationMode();
    bool    IsInFormation( int _uniqueId );
    void    RegisterWithFormation( int _uniqueId );

	bool	IsThereATeleportClose(Vector3 const &_orders);

    void CancelOrderSounds();
    void ListSoundEvents( LList<char *> *_list );

    static char *GetOrderType( int _orderType );

    void    CalculateBoundingSphere( Vector3 &centre, double &radius );
    char            *LogState( char *_message = NULL );
};


class OfficerOrders : public WorldObject
{
public:
    Vector3     m_wayPoint;
    double       m_arrivedTimer;
    
public:
    OfficerOrders();

    bool Advance();
    void Render( double _time );    
};



class MultiwiniaOfficerOrders : public WorldObject
{
public:
    Vector3     m_wayPoint;
    int         m_routeId;
    int         m_routeWayPointId;
    double       m_arrivedTimer;
    double       m_dropArrowTimer;

public:
    MultiwiniaOfficerOrders();

    bool Advance();
    void FollowRoute();
    void Render( double _time );    
};


class OfficerOrderTrail : public WorldObject
{
public:
    double   m_birthTime;
    Vector3 m_front;
    Vector3 m_right;

public:
    OfficerOrderTrail();

    bool Advance();
    void Render( double _time );
};


#endif
