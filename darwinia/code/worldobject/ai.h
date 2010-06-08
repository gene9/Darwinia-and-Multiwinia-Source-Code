#ifndef _included_ai_h
#define _included_ai_h

#include "location.h"

#include "worldobject/entity.h"


class AI : public Entity
{
protected:
    int FindTargetBuilding( int _fromTargetId, int _fromTeamId );
    int FindNearestTarget( Vector3 const &_fromPos );

    float m_timer;

public:
    AI();
    
    void Begin();
    bool Advance( Unit *_unit );
    void ChangeHealth( int _amount );

    void Render( float _predictionTime );
};


// ============================================================================


#define AITARGET_LINKRANGE      600.0f


class AITarget : public Building
{
protected:
    float           m_teamCountTimer;
   
public:
    LList<int>      m_neighbours;                               // Building IDs of nearby AITargets

    int             m_friendCount   [NUM_TEAMS];
    int             m_enemyCount    [NUM_TEAMS];
    int             m_idleCount     [NUM_TEAMS];
    float           m_priority      [NUM_TEAMS];
    
public:
    AITarget();

    bool    Advance ();
    void    Render          ( float _predictionTime );
    void    RenderAlphas    ( float _predictionTime );

    void    RecalculateNeighbours();
    void    RecountTeams();
    void    RecalculateOwnership();
    void    RecalculatePriority();

    float   IsNearTo        ( int _aiTargetId );                            // returns distance or -1

    bool DoesSphereHit          (Vector3 const &_pos, float _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
};


// ============================================================================


class AISpawnPoint : public Building
{
protected:
    float   m_timer;                // Master timer between spawns
    bool    m_online;
    int     m_numSpawned;           // Number spawned this batch
    int     m_populationLock;       // Building ID (if found), -1 = not yet searched, -2 = nothing found
    
    bool    PopulationLocked();

public:
    int     m_entityType;
    int     m_count;
    int     m_period;
    int     m_activatorId;          // Building ID
    int     m_spawnLimit;           // limits the number of times this building can spawn
	int		m_routeId;				// Route path that spawned units should follow

public:
    AISpawnPoint();

    void    Initialise      ( Building *_template );
    bool    Advance         ();
    void    RenderAlphas    ( float _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic ); 
    void Write  ( FileWriter *_out );						

    int  GetBuildingLink();                             
    void SetBuildingLink( int _buildingId );            

    bool DoesSphereHit      (Vector3 const &_pos, float _radius);
    bool DoesShapeHit       (Shape *_shape, Matrix34 _transform);
};



#endif