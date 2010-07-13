#ifndef INCLUDED_LOCATION_H
#define INCLUDED_LOCATION_H

#include <float.h>

#include "lib/tosser/fast_darray.h"
#include "lib/slice_darray.h"
#include "lib/vector3.h"

#include "network/server.h"

#include "globals.h"
#include "landscape.h"
#include "taskmanager.h"
#include "routing_system.h"

#include "worldobject/building.h"
#include "worldobject/worldobject.h"
#include "worldobject/weapons.h"
#include "worldobject/spirit.h"

#include "lib/unicode/unicode_string.h"

#include <vector>


class ServerToClientLetter;
class WorldObject;
class WorldObjectEffect;
class Entity;
class EntityGrid;
class EntityGridCache;
class ObstructionGrid;
class Unit;
class LaserGod;
class LevelFile;
class Clouds;
class Water;
class Light;
class Team;
class TeamControls;
class UnprocessedSpirit;
class CommandBuffer;


// ****************************************************************************
//  Class Location
// ****************************************************************************

struct ChatMessage
{
    UnicodeString   m_msg;
    int             m_clientId;
    float           m_recievedTime;
    RGBAColour      m_colour;
    UnicodeString   m_senderName;
};

struct BurnPatch
{
    Vector3     m_pos;
    double      m_radius;
};

class Location
{
	friend class DeleteCommandBuffers;

protected:
    int	 m_lastSliceProcessed;
	bool m_missionComplete;
    
    void SetMyTeamId			( unsigned char _teamId );
    void LoadLevel				( char const *_missionFilename, char const *_mapFilename );

    void AdvanceWeapons			( int _slice );
    void AdvanceBuildings		( int _slice );
    void AdvanceTeams			( int _slice );
    void AdvanceSpirits			( int _slice );
    void AdvanceClouds			( int _slice );
    void AdvanceCrates          ();
	
    void RenderLandscape		();
    void RenderWeapons			();
    void RenderBuildings		();
    void RenderBuildingAlphas	();
    void RenderParticles		();
    void RenderTeams			();
    void RenderMagic			();
    void RenderSpirits			();
    void RenderClouds			();
    void RenderWater			();
    void RenderCurrentMessage   ();
    
    void InitLandscape			();
    void InitLights				();
    void InitTeams				();

	void DoMissionCompleteActions();

    Vector3     FindValidSpawnPosition( Vector3 const &_pos, double _spread );

private:
    bool            m_coopGroupColourTaken[2][2];
    bool            m_showCrateDropMessage;

public:
    Landscape       m_landscape;
    EntityGrid      *m_entityGrid;  
    EntityGridCache *m_entityGridCache;
    ObstructionGrid *m_obstructionGrid;
	LevelFile		*m_levelFile;
    Clouds          *m_clouds;
    Water           *m_water;

    LandRoutingSystem   m_routingSystem;
	SeaRoutingSystem	m_seaRoutingSystem;

    Team            *m_teams[NUM_TEAMS];
    int             m_teamPosition[NUM_TEAMS];

    double           m_christmasTimer;
    double           m_spawnSync;    // timer for all other non-christmas effects
    double           m_crateTimer;
    double           m_spawnManiaTimer;
    double           m_bitzkreigTimer;

    bool            m_allowMonsterTeam;
    bool            m_teamsInitialised;

    double           m_messageTimer;     // this message crap might need moving once the new task interface is done
    UnicodeString   m_message;
    int             m_messageTeam;
    bool            m_messageNoOverwrite;
        
	FastDArray		<Light *>		m_lights;
    SliceDArray     <Building *>    m_buildings;
    SliceDArray     <Spirit>        m_spirits;    
    SliceDArray     <Laser>         m_lasers;
    SliceDArray     <WorldObject *> m_effects;

    LList           <ChatMessage *> m_chatMessages;

    Vector3         m_windDirection;

    int             m_coopGroups[NUM_TEAMS];    // set to 1 if the team is group 1, 2 for group 2
    int             m_groupColourId[2];

    LList<BurnPatch>    m_burnPatches;

    bool    m_christmasMode;
    
public:
    struct DepthSortedBuilding
    {
        int   m_buildingIndex;
        float m_distance;
    };

private:
    DepthSortedBuilding *m_sortedBuildingsAlphas;
    int                 m_sizeSortedBuildingsAlphas;

    static std::vector<WorldObjectId> s_neighbours;
    static bool                       s_neighboursInUse;

public:
    Location();
    ~Location();

    void Init               ( char const *_missionFilename, char const *_mapFilename );
    void InitBuildings			();
	void Empty				();
    
    void Advance            ( int _slice );        
    void Render             ( bool renderWaterAndClouds = true );
    void RenderChatMessages ();

    void InitialiseTeam     ( unsigned char _teamId );
    int  GetTeamPosition    ( unsigned char _teamId );
    int  GetTeamFromPosition( int _positionId );
    void RotatePositions    ();

    int GetBuildingId           ( Vector3 const &startRay, Vector3 const &direction, unsigned char teamId, double _maxDistance=FLT_MAX, double *_range=NULL );
    int GetUnitId               ( Vector3 const &startRay, Vector3 const &direction, unsigned char teamId, double *_range=NULL );
    WorldObjectId GetEntityId   ( Vector3 const &startRay, Vector3 const &direction, unsigned char teamId, double *_range=NULL, bool _ignoreDarwinians = false );

