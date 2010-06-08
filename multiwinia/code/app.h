#ifndef _INCLUDED_APP_H
#define _INCLUDED_APP_H

#include "lib/rgb_colour.h"
#include "lib/netlib/net_lib.h"
#include "lib/unicode/unicode_string.h"

#include "gametimer.h"

class Camera;
class Location;
class Server;
class ClientToServer;
class Renderer;
class UserInput;
class Resource;
class SoundSystem;
class LocationInput;
class LangTable;
class EffectProcessor;
class GlobalWorld;
class HelpSystem;
class Tutorial;
class ParticleSystem;
class TaskManagerInterface;
class TaskManagerInterface;
class Gesture;
class Script;
class Sepulveda;
class TestHarness;
class AVIGenerator;
class LocationEditor;
class MouseCursor;
class GameCursor;
class GameMenu;
class StartSequence;
class DemoEndSequence;
class AttractMode;
class ControlHelpSystem;
class BitmapRGBA;
class GameMenu;
class Multiwinia;
class ShamanInterface;
class MarkerSystem;
class MultiwiniaHelp;
class QNetManager;
class NetLib;

#ifdef BENCHMARK_AND_FTP
    class BenchMark;
#endif
class AchievementTracker;
class WorkQueue;
class DelayedJob;
class TextFileWriter;
class TextReader;
class EntityGrid;
class Team;

#include "lib/tosser/llist.h"
#include "globals.h"
#include <wchar.h>

#ifdef TESTBED_ENABLED
typedef enum TESTBED_MODE
{
	TESTBED_ON,
	TESTBED_OFF,

};

typedef enum TESTBED_TYPE
{
	TESTBED_CLIENT,
	TESTBED_SERVER,
};

typedef enum TESTBED_STATE
{
	TESTBED_MAINMENU,
	TESTBED_HOST_GAME,
	TESTBED_JOIN_GAME,
	TESTBED_PICK_SERVER,
	TESTBED_WAIT_FOR_CLIENT_ID,
	TESTBED_SELECT_GAME,
	TESTBED_SELECT_MAP,
	TESTBED_SETGAMEOPTIONSDELAY,
	TESTBED_SETGAMEOPTIONS,
	TESTBED_WAITFORGAMEOPTIONS,
	TESTBED_WAIT_FOR_MAP,
	TESTBED_WAIT_FOR_CLIENTS,
	TESTBED_PLAY,
	TESTBED_IDLE,
	TESTBED_GAME_OVER_DELAY,
	TESTBED_MAKE_CLIENT_SPECTATOR,
	TESTBED_SEQUENCEID_DELAY,
	TESTBED_BUILD_GAME_ENTRIES,
};

#endif

class App
{
public:
	// Library Code Objects
    UserInput           *m_userInput;
    Resource            *m_resource;
    SoundSystem         *m_soundSystem;
	ParticleSystem		*m_particleSystem;
	LangTable			*m_langTable;
    AVIGenerator        *m_aviGenerator;
	
	// Things that are the world
    GlobalWorld         *m_globalWorld;
    Location            *m_location;
	int					m_locationId;

	// Everything else
    Camera              *m_camera;
	Server              *m_server;                  // Server process, can be NULL if client
    ClientToServer      *m_clientToServer;          // Clients connection to Server
	NetMutex			m_networkMutex;
	NetMutex			m_delayedJobListMutex;

	char			    *m_originVersion;

#ifdef TESTBED_ENABLED
	// Testbed stuff
	TESTBED_MODE		m_eTestBedMode;
	TESTBED_TYPE		m_eTestBedType;
	TESTBED_STATE		m_eTestBedState;
	float				m_fTestBedDelay;
	float				m_fTestBedLastTime;
	float				m_fTestBedCurrentTime;
	int					m_iDesiredSequenceID;
	TESTBED_STATE		m_eNextState;
	int					m_iClientCount;
	int					m_iGameMode;
	int					m_iGameMap;
	int					m_iGameIndex;
	int					m_iLastGameIndex;
	int					m_iGameCount;
	int					m_iNumSyncErrors;
	char				m_cGameMode[100];
	char				m_cGameMap[100];
	int					m_iClientRandomSeed;
	bool				m_bRenderOn;
	int					m_iLastGame;
	int					m_iLastMap;
	UnicodeString		m_testbedServerName;
	int					m_iTerminationSequenceID;

