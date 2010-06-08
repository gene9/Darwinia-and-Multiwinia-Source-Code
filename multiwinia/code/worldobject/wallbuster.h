#ifndef _included_wallbuster_h
#define _included_wallbuster_h

/*
 *      Carryable Explosive designed to destroy walls
 *
 */

#include "carryablebuilding.h"

class AITarget;
class AI;

class WallBuster : public CarryableBuilding
{
public:
	AITarget *m_aiTarget;

protected:
    double   m_destructTimer;
    double  m_waypointCheck;

public:
    WallBuster();

    void Initialise     ( Building *_template );

    bool Advance();
    void Render( double _predictionTime );

    void Destroy        ();
    void ExplodeShape   ( double _fraction );
    void BreakWalls     ();

    void RunAI( AI *_ai);
};


#endif
