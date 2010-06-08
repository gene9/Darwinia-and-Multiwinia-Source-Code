#ifndef _included_ai_h
#define _included_ai_h

#include "location.h"

#include "worldobject/entity.h"

struct RRSolarPanelInfo
{
    Vector3 m_pos;
    int     m_numPanels;
    int     m_objectiveId;
    float   m_range;
};

class AI : public Entity
{
protected:
	void AdvanceRadarDishAI();  // used to retarget Radar Dishes
    void AdvanceOfficerAI();    // used to check if we need to create a new Officer 
    void AdvanceSpecialsAI();   // runs the AI updates on all special units (officers, armour etc)
    void AdvanceUnitAI();       // advance the ai for our units, probably just the odd squad
    void AdvanceCurrentTasks(); // check to see if we have any tasks (probably from crates) and run them

    void AdvanceSinglePlayerAI();   // used to move darwinians around in the standard single-player way
    void AdvanceMultiwiniaAI();     // uses officers to move darwinians from spawn points

    void AdvanceCaptureTheStatueAI();	// Capture the statue needs special AI to move statues around
    void AdvanceAssaultAI();            // special case AI for assault mode uses AIObjective system instead of AITarget priorities
    void AdvanceRocketRiotAI();         // Fills the objective list for this team for use by armour ai

	Vector3 GetClosestScoreZonePos( Vector3 _pos );

    void RunSquad       ( int _taskId );
    void RunArmour      ( int _taskId );
    void RunHarvester   ( int _taskId );
    void RunEngineer    ( int _taskId );
    void RunAirStrike   ( int _taskId );
    void RunNuke        ( int _taskId );
    void RunTank        ( int _taskId );

	void RunGunTurret	( int _taskId );
	void RunGunTurretBlitzkrieg( int _taskId );
    void RunGunTurretRocketRiot( int _taskId ); // rocket riot needs its own gun turret function because of the odd ai target layout
	void RunSpam		( int _taskId );
	void RunForest		( int _taskId );
	void RunAntNest		( int _taskId );
	void RunEggs		( int _taskId );
	void RunPlague      ( int _taskId );
	void RunMeteor		( int _taskId );
	void RunDarkForest	( int _taskId );

    void RunPowerup     ();
    void RunPowerup     ( Vector3 _pos );
    bool HasDarwinianPowerup(); // returns true if we have a Subversion, Personal Shield or Hot Feet task available

    double m_timer;
	double m_radarReposTimer;

    bool m_tutorialCentreZoneCaptured;

	LList<Vector3>	m_ctsScoreZones;

public:
    int             m_currentObjective;
    LList<RRSolarPanelInfo *>   m_rrObjectiveList;

public:
    AI();
    ~AI();
    
    void Begin();
    bool Advance( Unit *_unit );
    bool ChangeHealth( int _amount, int _damageType = DamageTypeUnresistable );

    static int FindNearestTarget( Vector3 const &_fromPos, bool _evaluateCliffs = true, bool _allowSteepDownhill = false );
    int FindTargetBuilding( int _fromTargetId, int _fromTeamId );

    void Render( double _predictionTime );

    int  SearchForOfficers( Vector3 _pos, double _range, bool _ignoreGotoOfficers = false, bool _gotoOnly = false );

    static AI *s_ai[NUM_TEAMS];
};


// ============================================================================


#define AITARGET_LINKRANGE      600.0

struct NeighbourInfo
{
    int m_neighbourId;
    int m_laserFenceId;
};

class AITarget : public Building
{
protected:
    double           m_teamCountTimer;
   
public:
    LList<NeighbourInfo *>      m_neighbours;                               // Building IDs of nearby AITargets

    int             m_friendCount   [NUM_TEAMS];
    int             m_enemyCount    [NUM_TEAMS];
    int             m_idleCount     [NUM_TEAMS];
    double          m_priority      [NUM_TEAMS];

    int             m_linkId;       // lets you manually connect 2 targets which would otherwise be out of range
    double          m_priorityModifier;
    int             m_checkCliffs;
    double          m_scanRange;
    int             m_areaControl;  // when very close to 1 or more other targets, this value determines whether this
                                    // target should update or simply pass and let another target update
                                    // -1 = pass, 1 = always update, 0 = act normally

	bool			m_radarTarget;	// this target is the entrance to a radar dish
	int			    m_statueTarget;	// this target is attached to a CTS Statue#
    bool            m_turretTarget; // this target is attached to a Gun Turret
    bool            m_crateTarget;
    int             m_scoreZone;    // if this target is inside a score zone, this will contain the id of the zone
    bool            m_aiObjectiveTarget[NUM_TEAMS];    // this target is controlled by an ai objective so leave its priorities alone
    bool            m_waterTarget;    // this target is in the water, and should only be used for Armour path finding

private:
    void RecalculatePriorityRocketRiot();
    void RecalculatePriorityAssault();
    void RecalculatePriorityCTS();
    void RecalculatePriorityStandard();
    
public:
    AITarget();
    ~AITarget();

    void    Initialise( Building *_template );

    bool    Advance ();
    void    Render          ( double _predictionTime );
    void    RenderAlphas    ( double _predictionTime );

    void    RecalculateNeighbours();
    void    RecountTeams();
    void    RecalculateOwnership();
    void    RecalculatePriority();

    double   IsNearTo        ( int _aiTargetId );                            // returns distance or -1
    bool    IsImportant();
    bool    IsImportant( int &_buildingId );  // returns true if this point is close to another building, eg spawn point or multiwinia zone
    bool    NeedsDefending( int _teamid );

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);

    void SetBuildingLink    ( int _buildingId ); 
    int  GetBuildingLink    ();

    double GetRange();

    void Read   ( TextReader *_in, bool _dynamic ); 
    void Write  ( TextWriter *_out );	
};


// ============================================================================


class AISpawnPoint : public Building
{
protected:
    double   m_timer;                // Master timer between spawns
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
    void    RenderAlphas    ( double _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic ); 
    void Write  ( TextWriter *_out );						

    int  GetBuildingLink();                             
    void SetBuildingLink( int _buildingId );            

    bool DoesSphereHit      (Vector3 const &_pos, double _radius);
    bool DoesShapeHit       (Shape *_shape, Matrix34 _transform);
};



#endif