	// TestBed functions
	TESTBED_MODE GetTestBedMode();
	void SetTestBedMode(TESTBED_MODE eMode);
	TESTBED_TYPE GetTestBedType();
	void SetTestBedType(TESTBED_TYPE eType);
	TESTBED_STATE GetTestBedState();
	void SetTestBedState(TESTBED_STATE eState);

	const UnicodeString &GetTestBedServerName();

	int GetTestBedGameIndex();
	void SetTestBedGameIndex(int iGameIndex);

	int GetTestBedGameMode();
	void SetTestBedGameMode(int iGameMode);
	int GetTestBedGameMap();
	void SetTestBedGameMap(int iGameMap);

	int GetTestBedGameCount();
	void IncrementGameCount();
	void ResetGameCount();

	int GetTestBedNumSyncErrors();
	void IncremementNumSyncErrors();
	void ResetNumSyncErrors();

	void SetTestBedClientRandomSeed(int iRandomSeed);
	int GetTestBedClientRandomSeed();

	void SetSequenceIDDelay(int iSequenceID, TESTBED_STATE eNextState);

	void ToggleTestBedRendering();	
	bool GetTestBedRendering();
	void SetTestBedRenderOn();
	void SetTestBedRenderOff();

	void TestBedLogWrite();

#endif

	NetLib				*m_netLib;

	NetThreadId			m_mainThreadId;
    Renderer            *m_renderer;
	LocationInput		*m_locationInput;
	LocationEditor		*m_locationEditor;
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    HelpSystem          *m_helpSystem;
    Tutorial            *m_tutorial;
#endif
	EffectProcessor		*m_effectProcessor;
    TaskManagerInterface *m_taskManagerInterface;
    Gesture             *m_gesture;
    Script              *m_script;
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    Sepulveda           *m_sepulveda;
#endif
    TestHarness         *m_testHarness;
    GameCursor          *m_gameCursor;
    StartSequence       *m_startSequence;
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    DemoEndSequence     *m_demoEndSequence;
#endif
	AttractMode			*m_attractMode;
    ControlHelpSystem   *m_controlHelpSystem;
    GameMenu            *m_gameMenu;
    Multiwinia          *m_multiwinia;
	ShamanInterface		*m_shamanInterface;
    MarkerSystem        *m_markerSystem;
    MultiwiniaHelp      *m_multiwiniaHelp;
#ifdef BENCHMARK_AND_FTP
    BenchMark           *m_benchMark;
#endif

    AchievementTracker  *m_achievementTracker;
    
    bool                m_negativeRenderer;
	int					m_difficultyLevel;			// Cached from preferences
	bool				m_largeMenus;

	bool				m_usingFontCopies;
	bool				m_steamInited;

	// State flags
    bool                m_userRequestsPause;
	bool				m_lostFocusPause;
	bool                m_editing;

	// Requested state flags
	int					m_requestedLocationId;		// -1 for global world
	bool                m_requestToggleEditing;
	bool				m_requestQuit;

    char                m_userProfileName[256];
    char                m_requestedMission[256];
    char                m_requestedMap[256];
    bool                m_levelReset;
    char                m_gameDataFile[256];

	RGBAColour			m_backgroundColour;

    enum
    {
        MultiwiniaTutorial1,
        MultiwiniaTutorial2
    };

    bool                m_atMainMenu;       // true when the player is viewing the darwinia/mutliwinia menu
	bool				m_atLobby;			// Viewing Lobby (Multiplayer Page)
    int                 m_gameMode;
	bool				m_loadingLocation;
    bool                m_spectator;
    bool                m_multiwiniaTutorial;
    int                 m_multiwiniaTutorialType;

    bool                m_hideInterface;
	bool				m_soundsLoaded;
	bool				m_requireSoundsLoaded;

    bool                m_doMenuTransition;
	bool				m_checkedForPDLC;

