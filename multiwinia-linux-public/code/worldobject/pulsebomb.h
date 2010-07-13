
#ifndef _included_pulsebomb_h
#define _included_pulsebomb_h

#include "worldobject/building.h"


class PulseBomb : public Building
{
public:
    bool    m_active;
    double   m_scale;
    double  m_detonateTime;
    double   m_startTime;

    LList<int>  m_links;

    bool    m_oneMinuteWarning; 

    static int s_pulseBombs[NUM_TEAMS];
    static double    s_defaultTime;

public:
    PulseBomb();
    ~PulseBomb();

    bool Advance    ();

    void Render     ( double _predictionTime );
    void RenderAlphas( double _predictionTime );
    void RenderCountdown( double _predictionTime );

    void Initialise ( Building *_template );
    void SetDetail  ( int _detail );

    void Destroy    ();
    void ExplodeShape( double _fraction );

    void SetBuildingLink( int _buildingId );

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( TextWriter *_out );	

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);  

};

#endif
