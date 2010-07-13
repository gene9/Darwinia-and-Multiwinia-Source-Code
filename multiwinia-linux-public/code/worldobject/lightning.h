
#ifndef _included_lightning_h
#define _included_lightning_h

#include "worldobject/worldobject.h"


class Lightning : public WorldObject
{    
public:

    bool m_striking;
    double m_life;

    Lightning();

    bool Advance();
    void Render( double _predictionTime );
};


#endif