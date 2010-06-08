
#ifndef _included_officer_h
#define _included_officer_h

#include "worldobject/entity.h"
#include "worldobject/flag.h"

#define OFFICER_ATTACKRANGE     10.0f
#define OFFICER_ABSORBRANGE     10.0f


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
        OrderGoto,
        OrderFollow,
        NumOrderTypes
    };

    int         m_state;
    Vector3     m_wayPoint;
    int         m_wayPointTeleportId;           // Id of teleport we wish to walk into
    
    int         m_shield;
    bool        m_demoted;
    bool        m_absorb;
    float       m_absorbTimer;
    
    int         m_orders;
    Vector3     m_orderPosition;                // Position in the world
    int         m_ordersBuildingId;             // Id of target building eg Teleport

    ShapeMarker *m_flagMarker;
    Flag        m_flag;
    
protected:
    bool AdvanceIdle                ();
    bool AdvanceToWaypoint          ();
    bool AdvanceGivingOrders        ();
    bool AdvanceToTargetPosition    ();        

    bool SearchForRandomPosition    ();
    
    void Absorb();

    void RenderFlag         ( float _predictionTime );    
    void RenderShield       ( float _predictionTime );
    void RenderSpirit       ( Vector3 const &_pos );

public:
    Officer();
    ~Officer();

    void Begin              ();
    void Render		        ( float _predictionTime );
    bool RenderPixelEffect  ( float _predictionTime );
    
    bool Advance            ( Unit *_unit );

    void ChangeHealth   ( int amount );

    void SetWaypoint    ( Vector3 const &_wayPoint );
    void SetOrders      ( Vector3 const &_orders );
    //void SetDirectOrders( Vector3 const &_orders ); // orders while in direct control mode - removes Goto command

    void SetNextMode     ();
    void SetPreviousMode ();

    void CancelOrderSounds();
    void ListSoundEvents( LList<char *> *_list );

    static char *GetOrderType( int _orderType );
};


class OfficerOrders : public WorldObject
{
public:
    Vector3     m_wayPoint;
    float       m_arrivedTimer;
    
public:
    OfficerOrders();

    bool Advance();
    void Render( float _time );    
};


#endif
