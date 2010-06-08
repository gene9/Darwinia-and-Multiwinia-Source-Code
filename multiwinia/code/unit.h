#ifndef _included_unit_h
#define _included_unit_h

#include "lib/slice_darray.h"
#include "lib/vector3.h"

#include "worldobject/entity.h"

class AI;

class Unit
{
protected:
    Vector3             m_wayPoint;

    double               m_aiTimer;

public:
	int					m_routeId;
	int					m_routeWayPointId;
    int                 m_teamId;
    int                 m_unitId;
    int                 m_troopType;
    SliceDArray         <Entity *> m_entities;

    Vector3             m_centrePos;
    Vector3             m_vel;
    double              m_radius;

    Vector3             m_targetDir;

    double              m_attackAccumulator;                // Used to regulate fire rate

    static double       *s_offsets;

    enum
    {
        FormationRectangle,
        FormationAirstrike,
        NumFormations
    };

	Vector3 GetWayPoint			();
	virtual void SetWayPoint	(Vector3 const &_pos);
    Vector3 GetFormationOffset  (int _formation, int _index);    
    Vector3 GetOffset           (int _formation, int _index);  // Takes into account formation AND obstructions
	
protected:
    Vector3     m_accumulatedCentre;
    double       m_accumulatedRadiusSquared;
    int         m_numAccumulated;

public:
    Unit    (int troopType, int teamId, int unitId, int numEntities, Vector3 const &_pos);
    virtual ~Unit();

    virtual void    Begin           ();
    virtual bool    Advance         ( int _slice );
    virtual void    Attack          ( Vector3 pos, bool withGrenade );
    virtual void    AdvanceEntities ( int _slice );
    virtual void    Render          ( double _predictionTime );

    virtual bool    IsInView        ();

    virtual void    RunAI           ( AI *_ai );

    Entity  *NewEntity              ( int *_index );
    int     AddEntity               ( Entity *_entity );
    void    RemoveEntity            ( int _index, double _posX, double _posZ );
    int     NumEntities             ();
    int     NumAliveEntities        ();                                 // Does not count entities still in the unit, but their m_dead=true
    void    UpdateEntityPosition    ( Vector3 pos, double _radius );
    void    RecalculateOffsets      ();

    virtual bool    IsSelectable            ();
    
	void	FollowRoute();

    virtual void DirectControl( TeamControls const& _teamControls );

	Entity *RayHit(Vector3 const &_rayStart, Vector3 const &_rayDir);
};


#endif
