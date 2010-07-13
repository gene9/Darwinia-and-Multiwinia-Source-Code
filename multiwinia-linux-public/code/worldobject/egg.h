
#ifndef _included_egg_h
#define _included_egg_h

#include "worldobject/entity.h"

#define EGG_DORMANTLIFE     120                             // total time alive while unfertilised


class Egg : public Entity
{
public:
    
    enum
    {
        StateDormant,
        StateFertilised,
        StateOpen
    };

    int     m_state;
    int     m_spiritId;
    double   m_timer;
    
public:
    Egg();

    bool ChangeHealth   ( int _amount, int _damageType = DamageTypeUnresistable );

    void Render         ( double predictionTime );
    bool Advance        ( Unit *_unit );
    void Fertilise      ( int spiritId );
};



#endif
