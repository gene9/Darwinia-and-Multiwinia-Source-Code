
#ifndef _included_randomiser_h
#define _included_randomiser_h

#include "worldobject/worldobject.h"


class Randomiser : public WorldObject
{
protected:
    double   m_timer;
    double   m_life;
    
public:
    Randomiser();

    bool Advance();
    void Render( double _predictionTime );
};


#endif