
#ifndef _included_wall_h
#define _included_wall_h

#include "worldobject/building.h"


class Wall : public Building
{
protected:
    double m_damage;
    double m_fallSpeed;
    double m_lastDamageTime;
    double m_scale;

    int   m_objectiveLink;
    bool    m_registered;
    
public:
    Wall();
    void Initialise( Building *_template );

    bool Advance    ();
    void Damage     ( double _damage );
    void Render     ( double _predictionTime );

    void SetDetail  ( int _detail );

    void SetBuildingLink( int _buildingId );
    int  GetBuildingLink();

    bool DoesSphereHit( const Vector3 &_pos, double _radius );
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( TextWriter *_out );	

};


#endif
