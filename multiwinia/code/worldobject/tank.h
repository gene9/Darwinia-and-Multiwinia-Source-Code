
#ifndef _included_tank_h
#define _included_tank_h

#include "worldobject/entity.h"

class ShapeMarker;
class Shape;

#define TANK_NUMFLOATERS    3
#define TANK_CAPACITY       40
#define TANK_UNLOADPERIOD   0.1


class Tank : public Entity
{
protected:    
    Shape           *m_shapeTurret;
    ShapeMarker     *m_markerTurret;
    ShapeMarker     *m_markerEntrance;
    ShapeMarker     *m_markerBarrelEnd;
    
    Vector3         m_up;
    Vector3         m_turretFront;
    Vector3         m_wayPoint;
    Vector3         m_attackTarget;   
    Vector3         m_missileTarget;
    
    double           m_speed;
    double           m_attackTimer;

    bool            m_missileFired;
   
    enum
    {
        StateCombat,
        StateUnloading,
        StateLoading,
    };
    int             m_state;
    int             m_numPassengers;
    double           m_previousUnloadTimer;

    double           m_mineTimer;

public:
    Tank();
    ~Tank();

    void Begin              ();
    bool Advance            ( Unit *_unit );
    void Render             ( double _predictionTime );
    bool RenderPixelEffect  ( double _predictionTime );

    bool ChangeHealth       ( int _amount, int _damageType = DamageTypeUnresistable );

    void SetWayPoint        ( Vector3 const &_wayPoint );
    void SetAttackTarget    ( Vector3 const &_attackTarget );

    void DropMine           ();

    void SetMissileTarget   ( Vector3 const &_startRay, Vector3 const &_rayDir );
    void SetMissileTarget    ( Vector3 const &_target );
    Vector3 GetMissileTarget();
    void LaunchMissile      ();

    void PrimaryFire        ();

    bool IsLoading          ();
    bool IsUnloading        ();
    void AddPassenger       ();
    void RemovePassenger    ();    

    void GetEntrance( Vector3 &_exitPos, Vector3 &_exitDir ); 

    bool IsSelectable();

    void RunAI( AI *_ai );
    void RunTankBattleAI( AI *_ai );
    void RunStandardAI( AI *_ai );

	inline double GetAttackTimer() { return m_mineTimer; }
};


#endif