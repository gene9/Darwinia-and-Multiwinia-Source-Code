#ifndef _included_smokemarker_h
#define _included_smokemarker_h

#include "worldobject/building.h"

class SmokeMarker : public Building
{
public:

    SmokeMarker();

    bool Advance();
    void RenderAlphas( float _predictionTime );

    bool DoesSphereHit          (Vector3 const &_pos, float _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 float _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);

};

#endif