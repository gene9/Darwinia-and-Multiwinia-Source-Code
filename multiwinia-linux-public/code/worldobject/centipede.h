
#ifndef _included_centipede_h
#define _included_centipede_h

#include "worldobject/entity.h"

#define CENTIPEDE_MINSEARCHRANGE        75.0
#define CENTIPEDE_MAXSEARCHRANGE        150.0
#define CENTIPEDE_SPIRITEATRANGE        10.0
#define CENTIPEDE_NUMSPIRITSTOREGROW    4
#define CENTIPEDE_NUMSPIRITSTOREGROW_MP 4
#define CENTIPEDE_MAXSIZE               20

class Shape;



class Centipede : public Entity
{
protected:
    double           m_size;
    WorldObjectId   m_next;                         // Guy infront of me
    WorldObjectId   m_prev;                         // Guy behind me
    
    Vector3         m_targetPos;
    WorldObjectId   m_targetEntity;

    LList<Vector3>  m_positionHistory;
    bool            m_linked;
    double           m_panic;
    int             m_numSpiritsEaten;
    double           m_lastAdvance;
    
    static Shape    *s_shapeBody[NUM_TEAMS];
    static Shape    *s_shapeHead[NUM_TEAMS];

protected:    
    bool        SearchForRandomPosition();
    bool        SearchForTargetEnemy();
    bool        SearchForSpirits();
    bool        SearchForRetreatPosition();

    bool        AdvanceToTargetPosition();        
    void        RecordHistoryPosition();          
    bool        GetTrailPosition( Vector3 &_pos, Vector3 &_vel, int _numSteps );
           
    void        Panic( double _time );      
    void        EatSpirits();

public:
    Centipede();

    void Begin              ();
    bool Advance            ( Unit *_unit );
    bool ChangeHealth       ( int _amount, int _damageType = DamageTypeUnresistable );
    void Render             ( double _predictionTime );
	bool RenderPixelEffect  ( double _predictionTime );

    bool IsInView           ();

    void Attack             ( Vector3 const &_pos );

    int GetSize            (); 

    void ListSoundEvents    ( LList<char *> *_list );
};


#endif
