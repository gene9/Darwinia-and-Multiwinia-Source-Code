#ifndef _included_eruptionmarker_h
#define _included_eruptionmarker_h

#include "worldobject/building.h"

class EruptionMarker : public Building
{
public:
    bool    m_erupting;
    double   m_timer;

    EruptionMarker();

    bool Advance();
    void RenderAlphas( double _predictionTime );

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);

};

#endif