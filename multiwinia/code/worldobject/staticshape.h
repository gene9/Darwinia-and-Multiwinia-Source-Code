
#ifndef _included_staticshape_h
#define _included_staticshape_h

#include "worldobject/building.h"


class StaticShape : public Building
{
public:
    char    m_shapeName[256];
    double   m_scale;

public:
    StaticShape();

    void Initialise     ( Building *_template );
    void SetDetail      ( int _detail );

    void SetShapeName   ( char *_shapeName );
    void SetStringId    ( char *_stringId );

    bool Advance();
    void Render( double _predictionTime );

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);        // pos/norm will not always be available

    void Read( TextReader *_in, bool _dynamic );
    void Write( TextWriter *_out );
};


#endif