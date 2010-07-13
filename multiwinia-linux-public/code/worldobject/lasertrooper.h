
#ifndef _included_lasertrooper_h
#define _included_lasertrooper_h

#include "worldobject/entity.h"


class LaserTrooper : public Entity
{
public:
    Vector3 m_targetPos;
    Vector3 m_unitTargetPos;

    double   m_victoryDance;
	
public:
    
    void Begin      ();

    bool Advance    ( Unit *_unit );   
    void Render     ( double _predictionTime, int _teamId );

    void AdvanceVictoryDance();
};


#endif