    // these should be renamed once the list of achievements has been finalised
	enum
    {
        DarwiniaAchievementDominator = 0,       // domnation achievement
        DarwiniaAchievementHoarder,             // CTS achievement
        DarwiniaAchievementHailToTheKing,       // king of the hill achievement
        DarwiniaAchievementUncladSkies,         // rocket riot achievement
		DarwiniaAchievementAggravatedAssault,	// Assault achievement
        DarwiniaAchievementMasterPlayer,        // Beat someone who already has Master Player
        DarwiniaAchievementMasterOfMultiwinia,  // won a game on every single game map
        DarwiniaAchievementExplorer,            // played a game on every single map
        DarwiniaAchievementCarnage,             // killed more than 3000 enemy darwinians in a single game
        DarwiniaAchievementBlitzMaster,         // blitzkrieg achievement
        DarwiniaAchievementWrongGame,           // nuke one of your allies in a 4p coop game
		DarwiniaAchievementGenocide,			// Kill a total of 100,000 Darwinians
        NumAchievements
    };

#ifdef TARGET_OS_VISTA
	BitmapRGBA			*m_thumbnailScreenshot;
	bool				m_saveThumbnail;
#endif
	
    enum
    {
        GameModeNone,
        GameModePrologue,
        GameModeCampaign,
        GameModeMultiwinia,
        NumGameModes
    };

public:
    App ();
	~App();

    void    SetProfileName  ( const char *_profileName );
    bool    LoadProfile     ();
    bool    SaveProfile     ( bool _global, bool _local );
    void    ResetLevel      ( bool _global );

	void	HandleDelayedJobs();
	void	AddDelayedJob(DelayedJob* _dJob);

	void	StartNetwork( bool _iAmAServer, const char *_serverIp, int _serverPort );
	bool	StartSinglePlayerServer();
	HRESULT	StartMultiPlayerServer();
	void	ShutdownCurrentGame();
	
	std::string GetFirstAvailableLanguage();
	const char *GetDefaultLanguage();
	void	InitLanguage();
    void    SetLanguage     ( const char *_language, bool _test );
	bool	TrySetLanguage	( std::string _language );

	void	InitMetaServer	();

	bool	Multiplayer		();
	bool	IsSinglePlayer	();

	static void        UseProfileDirectory( bool _profileDirectory );
	static const char *GetProfileDirectory();
	static const char *GetPreferencesPath();
    static const char *GetScreenshotDirectory();
    static const char *GetMapDirectory();
	
	void	UpdateDifficultyFromPreferences();

	bool	ToggleGamePaused();
	bool	GamePaused		() const;

    bool    EarnedAchievement( int _achievementId );
    void    CheckMasterAchievement();  // check to see if the signed in player is on the master list, and give the acheivement if they are
    void    GiveAchievement ( int _achievementId );

    bool    IsFullVersion();

	bool	MultiplayerPermitted();
	bool	HostGamePermitted();
    bool    IsMapAvailableInDemo( int gameMode, int mapId );
    bool    IsGameModePermitted ( int gameMode );
    bool    IsMapPermitted      ( int gameMode, int mapId );
    bool    IsMapPermitted      ( int mapId );
    int     GetMapID            ( int gameMode, int mapCrcId );

	const char *GetBuyNowURL() const;

#ifdef	TARGET_OS_VISTA
	void	SaveRichHeader();
	void	SaveThumbnailScreenshot();
#endif

	void CheckSounds();

	int GetMaxNumberofPlayers();
    bool    UseChristmasMode();

private:
	WorkQueue	*m_soundsWorkQueue;

	LList<DelayedJob*>	m_delayedJobs;
	
    void    Initialise();
	void	LoadSounds();

	LangTable*	m_oldLangTable;
	void		DeleteOldLangTable();

	void	DoStartServer();
	void	DoCheckForPDLC();
};

#ifdef TARGET_OS_VISTA
#define RM_MAXLENGTH     1024 
#define RM_MAGICNUMBER   'HMGR' 

#pragma pack(push)
#pragma pack(1)
typedef  struct  _RICH_GAME_MEDIA_HEADER 
{ 
     DWORD        dwMagicNumber; 
     DWORD        dwHeaderVersion; 
     DWORD        dwHeaderSize; 
     LARGE_INTEGER liThumbnailOffset; 
     DWORD        dwThumbnailSize; 
     GUID         guidGameId; 
     WCHAR        szGameName[RM_MAXLENGTH]; 
     WCHAR        szSaveName[RM_MAXLENGTH]; 
     WCHAR        szLevelName[RM_MAXLENGTH]; 
     WCHAR        szComments[RM_MAXLENGTH]; 
}  RICH_GAME_MEDIA_HEADER;
#pragma pack(pop)

#endif

extern App *g_app;

#endif
