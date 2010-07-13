
#ifndef _included_dustball_h
#define _included_dustball_h

#include "worldobject/worldobject.h"


class DustBall : public WorldObject
{    
public:
    double m_size;

    static Vector3  s_vortexPos;
    static double    s_vortexTimer;

    DustBall();

    bool Advance();
    void Render( double _predictionTime );
};


#endif