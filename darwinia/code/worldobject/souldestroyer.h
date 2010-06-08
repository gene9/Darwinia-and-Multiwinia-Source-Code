
#ifndef _included_souldestroyer_h
#define _included_souldestroyer_h

#include "worldobject/entity.h"

#define SOULDESTROYER_MINSEARCHRANGE       200.0f
#define SOULDESTROYER_MAXSEARCHRANGE       300.0f
#define SOULDESTROYER_DAMAGERANGE          25.0f
#define SOULDESTROYER_MAXSPIRITS           50

class Shape;



class SoulDestroyer : public Entity
{
protected:
    Vector3         m_targetPos;
    Vector3         m_up;
    WorldObjectId   m_targetEntity;
    LList<Vector3>  m_positionHistory;
    FastDArray      <float> m_spirits;

    float           m_retargetTimer;
    float           m_panic;

    static Shape        *s_shapeHead;
    static Shape        *s_shapeTail;
    static ShapeMarker  *s_tailMarker;

    Vector3      m_spiritPosition[SOULDESTROYER_MAXSPIRITS];

protected:   
    bool        SearchForRandomPosition();
    bool        SearchForTargetEnemy();
    bool        SearchForRetreatPosition();
    
    bool        AdvanceToTargetPosition();        
    void        RecordHistoryPosition();          
    bool        GetTrailPosition( Vector3 &_pos, Vector3 &_vel );

    void RenderShapes               ( float _predictionTime );
    void RenderShapesForPixelEffect ( float _predictionTime );
    void RenderSpirit               ( Vector3 const &_pos, float _alpha );
    bool RenderPixelEffect          ( float _predictionTime );
    
    void Panic( float _time );
    
public:
    SoulDestroyer();

    void Begin              ();
    bool Advance            ( Unit *_unit );
    void ChangeHealth       ( int _amount );
    void Render             ( float _predictionTime );

    void Attack             ( Vector3 const &_pos );

    void ListSoundEvents    ( LList<char *> *_list );

	void SetWaypoint( Vector3 const _waypoint );
};




class Zombie : public WorldObject
{
public:
    Vector3     m_front;
    Vector3     m_up;
    float       m_life;

    Vector3     m_hover;
    float       m_positionOffset;                       // Used to make them float around a bit
    float       m_xaxisRate;
    float       m_yaxisRate;
    float       m_zaxisRate;
    
public:
    Zombie();

    bool Advance();
    void Render( float _predictionTime );
};

#endif
