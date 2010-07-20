
#ifndef _included_snow_h
#define _included_snow_h

#include "worldobject/worldobject.h"


class Snow : public WorldObject
{
protected:
    float       m_timeSync;
    float       m_positionOffset;                       // Used to make them float around a bit
    float       m_xaxisRate;
    float       m_yaxisRate;
    float       m_zaxisRate;

public:
    Vector3     m_hover;

public:
    Snow();

    bool Advance();
    void Render( float _predictionTime );

    float GetLife();                        // Returns 0.0f-1.0f (0.0f=dead, 1.0f=alive)
};


#endif