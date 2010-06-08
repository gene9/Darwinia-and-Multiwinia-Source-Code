#ifndef _included_lander_h
#define _included_lander_h

#include "worldobject/entity.h"



class Lander : public Entity
{
protected:
    enum
    {
        StateSailing,
        StateLanded
    };

    int     m_state;
    float   m_spawnTimer;

public:
    Lander();

    bool Advance        ( Unit *_unit );
    bool AdvanceSailing ();
    bool AdvanceLanded  ();

    void ChangeHealth   ( int amount );
    void Render         ( float _predictionTime, int _teamId );

};


#endif
