#ifndef _included_spawnpoint_h
#define _included_spawnpoint_h

#include "worldobject/building.h"


// ============================================================================

class SpawnBuildingSpirit
{
public:
    int     m_targetBuildingId;
    float   m_currentProgress;
};


class SpawnBuildingLink
{
public:
    int m_targetBuildingId;                         // Who I am directly linked to

    LList<int> m_targets;                           // List of all SpawnPoint buildings reachable down this route
    LList<SpawnBuildingSpirit *> m_spirits;
};


class SpawnBuilding : public Building
{
protected:
    LList       <SpawnBuildingLink *> m_links;
    ShapeMarker *m_spiritLink;

    Vector3     m_visibilityMidpoint;
    float       m_visibilityRadius;

public:
    SpawnBuilding();
	~SpawnBuilding();

    void            Initialise  ( Building *_template );
    bool            Advance     ();

    virtual void    TriggerSpirit   ( SpawnBuildingSpirit *_spirit );

    bool            IsInView        ();
    void            Render          ( float _predictionTime );
    void            RenderAlphas    ( float _predictionTime );
    void            RenderSpirit    ( Vector3 const &_pos );

    Vector3         GetSpiritLink   ();
    void            SetBuildingLink ( int _buildingId );
    void            ClearLinks      ();

    LList<int>     *ExploreLinks    ();                     // Returns a list of all SpawnBuildings accessable

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );
};

// ============================================================================

class SpawnLink : public SpawnBuilding
{
public:
    SpawnLink();
};


// ============================================================================

class MasterSpawnPoint : public SpawnBuilding
{
protected:
    static int s_masterSpawnPointId;
    bool m_exploreLinks;

public:
    MasterSpawnPoint();

    bool Advance();
    void Render       ( float _predictionTime );
    void RenderAlphas ( float _predictionTime );

    void RequestSpirit( int _targetBuildingId );

    char *GetObjectiveCounter();

    static MasterSpawnPoint *GetMasterSpawnPoint();
};


// ============================================================================


class SpawnPoint : public SpawnBuilding
{
protected:
    void            RecalculateOwnership();
    bool            PopulationLocked();

protected:
    float           m_evaluateTimer;
    float           m_spawnTimer;
    int             m_populationLock;               // Building ID (if found), -1 = not yet searched, -2 = nothing found
    int             m_numFriendsNearby;
    ShapeMarker     *m_doorMarker;

public:
    SpawnPoint();

    bool            Advance();

    bool PerformDepthSort( Vector3 &_centrePos );

    void            TriggerSpirit( SpawnBuildingSpirit *_spirit );

    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );
    void RenderPorts    ();

};


// ============================================================================

class SpawnPopulationLock : public Building
{
public:
    float   m_searchRadius;
    int     m_maxPopulation;
    int     m_teamCount[NUM_TEAMS];

protected:
    static float    s_overpopulationTimer;
    static int      s_overpopulation;
    int             m_originalMaxPopulation;
    float           m_recountTimer;
    int             m_recountTeamId;

public:
    SpawnPopulationLock();

    void    Initialise      ( Building *_template );
    bool    Advance         ();
    void    Render          ( float _predictionTime );
    void    RenderAlphas    ( float _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );

    bool DoesSphereHit      (Vector3 const &_pos, float _radius);
    bool DoesShapeHit       (Shape *_shape, Matrix34 _transform);
};

#endif