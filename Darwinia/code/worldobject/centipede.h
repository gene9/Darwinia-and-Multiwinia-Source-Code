
#ifndef _included_centipede_h
#define _included_centipede_h

#include "worldobject/entity.h"

#define CENTIPEDE_MINSEARCHRANGE        75.0f
#define CENTIPEDE_MAXSEARCHRANGE        150.0f
#define CENTIPEDE_SPIRITEATRANGE        10.0f
#define CENTIPEDE_NUMSPIRITSTOREGROW    4
#define CENTIPEDE_MAXSIZE               20

class Shape;



class Centipede : public Entity
{
protected:
    float           m_size;
    WorldObjectId   m_next;                         // Guy infront of me
    WorldObjectId   m_prev;                         // Guy behind me

    Vector3         m_targetPos;
    WorldObjectId   m_targetEntity;

    LList<Vector3>  m_positionHistory;
    bool            m_linked;
    float           m_panic;
    int             m_numSpiritsEaten;
    float           m_lastAdvance;

    static Shape    *s_shapeBody;
    static Shape    *s_shapeHead;

protected:
    bool        SearchForRandomPosition();
    bool        SearchForTargetEnemy();
    bool        SearchForSpirits();
    bool        SearchForRetreatPosition();

    bool        AdvanceToTargetPosition();
    void        RecordHistoryPosition();
    bool        GetTrailPosition( Vector3 &_pos, Vector3 &_vel, int _numSteps );

    void        Panic( float _time );
    void        EatSpirits();

public:
    Centipede();

    void Begin              ();
    bool Advance            ( Unit *_unit );
    void ChangeHealth       ( int _amount );
    void Render             ( float _predictionTime );
	bool RenderPixelEffect  ( float _predictionTime );

    bool IsInView           ();

    void Attack             ( Vector3 const &_pos );

    void ListSoundEvents    ( LList<char *> *_list );
};


#endif
