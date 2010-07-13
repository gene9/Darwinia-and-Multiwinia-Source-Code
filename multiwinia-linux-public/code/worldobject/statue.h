#ifndef _included_statue_h
#define _included_statue_h

/*
 *      All objects for Capture the Statue
 *
 */

#include "carryablebuilding.h"

class AITarget;

class Statue : public CarryableBuilding
{
public:
	AITarget *m_aiTarget;
protected:
    double m_waypointCheck;
    bool    m_renderSpecial;

public:
    Statue();

    void Initialise     ( Building *_template );

    bool Advance();
    void Render( double _predictionTime );

    void Destroy        ();
    void ExplodeShape   ( double _fraction );
};


#endif