    bool  IsWalkable         ( Vector3 const &from, Vector3 const &to, bool evaluateCliffs=false, bool permitSteepDownhill=false, int *_laserFenceId = NULL );    
    double GetWalkingDistance ( Vector3 const &from, Vector3 const &to, bool checkObstructions = true );        // < 0 = not walkable
    bool  IsVisible          ( Vector3 const &from, Vector3 const &to );
    
    void UpdateTeam         ( unsigned char teamId, TeamControls const& teamControls );
    
    int  SpawnSpirit        ( Vector3 const &_pos, Vector3 const &_vel, unsigned char _teamId, WorldObjectId _id );
    int ThrowWeapon        ( Vector3 const &_pos, Vector3 const &_target, int _type, unsigned char _fromTeamId, double _force = 0.0f, double _lifeTime = 0.0f, bool _explodeOnImpact = false );
    void FireRocket         ( Vector3 const &_pos, Vector3 const &_target, unsigned char _fromTeamId, double _lifeTime = -1.0f, int _fromBuildingId = -1, bool _firework = false );
    void FireLaser          ( Vector3 const &_pos, Vector3 const &_vel, unsigned char _fromTeamId, bool _mindControl = false );
    void FireTurretShell    ( Vector3 const &_pos, Vector3 const &_vel, int _teamId, int _fromBuildingId );
    void LaunchFireball     ( Vector3 const &_pos, Vector3 const &_vel, double _life = 1.0f, int _fromBuildingId = -1 );
    int Bang               ( Vector3 const &_pos, double _range, double _damage, int _teamId = -1, int _fromBuildingId = -1 ); // returns number of entities killed
    void CreateShockwave    ( Vector3 const &_pos, double _size, unsigned char _teamId=255, int _fromBuildingId = -1 );

    void RecalculateAITargets();

	bool MissionComplete	();

    void AdvanceChristmas();
    void AdvanceSoulRain ();
    void AdvanceDustWind ();
    void AdvanceLightning();

    bool SelectDarwinians       ( unsigned char teamId, Vector3 const &pos, double radius );
    void IssueDarwinianOrders   ( unsigned char teamId, Vector3 const &pos, bool directRoute );
    int  PromoteDarwinian       ( unsigned char teamId, int _entityId );                                // returns officer id
    int  DemoteOfficer          ( unsigned char teamId, int _entityId );
    void CreateRandomCrate      ();


    bool ChristmasModEnabled();           // 0 = unavailable, 1 = enabled, 2 = disabled

    WorldObjectId SpawnEntities  ( Vector3 const &_pos, unsigned char _teamId, int _unitId,
                              unsigned char _type, int _numEntities, Vector3 const &_vel, 
                              double _spread, double _range=-1.0, int _routeId = -1, int _routeWaypointId = -1 );
    
    int         GetSpirit   ( WorldObjectId _id );

    bool        IsFriend    ( unsigned char _teamId1, unsigned char _teamId2 );

    Team        *GetMyTeam	();    
	Entity		*GetEntity	( Vector3 const &_rayStart, Vector3 const &_rayDir );
	Building	*GetBuilding( Vector3 const &_rayStart, Vector3 const &_rayDir );
    TaskManager *GetMyTaskManager();

    WorldObject *GetWorldObject ( WorldObjectId _id );
    Entity		*GetEntity      ( WorldObjectId _id );
    Entity      *GetEntitySafe  ( WorldObjectId _id, unsigned char _type );                 // Safe to cast
    Unit        *GetUnit        ( WorldObjectId _id );
    WorldObject *GetEffect      ( WorldObjectId _id );
    Building    *GetBuilding    ( int _id );
    Spirit      *GetSpirit      ( int _index );

    void        InitMonsterTeam( int _teamId );
    int         GetMonsterTeamId();

    void        InitFuturewinianTeam( int _teamId );
    int         GetFuturewinianTeamId();

    void SetupFog			();

    void SetupLights		();    
    void SetupSpecialLights ();

	void WaterReflect       (); // inverts direction of all lights

    void SetCurrentMessage  ( const UnicodeString &_message, int _teamId = 255, bool _noOverwrite = false );

	void FlushOpenGlState	();
	void RegenerateOpenGlState();

	void DumpWorldToSyncLog( const char *_filename );

    int GetLastProcessedSlice();
    void SetLastProcessedSlice( int _slice);

    RGBAColour  GetGroupColour( int _groupId );
    void        SetGroupColourId( int _groupId, int _colourId );

    int         GetNumAttackers();
    void        GetCurrentTeamSpread( int &_attackers, int &_defenders );
    int         GetFreeAttackerPosition();
    int         GetFreeDefenderPosition();

    void        SetupAssault();
    void        SetupCoop   ( int _teamId );
    void        SetCoopTeam ( int _teamId );
    void        SetCoopTeam ( int _teamId, int _groupId );
    void        SetSoloTeam ( int _teamId, int _groupId );

    bool        IsAttacking( int _teamId );
    bool        IsDefending( int _teamId );


    void        NewChatMessage( UnicodeString _msg, int _fromTeamId );
    void        AddBurnPatch( Vector3 const &_pos, double _radius );

    bool        ViablePosition( Vector3 const &_pos );
    /*returns true if the location is on a viable land position
      ie. if you can walk to an ai target from that position
      ideally this should return false on any small isolated landmasses */
};



#endif
