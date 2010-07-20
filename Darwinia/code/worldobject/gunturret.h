#ifndef _included_gunturret_h
#define _included_gunturret_h

#include "worldobject/building.h"

#define GUNTURRET_RETARGETTIMER     3.0f
#define GUNTURRET_MINRANGE          100.0f
#define GUNTURRET_MAXRANGE          300.0f
#define GUNTURRET_NUMSTATUSMARKERS  5
#define GUNTURRET_NUMBARRELS        4
#define GUNTURRET_OWNERSHIPTIMER    1.0f


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

    bool IsInView       ();
    void Render         ( float _predictionTime );
    void RenderPorts    ();

    bool DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir,
                    float _rayLen, Vector3 *_pos, Vector3 *norm );

    void ListSoundEvents( LList<char *> *_list );
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
