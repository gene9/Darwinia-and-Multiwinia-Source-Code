#ifndef _included_spirit_h
#define _included_spirit_h

#include "worldobject/entity.h"
#include "worldobject/worldobject.h"

#define SPIRIT_MAXNEARBYEGGS        8


class Spirit : public WorldObject
{
public:
    enum
    {
        StateUnknown,
        StateBirth,                                     // Just been created, float to a certain height
        StateFloating,                                  // Float around, can be captured
        StateAttached,                                  // Attached to a garbage collector
        StateInStore,                                   // Locked in a Spirit Store
        StateInEgg,                                     // Fertilising an Egg
        StateDeath                                      // Fade away
    };

    unsigned char   m_teamId;
    int             m_state;
    WorldObjectId   m_worldObjectId;                    // The Id of the entity that died

    WorldObjectId   m_nearbyEggs[SPIRIT_MAXNEARBYEGGS];               
    int             m_numNearbyEggs;
    float           m_eggSearchTimer;                   // How often to re-search
    
protected:
    float       m_timeSync;
    
    Vector3     m_hover;
    float       m_positionOffset;                       // Used to make them float around a bit
    float       m_xaxisRate;
    float       m_yaxisRate;
    float       m_zaxisRate;
    
    bool        m_pushFromBuildings;
    
public:

    Spirit();
    ~Spirit();

    void Begin      ();
    bool Advance    ();
    void Render     ( float predictionTime );

    void CollectorArrives   ();                             // A collector is above me and picks me up
    void CollectorDrops     ();                             // My collector has dropped me

    void InEgg();                                           // I have been used to fertilise an egg
    void EggDestroyed();                                    // My egg was destroyed, i'm now free
    
    void SkipStage();
    void AddToGlobalWorld();

    void PushFromBuildings();

    int             NumNearbyEggs();
    WorldObjectId  *GetNearbyEggs();
};


#endif
