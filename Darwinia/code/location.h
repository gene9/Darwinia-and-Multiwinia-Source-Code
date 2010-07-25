#ifndef INCLUDED_LOCATION_H
#define INCLUDED_LOCATION_H

#include <float.h>

#include "lib/fast_darray.h"
#include "lib/slice_darray.h"
#include "lib/vector3.h"

#include "network/server.h"

#include "globals.h"
#include "landscape.h"
#include "worldobject/building.h"
#include "worldobject/worldobject.h"
#include "worldobject/weapons.h"
#include "worldobject/spirit.h"


class ServerToClientLetter;
class WorldObject;
class WorldObjectEffect;
class Entity;
class EntityGrid;
class ObstructionGrid;
class WorldObjectId;
class Unit;
class LaserGod;
class LevelFile;
class Clouds;
class Water;
class Light;
class Team;
class TeamControls;


// ****************************************************************************
//  Class Location
// ****************************************************************************

class Location
{
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

    void InitLandscape			();
    void InitLights				();
    void InitTeams				();

	void DoMissionCompleteActions();

    Vector3     FindValidSpawnPosition( Vector3 const &_pos, float _spread );

public:
    Landscape       m_landscape;
    EntityGrid      *m_entityGrid;
    ObstructionGrid *m_obstructionGrid;
	LevelFile		*m_levelFile;
    Clouds          *m_clouds;
    Water           *m_water;

    Team            *m_teams;

    float           m_christmasTimer;

	FastDArray		<Light *>			m_lights;
    SliceDArray     <Building *>		m_buildings;
    SliceDArray     <Spirit>			m_spirits;
    SliceDArray     <Laser>				m_lasers;
    SliceDArray     <SubversionBeam>	m_subversion;
    SliceDArray     <WorldObject *>		m_effects;

	bool			m_isSnowing;
public:
    Location();
    ~Location();

    void Init               ( char const *_missionFilename, char const *_mapFilename );
    void InitBuildings			();
	void Empty				();

    void Advance            ( int _slice );
    void Render             ( bool renderWaterAndClouds = true );

    void InitialiseTeam     ( unsigned char _teamId,
                              unsigned char _teamType );

    void RemoveTeam         ( unsigned char _teamId );

    int GetBuildingId           ( Vector3 const &startRay, Vector3 const &direction, unsigned char teamId, float _maxDistance=FLT_MAX, float *_range=NULL );
    int GetUnitId               ( Vector3 const &startRay, Vector3 const &direction, unsigned char teamId, float *_range=NULL );
    WorldObjectId GetEntityId   ( Vector3 const &startRay, Vector3 const &direction, unsigned char teamId, float *_range=NULL );

    bool IsWalkable         ( Vector3 const &_from, Vector3 const &_to, bool _evaluateCliffs=false );
    bool IsVisible          ( Vector3 const &_from, Vector3 const &_to );

    void UpdateTeam         ( unsigned char teamId, TeamControls const& teamControls );

    int  SpawnSpirit        ( Vector3 const &_pos, Vector3 const &_vel, unsigned char _teamId, WorldObjectId _id );
    void ThrowWeapon        ( Vector3 const &_pos, Vector3 const &_target, int _type, unsigned char _fromTeamId );
    void FireRocket         ( Vector3 const &_pos, Vector3 const &_target, unsigned char _fromTeamId );
    void FireLaser          ( Vector3 const &_pos, Vector3 const &_vel, unsigned char _fromTeamId );
    void FireSubversion     ( Vector3 const &_pos, Vector3 const &_vel, unsigned char _fromTeamId );
    void FireTurretShell    ( Vector3 const &_pos, Vector3 const &_vel );
    void Bang               ( Vector3 const &_pos, float _range, float _damage );
    void CreateShockwave    ( Vector3 const &_pos, float _size, unsigned char _teamId=255 );

	bool MissionComplete	();

    void AdvanceChristmas();
    static int ChristmasModEnabled();           // 0 = unavailable, 1 = enabled, 2 = disabled

    WorldObjectId SpawnEntities  ( Vector3 const &_pos, unsigned char _teamId, int _unitId,
                              unsigned char _type, int _numEntities, Vector3 const &_vel,
                              float _spread, float _range=-1.0f, int _routeId = -1, int _routeWaypointId = -1 );

    int         GetSpirit   ( WorldObjectId _id );

    bool        IsFriend    ( unsigned char _teamId1, unsigned char _teamId2 );

    Team        *GetMyTeam	();
	Entity		*GetEntity	( Vector3 const &_rayStart, Vector3 const &_rayDir );
	Building	*GetBuilding( Vector3 const &_rayStart, Vector3 const &_rayDir );

    WorldObject *GetWorldObject ( WorldObjectId _id );
    Entity		*GetEntity      ( WorldObjectId _id );
    Entity      *GetEntitySafe  ( WorldObjectId _id, unsigned char _type );                 // Safe to cast
    Unit        *GetUnit        ( WorldObjectId _id );
    WorldObject *GetEffect      ( WorldObjectId _id );
    Building    *GetBuilding    ( int _id );
    Spirit      *GetSpirit      ( int _index );

    void SetupFog			();
    void SetupLights		();

	void WaterReflect       (); // inverts direction of all lights

	void FlushOpenGlState	();
	void RegenerateOpenGlState();
};



#endif
