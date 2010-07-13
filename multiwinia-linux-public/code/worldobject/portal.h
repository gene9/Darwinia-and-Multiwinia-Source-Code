#ifndef _included_portal_h
#define _included_portal_h

#include "worldobject/building.h"
#include "worldobject/worldobject.h"

#define NUM_TENTACLES 6

class Portal : public Building
{
public:

    enum
    {
        SummonStateInactive,            // no final summon yet
        SummonStateLoweringPortal,      // the portal is lowering to the ground
        SummonStatePortalExpand,        // The portal is expanding
        SummonStateThrash,              // the tentacles are at full size and thrasing around
        SummonStateExplode,              // the portal explodes, killing the entire team
        SummonStateCoolDown,            // explosions done, so wait a little while then eliminate the team
        SummonStateFinished             // the end, nothing else
    };

    int         m_finalSummonState;
    double       m_portalRadius;

protected:    
    double       m_explosionTimer;
    Vector3     m_portalCentre;

protected:

    void AdvanceFinalSummon     ();

    void AdvanceStateLowering   ();
    void AdvanceStateExpanding  ();
    void AdvanceStateThrashing  ();
    void AdvanceStateExploding  ();
    void AdvanceStateCooldown   ();

    void RenderExplosionBuildup ( double _predictionTime );
    void RenderBeam             ();

public:
    Portal();

    bool Advance                ();
    void RenderAlphas           ( double _predictionTime );
    static void RenderBeam      ( Vector3 _from, Vector3 _to );

    void SpecialSummon          ( int _teamId );

    Vector3 GetPortalCenter     ();

    bool DoesSphereHit      (Vector3 const &_pos, double _radius);
    bool DoesShapeHit       (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit         (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                             double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);
};

#endif