
#ifndef _included_souldestroyer_h
#define _included_souldestroyer_h

#include "worldobject/entity.h"

#define SOULDESTROYER_MINSEARCHRANGE       200.0
#define SOULDESTROYER_MAXSEARCHRANGE       300.0
#define SOULDESTROYER_DAMAGERANGE          25.0
#define SOULDESTROYER_MAXSPIRITS           50

class Shape;



class SoulDestroyer : public Entity
{
protected:
    Vector3         m_targetPos;
    Vector3         m_up;
    WorldObjectId   m_targetEntity;
    LList<Vector3>  m_positionHistory;
    FastDArray      <double> m_spirits;

    double           m_retargetTimer;
    double           m_panic;

    static Shape        *s_shapeHead[NUM_TEAMS];
    static Shape        *s_shapeTail[NUM_TEAMS];
    static ShapeMarker  *s_tailMarker[NUM_TEAMS];

    Vector3      m_spiritPosition[SOULDESTROYER_MAXSPIRITS];

public:
    static int  s_numSoulDestroyers;

protected:   
    bool        SearchForRandomPosition();
    bool        SearchForTargetEnemy();
    bool        SearchForRetreatPosition();
    
    bool        AdvanceToTargetPosition();        
    void        RecordHistoryPosition();          
    bool        GetTrailPosition( Vector3 &_pos, Vector3 &_vel );

    void RenderShapes               ( double _predictionTime );
    void RenderShapesForPixelEffect ( double _predictionTime );
    void RenderSpirit               ( Vector3 const &_pos, double _alpha );
    bool RenderPixelEffect          ( double _predictionTime );
    
    void Panic( double _time );
    
public:
    SoulDestroyer();
    ~SoulDestroyer();

    void Begin              ();
    bool Advance            ( Unit *_unit );
    bool ChangeHealth( int _amount, int _damageType = DamageTypeUnresistable );
    void Render             ( double _predictionTime );

    void Attack             ( Vector3 const &_pos );

    void ListSoundEvents    ( LList<char *> *_list );

	void SetWaypoint( Vector3 const _waypoint );
};




class Zombie : public WorldObject
{
public:
    Vector3     m_front;
    Vector3     m_up;
    double       m_life;

    Vector3     m_hover;
    double       m_positionOffset;                       // Used to make them double around a bit
    double       m_xaxisRate;
    double       m_yaxisRate;
    double       m_zaxisRate;
    
public:
    Zombie();

    bool Advance();
    void Render( double _predictionTime );
};

#endif
