#ifndef _included_restrictionzone_h
#define _included_restrictionzone_h

#include "worldobject/building.h"

class RestrictionZone : public Building
{
public:
    double   m_size;
    int    m_blockPowerups;

    static  LList<int>  s_restrictionZones;

public:

    RestrictionZone();
    ~RestrictionZone();

    void Initialise( Building *_template );

    bool Advance();
    void RenderAlphas( double _predictionTime );

    static bool IsRestricted( Vector3 _pos, bool _powerups = false );

    void Read   ( TextReader *_in, bool _dynamic ); 
    void Write  ( TextWriter *_out );

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);
};

#endif
