#ifndef INCLUDED_LEVEL_FILE
#define INCLUDED_LEVEL_FILE

#include <stdlib.h>
#include "lib/tosser/llist.h"
#include "worldobject/worldobject.h"
#include "landscape.h"

#include "worldobject/crate.h"


#define CAMERA_MOUNT_MAX_NAME_LEN	63
#define CAMERA_ANIM_MAX_NAME_LEN	63
#define MAX_FILENAME_LEN			512
#define MAGIC_MOUNT_NAME_START_POS	"CamPosBefore"


class Building;
class TextReader;
class Route;
class GlobalEventCondition;
class TextWriter;


class CameraMount
{
public:
	char	m_name[CAMERA_MOUNT_MAX_NAME_LEN + 1];
	Vector3	m_pos;
	Vector3	m_front;
	Vector3 m_up;
};


class CamAnimNode
{
public:
	enum
	{
		TransitionMove,
		TransitionCut,
		TransitionNumModes
	};

	int		m_transitionMode;
	char	*m_mountName;
	double	m_duration;

public:
	CamAnimNode()
	:	m_mountName(NULL), 
		m_transitionMode(CamAnimNode::TransitionMove), 
		m_duration(1.0)
	{
	}

	~CamAnimNode() 
	{ 
		free(m_mountName); m_mountName = NULL; 
	}

	static int GetTransitModeId(char const *_word);
	static char const *GetTransitModeName(int _modeId);
};


class CameraAnimation
{
public:
	LList <CamAnimNode *>	m_nodes;
	char	m_name[CAMERA_ANIM_MAX_NAME_LEN + 1];

	~CameraAnimation() 
	{
		m_nodes.EmptyAndDelete();
	}
};


class InstantUnit
{
public:
    InstantUnit()
        :   m_type(-1),
            m_teamId(-1),
            m_posX(0.0),
            m_posZ(0.0),
            m_number(0),
            m_inAUnit(false),
            m_state(-1),
            m_spread(200.0),
            m_waypointX(0.0),
            m_waypointZ(0.0),
			m_routeId(-1),
            m_routeWaypointId(-1),
            m_runAsTask(false)
    {}

	int			m_type;
	int			m_teamId;
	double		m_posX;
	double		m_posZ;
	int			m_number;
    bool        m_inAUnit;
	int			m_state;
    double       m_spread;
    double       m_waypointX;
    double       m_waypointZ;
	int			m_routeId;
    int         m_routeWaypointId;
    bool        m_runAsTask;
};


class LandscapeFlattenArea
{
public:

    enum
    {
        FlattenTypeAbsolute,
        FlattenTypeAdd,
        FlattenTypeSubtract,
        FlattenTypeSmooth,
        FlattenTypeSubtract2
    };

    LandscapeFlattenArea()
    :   m_size(40.0),
        m_flattenType( FlattenTypeAbsolute ),
        m_heightThreshold(40.0)
    {
    }

	Vector3		m_centre;
	double		m_size;
    double       m_heightThreshold;
    int         m_flattenType;
};


class LandscapeDef
{
public:
	LList       <LandscapeTile *>           m_tiles;
	LList       <LandscapeFlattenArea *>    m_flattenAreas;
	double		m_cellSize;
	int			m_worldSizeX;
	int			m_worldSizeZ;
	double		m_outsideHeight;
    float       m_maxHeight;

    LandscapeDef()
    :   m_cellSize(12.0),
        m_worldSizeX(2000),
        m_worldSizeZ(2000),
        m_outsideHeight(-10),
        m_maxHeight(0.0f)
	{
	}

	~LandscapeDef()
	{
		m_tiles.EmptyAndDelete();
		m_flattenAreas.EmptyAndDelete();
	}
};


class RunningProgram
{
public:
    int     m_type;
    int     m_count;
    int     m_state;
    int     m_data;
    double   m_waypointX;
    double   m_waypointZ;
    
    double   m_positionX[10];
    double   m_positionZ[10];
    int     m_health[10];
};


// ***************************************************************************
// Class LevelFile
// ***************************************************************************

class LevelFile
{
private:
	void				ParseMissionFile			(char const *_filename);
	void				ParseMapFile				(char const *_filename);
    void                ParseMapFile                (TextReader *_in );
    void                ParseMWMFile                (char const *_filename);

	void				ParseCameraMounts			(TextReader *_in);
	void				ParseCameraAnims			(TextReader *_in);
	void				ParseBuildings              (TextReader *_in, bool _dynamic);
	void				ParseInstantUnits           (TextReader *_in);
	void				ParseLandscapeData          (TextReader *_in);
	void				ParseLandscapeTiles         (TextReader *_in);
	void				ParseLandFlattenAreas       (TextReader *_in);
	void				ParseLights                 (TextReader *_in);
	void				ParseRoute					(TextReader *_in, int _id);
	void				ParseRoutes					(TextReader *_in);
	void				ParsePrimaryObjectives		(TextReader *_in);
    void                ParseRunningPrograms        (TextReader *_in);
	void				ParseDifficulty				(TextReader *_in);
    void                ParseMultiwiniaOptions      (TextReader *_in);
	void				ParseLeaderboardStats		(TextReader *_in);

	void				GenerateAutomaticObjectives	();

