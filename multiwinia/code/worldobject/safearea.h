
#ifndef _included_safearea_h
#define _included_safearea_h

#include "worldobject/building.h"


class SafeArea : public Building
{
public:
    double       m_size;
    int         m_entitiesRequired;
    int         m_entityTypeRequired;
    
    double       m_recountTimer;
    int         m_entitiesCounted;
    
public:
    SafeArea();

    void Initialise ( Building *_template );
    bool Advance    ();
    void Render     ( double predictionTime );        

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);

    void GetObjectiveCounter( UnicodeString& _dest );

    void Read       ( TextReader *_in, bool _dynamic );     
    void Write      ( TextWriter *_out );							
};


#endif
