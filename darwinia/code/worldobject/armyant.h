
#ifndef _included_armyant_h
#define _included_armyant_h

#include "worldobject/entity.h"

#define ARMYANT_SEARCHRANGE         10


class ArmyAnt : public Entity
{
protected:
    float       m_scale;
    Shape       *m_shapes[3];
    ShapeMarker *m_carryMarker;

    bool    AdvanceScoutArea();
    bool    AdvanceCollectSpirit();
    bool    AdvanceCollectEntity();
    bool    AdvanceAttackEnemy();
    bool    AdvanceReturnToBase();
    bool    AdvanceToTargetPosition();
    bool    AdvanceBaseDestroyed();

    bool    SearchForTargets();
    bool    SearchForSpirits();
    bool    SearchForEnemies();
    bool    SearchForAntHill();
    bool    SearchForRandomPosition();

public:
    Vector3     m_wayPoint;
    int         m_orders;
    bool        m_targetFound;
    int         m_spiritId;
    WorldObjectId    m_targetId;

    enum
    {
        NoOrders,
        ScoutArea,
        CollectSpirit,
        CollectEntity,
        AttackEnemy,
        ReturnToBase,
        BaseDestroyed
    };

public:
    ArmyAnt();

    void Begin          ();
    bool Advance        ( Unit *_unit );
    void ChangeHealth   ( int _amount );
    void Render         ( float _predictionTime );

    void OrderReturnToBase();

    void GetCarryMarker( Vector3 &_pos, Vector3 &_vel );

};


#endif
