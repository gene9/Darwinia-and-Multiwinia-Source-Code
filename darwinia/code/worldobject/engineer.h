#ifndef _included_engineer_h
#define _included_engineer_h

#include "lib/llist.h"

#include "worldobject/entity.h"


#define ENGINEER_RETARGETTIMER          1.0f                    // Re-search area for viable targets every x seconds
#define ENGINEER_SEARCHRANGE            200.0f                  // Range around which to search for viable targets


class Engineer : public Entity
{
public:
    enum
    {
        StateIdle,                          // Hover around aimlessly
        StateToWaypoint,                    // Travelling to user waypoint
        StateToSpirit,                      // Travelling to spirit
        StateToIncubator,                   // Travelling to factory with spirit
        StateToControlTower,                // Travelling to control tower
        StateReprogramming,                 // Reprogramming control tower
        StateToBridge,                      // Travelling to bridge        
        StateOperatingBridge,               // Holding a bridge open
        StateToResearchItem,                // Travelling to research item
        StateResearching                    // Reprogramming a research item
    };
       
    int         m_state;
    Vector3     m_wayPoint;                             // User specified waypoint

protected:
    float       m_hoverHeight;
    float       m_idleRotateRate;
        
    Vector3     m_targetPos;                            // Our internal target position
    Vector3     m_targetFront;                          // and orientation
    
    float       m_retargetTimer;                        
    LList       <int> m_spirits;                        // Collector only, Spirits already collected
    int         m_spiritId;                             // Collector only, current target spirit
    
    int         m_positionId;                           // Position on the building we are working on
    int         m_bridgeId;                             // Building ID of a bridge we own
    
    LList       <Vector3 *> m_positionHistory;          
    
protected:
    bool SearchForRandomPosition();
    bool SearchForSpirits();
    bool SearchForControlTowers();
    bool SearchForBridges();
    bool SearchForResearchItems();
    bool SearchForIncubator();

    bool AdvanceIdle            ();
    bool AdvanceToWaypoint      ();
    bool AdvanceToSpirit        ();
    bool AdvanceToIncubator     ();
    bool AdvanceToControlTower  ();
    bool AdvanceReprogramming   ();
    bool AdvanceToBridge        ();
    bool AdvanceOperatingBridge ();
    bool AdvanceToResearchItem  ();
    bool AdvanceResearching     ();
    bool AdvanceToTargetPos     ();                         // returns have-I-Arrived?

public:
    Engineer();

    void Begin                  ();
    bool Advance                ( Unit *_unit );    

    void BeginBridge            ( Vector3 _to );
    void EndBridge              ();

    void SetWaypoint            ( Vector3 const &_wayPoint );
    void ChangeHealth           ( int amount );

    int  GetNumSpirits          ();
    int  GetMaxSpirits          ();
    void CollectSpirit          ( int _spiritId );

    void Render                 ( float predictionTime );
    void RenderShape            ( float predictionTime );
    bool RenderPixelEffect      ( float predictionTime );

    char *GetCurrentAction      ();

    void ListSoundEvents        ( LList<char *> *_list );
};


#endif
