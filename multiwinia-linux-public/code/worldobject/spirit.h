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
        StateBirth,                                     // Just been created, double to a certain height
        StateFloating,                                  // Float around, can be captured
        StateAttached,                                  // Attached to a garbage collector
        StateInStore,                                   // Locked in a Spirit Store
        StateInEgg,                                     // Fertilising an Egg
        StateDeath,                                     // Fade away
        StateShaman,                                     // A shaman is using this soul in a summoning
        StateHarvested,                                  // Currently being sucked up by a harvester
        StateCollected                                  // Has been harvested and is currently sitting in the harvesters collector
    };

    unsigned char   m_teamId;
    int             m_state;
    WorldObjectId   m_worldObjectId;                    // The Id of the entity that died
    WorldObjectId   m_nearbyEggs[SPIRIT_MAXNEARBYEGGS];               
    int             m_numNearbyEggs;
    double          m_eggSearchTimer;                   // How often to re-search
    double           m_broken;           // time since the soul was broken by a summoning
    WorldObjectId   m_harvesterId;    // id of a harvester that has collected us
    
protected:
    double       m_timeSync;
    
    Vector3     m_hover;
    double       m_positionOffset;                       // Used to make them double around a bit
    double       m_xaxisRate;
    double       m_yaxisRate;
    double       m_zaxisRate;
    double       m_groundHeight;

    bool        m_pushFromBuildings;

    Vector3     m_shellPos;         // position of the large outer part of the soul
    
    void        SearchForNearbyEggs();

public:

    Spirit();
    ~Spirit();

    void Begin      ();
    bool Advance    ();
    void Render     ( double predictionTime );
    void RenderWithoutGlBegin( double predictionTime );

    void CollectorArrives   ();                             // A collector is above me and picks me up
    void CollectorDrops     ();                             // My collector has dropped me
    void Harvest            (WorldObjectId _id);            // Caught by Harvester

    void InEgg();                                           // I have been used to fertilise an egg
    void EggDestroyed();                                    // My egg was destroyed, i'm now free
    
    void SkipStage();
    void AddToGlobalWorld();
    void PushFromBuildings();

    void SetShamanMode();     // give this spirit to a shaman for summoning
    void BreakSpirit();

    int             NumNearbyEggs();
    WorldObjectId  *GetNearbyEggs();
};


#endif