	void				WriteCameraMounts			(TextWriter *_out);
	void				WriteCameraAnims			(TextWriter *_out);
	void				WriteBuildings              (TextWriter *_out, bool _dynamic);
	void				WriteInstantUnits           (TextWriter *_out);
	void				WriteLights                 (TextWriter *_out);
	void				WriteLandscapeData          (TextWriter *_out);
	void				WriteLandscapeTiles         (TextWriter *_out);
	void				WriteLandFlattenAreas       (TextWriter *_out);
	void				WriteRoutes					(TextWriter *_out);
	void				WritePrimaryObjectives		(TextWriter *_out);
    void                WriteRunningPrograms        (TextWriter *_out);
	void				WriteDifficulty				(TextWriter *_out);
    void                WriteMultiwiniaOptions      (TextWriter *_out);
	void                WriteLeaderboardStats       (TextWriter *_out);

public:

    enum // level options
    {
        LevelOptionNoHandicap = 0,          // Handicap is overridden to 0 regardless of game option
        LevelOptionInstantSpawnPointCapture,// spawn points have no 30 second capture timer and become active instantly
        LevelOptionTrunkPortReinforcements, // activates the RocketRiot style trunk port reinforcements on a level
        LevelOptionTrunkPortArmour,         // activates trunk port armour spawns ala rocket riot
        LevelOptionNoGunTurrets,            // disables GunTurret tasks from TrunkPorts on RocketRiot
        LevelOptionAttackerReinforcements,  // defines the number of darwinians attacking teams in assault receive as reinforcements
        LevelOptionDefenderReinforcements,  // defines the number of darwinians the defending team receieves
        LevelOptionDefenderReinforcementType,   // specify what type of reinforcement the defending team receives. replaces the TrunkPortArmour option (which must be active for this option to have effect). uses globalresearch type names
        LevelOptionAttackerReinforcementType,   // specify what type of reinforcements the attacking team recieves
        LevelOptionForceRadarTeamMatch,     // 2 radar dishes must be owned by the same team before they will connect
        LevelOptionNoRadarControl,          // radar dishes cannot be manually aimed
        LevelOptionAirstrikeCaptureZones,   // the number of flags you need to have captured in blitzkrieg before your gunturrets are replaced with airstrikes
        LevelOptionFuturewinianTeam,       // the team id specified here has Futurewinians instead of darwinians
        LevelOptionAttackReinforcementMode, // sets a specific option to the reinforcement task (ie, napalm strike, flame turret)
        LevelOptionDefendReinforcementMode, // sets a specific option to the reinforcement task (ie, napalm strike, flame turret)
        NumLevelOptions
    };

    enum
    {
        LevelEffectSnow,
        LevelEffectLightning,
        LevelEffectDustStorm,
        LevelEffectSoulRain,
        LevelEffectSpecialLighting,
        NumLevelEffects
    };

	char				m_missionFilename           [MAX_FILENAME_LEN];
	char				m_mapFilename               [MAX_FILENAME_LEN];
    char                m_levelName                 [MAX_FILENAME_LEN];

    char                m_landscapeColourFilename   [MAX_FILENAME_LEN];
    char                m_wavesColourFilename       [MAX_FILENAME_LEN];
    char                m_waterColourFilename       [MAX_FILENAME_LEN];
    char                m_thumbnailFilename         [MAX_FILENAME_LEN];

    // multiwinia level options
    int                 m_gameTypes;         
    bool                m_effects                   [NumLevelEffects];

    LList               <int>                       m_coopGroup1Positions;
    LList               <int>                       m_coopGroup2Positions;

    int                 m_populationCap;
    int                 m_defenderPopulationCap;    // used to enforce defender/attack ratios in Assault by limiting the number of darwinians per defending team
    int                 m_numPlayers;
    int                 m_levelOptions              [NumLevelOptions];
    int                 m_defaultPlayTime;
	double				m_leaderboardLevelTime;
	double				m_lastLeaderboardLevelTime;

    bool                m_attacking                 [NUM_TEAMS];    // used for assault mode, set to true for each attacking team
    bool                m_invalidCrates             [Crate::NumCrateRewards];
    bool                m_coop;                     // coop mode is optional
    bool                m_forceCoop;                // coop mode is mandatory
    bool                m_official;
	int					m_mapId;

    char                m_author[64];
    char                m_description[512];
    char                *m_difficultyRating;
    // =========

	LList				<CameraMount *>				m_cameraMounts;
	LList				<CameraAnimation *>			m_cameraAnimations;
	LList               <Building *>                m_buildings;
	LList				<InstantUnit *>			    m_instantUnits;
	LList               <Light *>                   m_lights;
	LList				<Route *>					m_routes;
    LList               <RunningProgram *>          m_runningPrograms;
	LList				<GlobalEventCondition *>	m_primaryObjectives;
	LList				<GlobalEventCondition *>	m_secondaryObjectives; // This data isn't stored in the map or mission files
																		   // directly, but is calculated at load time for your
																		   // convenience
	int					m_levelDifficulty;	// The difficulty factor that this level represents.

	LandscapeDef		m_landscape;
	
	LevelFile           ();
    LevelFile           (char const *_missionFilename, char const *_mapFilename, bool _textFileLevel = true);
	~LevelFile          ();

	void				Save();
    void                Save                ( TextWriter *_out );
    void                SaveMapFile         (TextWriter *_out );
    void                SaveMissionFile     (char const *_filename);

    Building           *GetBuilding         (int _id);
	CameraMount		   *GetCameraMount		(char const *_name);
	int					GetCameraAnimId		(char const *_name);
    void                RemoveBuilding      (int _id);
	int					GenerateNewRouteId	();
	Route			   *GetRoute			(int _id);

    void                GenerateInstantUnits();
    void                GenerateDynamicBuildings();

    bool                HasEffect( char *_effect );

    static int          CalculeCRC( char const *_mapFilename );
};


#endif
