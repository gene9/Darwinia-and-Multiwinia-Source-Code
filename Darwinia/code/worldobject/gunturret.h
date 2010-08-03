#ifndef _included_gunturret_h
#define _included_gunturret_h

#include "worldobject/building.h"

#define GUNTURRET_RETARGETTIMER     3.0f
//#define GUNTURRET_MINRANGE          100.0f
//#define GUNTURRET_MAXRANGE          300.0f
#define GUNTURRET_NUMSTATUSMARKERS  5
#define GUNTURRET_NUMBARRELS        4
#define GUNTURRET_OWNERSHIPTIMER    1.0f

#define GUNTURRET_MINRANGE          100.0 
#define GUNTURRET_MAXRANGE          300.0
#define MORTARTURRET_MINRANGE       100.0 
#define MORTARTURRET_MAXRANGE       200.0
#define FLAMETURRET_MINRANGE        10.0
#define FLAMETURRET_MAXRANGE        150.0
#define ROCKETTURRET_MINRANGE       100.0
#define ROCKETTURRET_MAXRANGE       200.0
#define SUBVERSIONTURRET_MINRANGE   10.0f
#define SUBVERSIONTURRET_MAXRANGE   150.0f
#define LASERTURRET_MINRANGE		10.0f
#define LASERTURRET_MAXRANGE		150.0f

#define GUNTURRET_DEFAULT_TYPE		0		// See the enum further down for possible values here
class GunTurret : public Building
{
protected:
    Shape           *m_turret;
    Shape           *m_barrel;
    ShapeMarker     *m_barrelMount;
    ShapeMarker     *m_barrelEnd[GUNTURRET_NUMBARRELS];
    ShapeMarker     *m_statusMarkers[GUNTURRET_NUMSTATUSMARKERS];

    Vector3         m_turretFront;
    Vector3         m_barrelUp;

    bool            m_aiTargetCreated;

    Vector3         m_target;
    WorldObjectId   m_targetId;
    float           m_fireTimer;
    int             m_nextBarrel;
    float           m_retargetTimer;
    bool            m_targetCreated;

    float           m_health;
    float           m_ownershipTimer;

public:
    int             m_state;
	    // used for the grenade turret
    Vector3         m_targetPos;
    double          m_targetRadius; 
    double          m_targetForce;
    double          m_targetAngle;

    enum
    {
        GunTurretTypeStandard = 0,
        GunTurretTypeRocket,
        GunTurretTypeMortar,
        GunTurretTypeFlame,
        GunTurretTypeSubversion,
		GunTurretTypeLaser,
        NumGunTurretTypes
    };

protected:
    void PrimaryFire();
    bool SearchForTargets();
    void SearchForRandomPos();
    void RecalculateOwnership();

public:
    GunTurret();

    void Initialise     ( Building *_template );
    void SetDetail      ( int _detail );

    void ExplodeBody    ();
    void Damage         ( float _damage );
    bool Advance        ();

    Vector3 GetTarget   ();
    void SetTarget( Vector3 _target );

    bool IsInView       ();
    void Render         ( float _predictionTime );
    void RenderPorts    ();

    bool DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir,
                    float _rayLen, Vector3 *_pos, Vector3 *norm );

    void ListSoundEvents( LList<char *> *_list );

    static char *GetTurretTypeName( int _type );
    static char *GetTurretTypeShortName( int _type );
    static char *GetTurretModelName( int _type );

    double GetReloadTime();
    double GetMinRange();
    double GetMaxRange();

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( FileWriter *_out );	

};


class GunTurretTarget : public WorldObject
{
public:
    int m_buildingId;

public:
    GunTurretTarget ( int _buildingId );
    bool Advance    ();
    void Render     ( float _time );
};


#endif
