
#ifndef _included_snow_h
#define _included_snow_h

#include "worldobject/worldobject.h"


class Snow : public WorldObject
{
protected:
    double       m_timeSync;
    double       m_positionOffset;                       // Used to make them double around a bit
    double       m_xaxisRate;
    double       m_yaxisRate;
    double       m_zaxisRate;

public:
    Vector3     m_hover;
    
public:
    Snow();

    bool Advance();
    void Render( double _predictionTime );

    double GetLife();                        // Returns 0.0-1.0 (0.0=dead, 1.0=alive)
};


#endif