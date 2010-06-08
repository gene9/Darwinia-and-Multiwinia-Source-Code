#ifndef _included_jumppad_h
#define _included_jumppad_h

#include "lib/tosser/llist.h"

#include "worldobject/building.h"

class JumpPad : public Building
{
public:
    double   m_force;
    double   m_angle;

protected:
    double   m_launchTimer;

public:
    JumpPad         ();
    void Initialise ( Building *_template );

    bool Advance    ();
    void Render     ( double _predictionTime );
    void RenderAlphas( double _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( TextWriter *_out );

    bool DoesSphereHit      (Vector3 const &_pos, double _radius);
    bool DoesShapeHit       (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit         (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                             double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);
};

#endif
