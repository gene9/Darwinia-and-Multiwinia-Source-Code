
#ifndef _included_tentacle_h
#define _included_tentacle_h

#include "worldobject/entity.h"

/*#define CENTIPEDE_MINSEARCHRANGE        75.0
#define CENTIPEDE_MAXSEARCHRANGE        150.0
#define CENTIPEDE_SPIRITEATRANGE        10.0
#define CENTIPEDE_NUMSPIRITSTOREGROW    4
#define CENTIPEDE_NUMSPIRITSTOREGROW_MP 2
#define CENTIPEDE_MAXSIZE               20*/

class Shape;



class Tentacle : public Entity
{
protected:

    enum 
    {
        TentacleStateIdle,
        TentacleStateSlamGround,
        TentacleStateSwipeAcross,
    };

    int             m_state;

    double           m_size;
    WorldObjectId   m_next;                         // Guy infront of me
    WorldObjectId   m_prev;                         // Guy behind me
    
    Vector3         m_targetDirection;
    Vector3         m_direction;
    int             m_portalId;

    LList<Vector3>  m_rotationHistory; 

    double           m_idleTimer;

    Vector3         m_previousVel;
    int             m_positionId;

    bool            m_impactDetected;
    
    static Shape    *s_shapeBody[NUM_TEAMS];
    static Shape    *s_shapeHead[NUM_TEAMS];

protected:
    bool        AdvanceIdle();
    bool        AdvanceSlam();
    bool        AdvanceSwipe();

    void        HandleGroundCollision();

    void        RecordHistoryRotation();          
    bool        GetTrailPosition( Vector3 &_pos, Vector3 &_vel, int _numSteps );

public:
    Tentacle();

    void Begin              ();
    bool Advance            ( Unit *_unit );
    bool ChangeHealth       ( int _amount, int _damageType = DamageTypeUnresistable );
    void Render             ( double _predictionTime );
	bool RenderPixelEffect  ( double _predictionTime );

    bool IsInView           ();
    
    void SetPortalId        ( int _id );

    void Attack             ( Vector3 const &_pos );
};


#endif
