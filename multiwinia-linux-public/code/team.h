#ifndef _INCLUDED_TEAM_H
#define _INCLUDED_TEAM_H

#include "lib/tosser/fast_darray.h"
#include "lib/tosser/llist.h"
#include "lib/slice_darray.h"
#include "lib/rgb_colour.h"

#include "worldobject/worldobject.h"
#include "worldobject/entity.h"

class Unit;
class InsertionSquad;
class TaskManager;
class IFrame;
class SyncDiff;

enum 
{
    TeamTypeUnused = -1,
    TeamTypeLocalPlayer,
    TeamTypeRemotePlayer,
    TeamTypeCPU,
	TeamTypeSpectator,
};


// ****************************************************************************
//  Class LobbyTeam
// ****************************************************************************
class LobbyTeam
{
public:
	int				m_teamId;
	int				m_clientId;
	int				m_teamType;

    int             m_colourId;
    RGBAColour      m_colour;
    int             m_coopGroupPosition;

	int				m_score;
	int				m_prevScore;
    int             m_rocketId;

    bool            m_disconnected;
    bool            m_ready;
    bool            m_demoPlayer;

    UnicodeString   m_teamName;

	float			m_accurateCPUTakeOverTime;

	unsigned char	m_achievementMask;

	// For sync error tracking
	IFrame			*m_iframe;
	LList<SyncDiff *>m_syncDifferences;

public:
	LobbyTeam();
	~LobbyTeam();
};

// ****************************************************************************
//  Class Team
// ****************************************************************************

class Team 
{
public:

    int                             &m_teamId;
	int								&m_clientId;
    int                             &m_teamType;
	RGBAColour                      &m_colour;
	int								m_colourID;
	LobbyTeam						*m_lobbyTeam;

    FastDArray  <Unit *>            m_units;
    SliceDArray <Entity *>          m_others;
    LList       <WorldObjectId>     m_specials;             // Officers and tanks for quick lookup

    TaskManager                     *m_taskManager;
    
    bool            m_ready;

    int             m_currentUnitId;                    // 
    int             m_currentEntityId;                  // Do not set these directly
    int             m_currentBuildingId;                // They are updated by the network
                                                        //
    Vector3         m_currentMousePos;                  // 

    bool            m_monsterTeam;                      // this a monster team for Multiplayer, which contains monsters to attack all actual teams
    bool            m_futurewinianTeam;                 // Team for the futurewinians
    
    int				m_numDarwiniansSelected;
	Vector3			m_selectedDarwinianCentre;

	int				m_newNumDarwiniansSelected;
	Vector3			m_newSelectedDarwinianCentre;

    Vector3         m_selectionCircleCentre;
    double           m_selectionCircleRadius;
    double          m_lastOrdersSet;

    int             m_teamKills; // tracks the number of enemy darwinians the team has killed during the course of a level
    int             m_oldKills;

    bool            m_blockUnitChange;

    enum
    {
        EvolutionFormationSize,
        EvolutionRockets,
        NumEvolutions
    };

    int             m_evolutions[NumEvolutions];

public:
    Team( LobbyTeam &_lobbyTeam );
    ~Team();

    void Initialise     ();								// Call when this team enters the game
    void SetTeamType    (int _teamType);

    void SelectUnit     (int _unitId, int _entityId, int _buildingId );

    void RegisterSpecial    ( WorldObjectId _id );
    void UnRegisterSpecial  ( WorldObjectId _id );

	Entity *RayHitEntity(Vector3 const &_rayStart, Vector3 const &_rayEnd);
    Unit   *GetMyUnit   ();
    Entity *GetMyEntity ();
    Unit   *NewUnit     (int _troopType, int _numEntities, int *_unitId, Vector3 const &_pos);
    Entity *NewEntity   (int _troopType, int _unitId, int *_index);

    UnicodeString   GetTeamName();

    int  NumEntities    (int _troopType);               // Counts the total number

    void Advance        (int _slice);
    
    void Render             ();
    void RenderVirii        (double _predictionTime);
    void RenderDarwinians   (double _predictionTime);
    void RenderOthers       (double _predictionTime);
	void RenderSyncErrors	();

    static char *GetTeamType( int _teamType );

    bool IsFuturewinianTeam();

    void Eliminate();   // wipes all of this teams active units and entities
};


// ****************************************************************************
//  Class TeamControls
//
//   capture all the control information necessary to send
//   over the network for "remote" control of units
// ****************************************************************************

class TeamControls {
public:
	TeamControls();

	unsigned short GetFlags() const;
	void SetFlags( unsigned short _flags );
	void ClearFlags();
	void Advance();
	void Clear();

	void Pack( char *_data, int &_length ) const; 
	void Unpack( const char *_data, int _length ); 

public:
	
	Vector3			m_mousePos;

	// Be sure to update GetFlags, SetFlags, ZeroFlags if you change these flags
	// Also, NetworkUpdate::GetByteStream and NetworkUpdate::ReadByteStream
	// if you use more than 8 bits

	unsigned int	m_unitMove : 1;
	unsigned int	m_primaryFireTarget : 1;
	unsigned int	m_secondaryFireTarget : 1;
	unsigned int	m_targetDirected : 1;
	unsigned int	m_secondaryFireDirected : 1;
	unsigned int	m_cameraEntityTracking : 1;
	unsigned int	m_directUnitMove : 1;
	unsigned int	m_unitSecondaryMode : 1;
	unsigned int	m_endSetTarget : 1;
	unsigned int	m_consoleController : 1;
    unsigned int    m_leftButtonPressed : 1;

	short			m_directUnitMoveDx;
	short			m_directUnitMoveDy;
    short			m_directUnitTargetDx;
    short           m_directUnitTargetDy;
};

#endif
