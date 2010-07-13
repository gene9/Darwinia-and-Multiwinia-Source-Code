#ifndef _included_meteor_h
#define _included_meteor_h

#include "worldobject/entity.h"

class Meteor : public Entity
{
public:
    Vector3 m_targetPos;
    double   m_life;
    bool    m_timeSlowTriggered;

public:
    Meteor();
    ~Meteor();

    void Begin          ();

    bool Advance        ( Unit *_unit );
    bool AdvanceToTarget();

    void Render         ( double _predictionTime );

    void    ListSoundEvents	( LList<char *> *_list );
};


#endif
