#ifndef _included_multiwinia_h
#define _included_multiwinia_h

#include "team.h"
#include "globals.h"
#include "global_world.h"

class MultiwiniaGameBlueprint;
class MultiwiniaGameParameter;
class RocketStatusPanel;


// ============================================================================
// class Multiwinia

#define     MULTIWINIA_MAXPARAMS    64
#define     MULTIWINIA_NUM_TEAMCOLOURS 8
#define     MULTIWINIA_DRAW_TEAM     255     // team id used by m_winner in the case of a tie

#define     ASSAULT_ATTACKER_COLOURID   1
#define     ASSAULT_DEFENDER_COLOURID   0

#define     BLITZKRIEGSCORE_ELIMINATED      10000      // or less
#define     BLITZKRIEGSCORE_ENEMYOCCUPIED   10001      // home zone occupied by enemy
#define     BLITZKRIEGSCORE_UNDERATTACK     10002      // Under attack by enemy team
#define     BLITZKRIEGSCORE_UNOCCUPIED      10003      // un-owned home zone
#define     BLITZKRIEGSCORE_UNDERTHREAT     10004      // home zone held but under threat
#define     BLITZKRIEGSCORE_SAFE            10005      // home zone held and locked

class Multiwinia
{
protected:
    double	m_timer;

    void    AdvanceGracePeriod();
    void    AdvanceSkirmish();
    void    AdvanceKingOfTheHill();
    void    AdvanceChess();
    void    AdvanceShaman();
    void    AdvanceRocketRiot();
    void    AdvanceCaptureStatue();
    void    AdvanceTankBattle();
    void    AdvanceAssault();
    void    AdvanceBlitkrieg();

    void    AdvanceTrunkPorts();
    void    AdvanceRetribution();

    void    EnsureStartingStatue();

	void	LoadBlueprints();

    void    RenderOverlay_GracePeriod();
    void    RenderRocketStatusPanel();

    void    CheckMasterAchievement();
    void    CheckMentorAchievement();

public:
    int			m_gameType;
    int			m_params        [MULTIWINIA_MAXPARAMS];    
	
	LobbyTeam	m_teams			[NUM_TEAMS];

    int			m_timeRemaining;
    int         m_winner;
    
    double      m_statueSpawner;
    
    bool        m_rocketsFound;
    double      m_reinforcementTimer;
    int         m_reinforcementCountdown;
    int         m_turretTimer;
    int         m_tankDeaths    [NUM_TEAMS];

    int         m_currentPositions[NUM_TEAMS];
    double      m_positionTimer;

    bool        m_initialised;
	bool		m_defendedAll;

    LList<int>  m_assaultObjectives;                // building IDs for assault : fences, control stations

    double      m_startTime;
    double      m_victoryTimer;

    int         m_aiType;

    bool        m_resetLevel;

    double      m_pulseTimer;
    double      m_timeRecords[NUM_TEAMS];
    int         m_numRotations;
    bool        m_pulseDetonated;

    double      m_gracePeriodTimer;    
    bool        m_oneMinuteWarning;

    bool        m_eliminated    [NUM_TEAMS];
    int         m_numEliminations[NUM_TEAMS];   // tracks the number of team eliminations each team has scored (blitzkrieg)
    double      m_timeEliminated[NUM_TEAMS];    // the GetNetworkTime() that a team was eliminated (dom)
    int         m_eliminationTimer[NUM_TEAMS];
    double       m_eliminationBonusTimer[NUM_TEAMS];

    bool        m_coopMode;
    bool        m_assaultSingleRound;
    bool        m_suddenDeathActive;
    bool        m_unlimitedTime;
    bool        m_basicCratesOnly;

    bool        m_firstReinforcements;

    int         m_teamsStatus[NUM_TEAMS];

    RocketStatusPanel   *m_rocketStatusPanel;

    enum
    {
        AITypeEasy,
        AITypeStandard,
        AITypeHard,
        NumAITypes
    };

    enum
    {
        GameTypeSkirmish,			// 0
        GameTypeKingOfTheHill,		// 1
        GameTypeCaptureTheStatue,	// 2
        GameTypeAssault,			// 3
        GameTypeRocketRiot,			// 4
        GameTypeBlitzkreig,			// 5
        GameTypeTankBattle,			// 6
		NumGameTypes
    };

public:
    Multiwinia();
	~Multiwinia();

	void Reset();
    void ResetLocation  ();   // used to reload the level in assault mode

    void InitialiseTeam     ( unsigned char _teamId, unsigned char _teamType, int _clientId );
	void RemoveTeams        ( int _clientId/*, int _reason */ );
    void SetGameResearch    ( int *_research );
	bool ValidGameOption	( int _optionIndex );
	void SetGameOption		( int _optionIndex, int _param );
    void SetGameOptions     ( int _gameType, int *_params );
    int  GetGameOption      ( char *_name );

    void AwardPoints        ( int _teamId, int _points );
    int  GetMaxPointsAvail  ();

    void Advance();
    void Render();

    bool GameInGracePeriod();
	bool GameInLobby();
    bool GameRunning();
    bool GameOver();

    void EliminateTeam      ( int _teamId );
    void RecordDefenseTime  ( double _time, int _teamId );
    bool IsEliminated       ( int _teamId );

	int GetNumHumanTeams() const;
	int GetNumCpuTeams() const;
	static std::string GameTypeString( int _gameType );

    int  CountDarwinians ( int _teamId );

    void DeclareWinner( int _teamId );

	void SaveIFrame( int _clientId, IFrame *_frame );	// Takes ownership of the IFrame

    RGBAColour GetDefaultTeamColour( int _teamId );
    RGBAColour GetColour( int _colourId );
    RGBAColour GetGroupColour( int _memberId, int _colourId );

    void SetTeamColour( int _teamId, int _colourId );

    int GetNextAvailableColour();
    int GetCPUColour();
    int IsColourTaken( int _colourId );
    bool IsValidCoopColour( int _colourId );

    double GetPreciseTimeRemaining();

private:
	void ZeroScores();
	void BackupScores();
	void GameOptionsChanged();

	bool GetScoreLine( int _teamId, wchar_t *_statusLine, size_t bufferBytes );

public:
    static DArray   <MultiwiniaGameBlueprint *> s_gameBlueprints;
};


// ============================================================================
// class MultiwiniaGameBlueprint

class MultiwiniaGameBlueprint
{
public:
    char    m_name[256];
    DArray  <MultiwiniaGameParameter *> m_params;

	char	m_stringId[256];

public:
	MultiwiniaGameBlueprint( const char *_name );
	~MultiwiniaGameBlueprint();

    char *GetName();

    static char *GetDifficulty( int _type );
};



// ============================================================================
// class MultiwiniaGameParameter

class MultiwiniaGameParameter
{
private:
	char	m_stringId[256];

public:
    char    m_name[256];
    double  m_min;
    double  m_max;
    double  m_default;
    int     m_change;    
    
public:
	MultiwiniaGameParameter( const char *_name );

    void	GetNameTranslated(UnicodeString& _dest);
    char    *GetStringId();
};


#endif
