#ifndef _included_spaceship_h
#define _included_spaceship_h

#include "lib/tosser/llist.h"

#include "worldobject/entity.h"


class SpaceShip : public Entity
{
protected:
    void AdvanceAbducting( bool _atTarget );
    void AdvanceDropping();
    void AdvanceEngines();

public:
    WorldObjectId   m_target;
    WorldObjectId   m_lastDrop;     // track the last darwinian we dropped so that once they are all landed, we can shut off the beam and leave

    int             m_abductees;
    int             m_state;

    Vector3         m_waypoint;

    double           m_speed;
    float           m_abductionTimer;

    enum
    {
        ShipStateIdle,      // just arrived, we should only be in this state while we decend towards our target
        ShipStateAbducting, // using our beam to abduct darwinians
        ShipStateWaiting,
        ShipStateDropping,  // we've collected our quota of Darwinians, so drop off some Futurewinians 
        ShipStateLeaving,   // we've dropped off our futurewinians, so lets go away
        NumStates
    };

public:
    SpaceShip();

    void Begin                  ();

    bool Advance                ( Unit *_unit );  
    bool AdvanceToTargetPos     ();

    void Render                 ( double predictionTime );
    void RenderBeam             ( double _predictionTime );

    void SetWaypoint            ( Vector3 const &_wayPoint );
    bool ChangeHealth           ( int amount, int _damageType = DamageTypeUnresistable );

    void FindNewTarget          ();

    void ListSoundEvents        ( LList<char *> *_list );

    void IncreaseAbductees      ();
};


#endif
