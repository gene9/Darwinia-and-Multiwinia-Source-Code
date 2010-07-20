#ifndef _included_virii_h
#define _included_virii_h

#include "worldobject/entity.h"
#include "unit.h"

#define VIRII_MAXSEARCHRANGE    60.0f
#define VIRII_MINSEARCHRANGE    30.0f
#define VIRII_TAILLENGTH        175.0f


class ViriiHistory;

//*****************************************************************************
// Class ViriiUnit
//*****************************************************************************


class ViriiUnit : public Unit
{
public:
    bool m_enemiesFound;
    bool m_cameraClose;

public:
    ViriiUnit(int teamId, int unitId, int numEntities, Vector3 const &_pos);

    bool Advance( int _slice );
    void Render( float _predictionTime );
};


//*****************************************************************************
// Class Virii
//*****************************************************************************

class Virii : public Entity
{
public:
    enum
    {
        StateIdle,
        StateAttacking,
        StateToSpirit,
        StateToEgg
    };
    int                 m_state;

    float               m_hoverHeight;
    float               m_retargetTimer;
    WorldObjectId	    m_enemyId;
    WorldObjectId		m_eggId;
    int                 m_spiritId;
    Vector3             m_wayPoint;
    float               m_historyTimer;

    Vector3             m_prevPos;
    float               m_prevPosTimer;

protected:

    bool SearchForEnemies();
    bool SearchForSpirits();
    bool SearchForEggs();
    bool SearchForIdleDirection();

    WorldObjectId  FindNearbyEgg        ( int _spiritId, float _autoAccept=99999.9f );
    WorldObjectId  FindNearbyEgg        ( Vector3 const &_pos );

    bool    AdvanceToTargetPos          ( Vector3 const &_pos ); // returns have-I-Arrived?
    void    RecordHistoryPosition       ( bool _required );      // if !_required this is simply to make it smoother
    Vector3 AdvanceDeadPositionVector   ( int _index, Vector3 const &_pos, float _time );

    LList <ViriiHistory *> m_positionHistory;

public:
    Virii();
    ~Virii();

    bool Advance            ( Unit *_unit );
    bool AdvanceIdle        ();
    bool AdvanceAttacking   ();
    bool AdvanceToSpirit    ();
    bool AdvanceToEgg       ();
    bool AdvanceDead        ();

    bool IsInView           ();

    void Render             ( float predictionTime, int teamId, int _detail );

    void ListSoundEvents    ( LList<char *> *_list );
};


//*****************************************************************************
// Class ViriiHistory
//*****************************************************************************

class ViriiHistory
{
public:
    Vector3     m_pos;                                  // Position in world
    Vector3     m_right;                                // Right vector (front is to next point, up is land normal)
    Vector3     m_glowDiff;                             // Diff to previous history point, sized for glow effect
    float       m_distance;                             // Distance to previous history point
    bool        m_required;                             // True means this is an absolute history position (eg direction change)
                                                        // false means its just to smooth out the path (eg height change)
};


#endif
