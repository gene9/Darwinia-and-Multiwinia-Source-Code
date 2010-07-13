#include "lib/universal_include.h"
#include "lib/unicode/unicode_text_stream_reader.h"

#include <stdlib.h>
#include <stdio.h>

#if defined(TARGET_MSVC)
#include <shlobj.h>
#endif

#include <time.h>

#include "network/clienttoserver.h"
#include "network/demo_limitations.h"

#include "lib/metaserver/metaserver_defines.h"
#include "lib/metaserver/metaserver.h"
#include "lib/metaserver/authentication.h"
#include "lib/metaserver/matchmaker.h"

#ifdef TARGET_OS_VISTA
#include "lib/poster_maker.h"
#endif
#include "lib/avi_generator.h"
#include "lib/hi_res_time.h"
#include "lib/language_table.h"
#include "lib/mouse_cursor.h"
#include "lib/preferences.h"
#include "lib/resource.h"
#include "lib/profiler.h"
#include "lib/system_info.h"
#include "lib/text_renderer.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/bitmap.h"
#include "lib/filesys/text_file_writer.h"
#include "interface/prefs_other_window.h"
#include "lib/window_manager.h"

#include "sound/sound_stream_decoder.h"
#include "sound/soundsystem.h"
#include "sound/sample_cache.h"

#include "lib/netlib/net_lib.h"

#include "app.h"
#include "camera.h"
#include "effect_processor.h"
#include "gesture.h"
#include "global_world.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "helpsystem.h"
    #include "tutorial.h"
#endif
#include "keygen.h"
#include "location.h"
#include "location_input.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "script.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "user_input.h"
#include "taskmanager_interface.h"
#include "taskmanager_interface_gestures.h"
#include "taskmanager_interface_icons.h"
#include "gamecursor.h"
#include "level_file.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "demoendsequence.h"
#endif
#include "attract.h"
#include "control_help.h"
#include "game_menu.h"
#include "multiwinia.h"
#include "shaman_interface.h"
#include "explosion.h"
#include "markersystem.h"
#include "multiwiniahelp.h"
#ifdef BENCHMARK_AND_FTP
    #include "benchmark.h"
#endif
#include "achievement_tracker.h"
#include "loading_screen.h"
#include "game_menu.h"
#include "entity_grid.h"
#include "team.h"
#include "UI/MapData.h"

#include "UI/GameMenuWindow.h"

#ifdef TARGET_OS_MACOSX
#include <CoreServices/CoreServices.h> // for FSFindFolder()
#include <sys/stat.h>
#endif

#ifdef TESTBED_ENABLED
#include <fstream>
#include "lib/map_utils.h"
#endif

#include "mapcrc.h"

void SetPreferenceOverrides(); // See main.cpp

App *g_app = NULL;

static bool s_profileDirectory = true;


#ifdef DEMO2
    #define GAMEDATAFILE "game_demo2.txt"
#else
    #ifdef DEMOBUILD
        #define GAMEDATAFILE "game_demo.txt"
    #else
        #define GAMEDATAFILE "game.txt"
    #endif
#endif

static void SetWorkingThreadProcessor()
{
#if defined( WIN32 )
	SetThreadAffinityMask( GetCurrentThread(), 0x01 | 0x4 | 0x10 | 0x40 ); // Processor Number % 3 == 2
#endif
}

App::App()
:   m_camera(NULL), 
    m_location(NULL),
	m_locationId(-1),
    m_server(NULL),
    m_clientToServer(NULL),
    m_renderer(NULL),
    m_userInput(NULL),
    m_resource(NULL),
    m_soundSystem(NULL),
	m_locationInput(NULL),
	m_locationEditor(NULL),
    m_aviGenerator(NULL),
	m_effectProcessor(NULL),
    m_globalWorld(NULL),
	m_originVersion("unknown"),
#ifdef TESTBED_ENABLED
	m_eTestBedMode(TESTBED_OFF),
	m_testbedServerName("TESTBED"),
#endif
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    m_helpSystem(NULL),
    m_tutorial(NULL),
#endif
	m_particleSystem(NULL),
    m_taskManagerInterface(NULL),
    m_gesture(NULL),
    m_script(NULL),
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    m_sepulveda(NULL),
#endif
    m_testHarness(NULL),
    m_userRequestsPause(false),
	m_lostFocusPause(false),
	m_editing(false),
	m_requestedLocationId(-1),
	m_requestToggleEditing(false),
	m_requestQuit(false),
    m_negativeRenderer(false),
	m_difficultyLevel(0),
    m_levelReset(false),
    m_langTable(NULL),
    m_startSequence(NULL),
	m_atLobby(false),
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    m_demoEndSequence(NULL),
#endif
	m_largeMenus(false),
	m_usingFontCopies(false),
	m_attractMode(NULL),
    m_controlHelpSystem(NULL),
    m_atMainMenu(true),
    m_gameMenu(NULL),
    m_gameMode(GameModeNone),
    m_multiwinia(NULL),
	m_shamanInterface(NULL),
    m_hideInterface(false),
#ifdef BENCHMARK_AND_FTP
    m_benchMark(NULL),
#endif
	m_soundsLoaded( false ),
	m_mainThreadId( NetGetCurrentThreadId() ),
	m_soundsWorkQueue( new WorkQueue ),
    m_spectator(false),
	m_loadingLocation( false ),
	m_oldLangTable( NULL ),
    m_doMenuTransition(false),
	m_checkedForPDLC(false),
    m_multiwiniaTutorial(false),
	m_requireSoundsLoaded(false),
    m_multiwiniaTutorialType(MultiwiniaTutorial1),
	m_steamInited(false)
#ifdef TARGET_OS_VISTA
	,m_thumbnailScreenshot(NULL),
	m_saveThumbnail(false)
#endif
{
    g_app = this;

    m_netLib = new NetLib();
    m_netLib->Initialise();

#if defined( WIN32 )
	SetThreadAffinityMask( GetCurrentThread(), 0x02 | 0x8 | 0x20 | 0x80 ); // Even processors
#endif
	
    m_resource = new Resource();

	m_resource->ParseArchive( "init.dat", NULL );    

    g_prefsManager = new PrefsManager(App::GetPreferencesPath());
	SetPreferenceOverrides();  

    m_renderer = new Renderer();
    m_renderer->Initialise();
    m_renderer->SetOpenGLState();

	// ask opengl what it's max texture size is
	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	
	// set it on the bitmap class so any bitmaps larger than the max size will get scaled
	BitmapRGBA::SetMaxTextureSize(maxTextureSize);
	
#ifdef SPECTATOR_ONLY
    m_spectator = true;
#endif

#ifdef TESTBED_ENABLED
	m_testbedServerName = g_prefsManager->GetUnicodeString( "TestbedServerName", m_testbedServerName );
#endif

	g_loadingScreen->m_workQueue->Add( &App::Initialise, this );

#ifdef TARGET_OS_MACOSX
	g_loadingScreen->m_workQueue->Add( &App::LoadSounds, this );
	g_loadingScreen->Render();
#else
	// Need to serialise the loading of the sounds on PC 
	// Can't unrar too files at once (unrar not thread safe, regrettably)
	g_loadingScreen->Render();	
	m_soundsWorkQueue->Add( &SetWorkingThreadProcessor );
	m_soundsWorkQueue->Add( &App::LoadSounds, this );
#endif
}

const char *App::GetDefaultLanguage()
{
    return g_systemInfo->m_localeInfo.m_language;
}

std::string App::GetFirstAvailableLanguage()
{
	LList<char*> *files = g_app->m_resource->ListResources("language/", "*.*", false);
	if( files->Size() > 0 )
	{
		std::string first = files->GetData(0);
		return first.substr( 0, first.find( '.' ) );
	}
	files->EmptyAndDeleteArray();
	delete files;

	return "unknown";
}

bool App::TrySetLanguage( std::string _language )
{
	std::string languageFile = "language/" + _language + ".txt";
	StrToLower( languageFile );

	if( !m_resource->FileExists( languageFile.c_str() ) )
		return false;

	const char *language = _language.c_str();
	g_prefsManager->SetString( "TextLanguage", language );

    SetLanguage( language, g_prefsManager->GetInt( "TextLanguageTest", 0 ) );
	return true;
}

void App::InitLanguage()
{
	std::list< std::string > languagePreference;

	// On OS X we always get the preferred language from the OS
#ifndef TARGET_OS_MACOSX
	languagePreference.push_back( g_prefsManager->GetString( "TextLanguage", "unknown" ) );
#endif
	languagePreference.push_back( GetDefaultLanguage() );
	languagePreference.push_back( "english" );
	languagePreference.push_back( GetFirstAvailableLanguage() );

	AppReleaseAssert(
		languagePreference.end() !=
		std::find_if( languagePreference.begin(), languagePreference.end(), 
			std::bind1st( std::mem_fun( &App::TrySetLanguage ), this ) ),
		"Failed to load language file" );
}

void App::Initialise()
{
    strcpy( m_requestedMission, "null" );
    strcpy( m_requestedMap, "null" );

	// Set the origin version

	TextFileReader originReader( "origin" );
	if( originReader.IsOpen() )
	{
		if( originReader.ReadLine() )
		{
			const char *origin = originReader.GetRestOfLine();
			if( origin )
			{
				m_originVersion = strdup( origin );
			}
		}
	}



	// Load resources

	double start = GetHighResTime();

	m_resource->ParseArchive( "language.dat", NULL );	
	InitLanguage();

	m_resource->ParseArchive( "main.dat", NULL );   
	m_resource->ParseArchive( "patch.dat", NULL );	
	m_resource->ParseArchive( "menusounds.dat", NULL );
    m_resource->ParseArchive( "branding.dat", NULL );
	

	//m_soundsLoaded = true;

	m_negativeRenderer  = g_prefsManager->GetInt("RenderNegative", 0) ? true : false;
    if( m_negativeRenderer )    m_backgroundColour.Set(255,255,255,255);
    else                        m_backgroundColour.Set(0,0,0,0);
    	
	UpdateDifficultyFromPreferences();    
	
    //int textureId = m_resource->GetTexture("textures/editor_font_normal.bmp");
    
    m_gameCursor        = new GameCursor();
    m_markerSystem      = new MarkerSystem();
    m_multiwiniaHelp    = new MultiwiniaHelp();
    m_soundSystem       = new SoundSystem();
    m_clientToServer    = new ClientToServer();

	AppDebugOut("Inits 1: %f\n", GetHighResTime() - start ); start = GetHighResTime();

	InitMetaServer();
	AppDebugOut("Inits 2: %f\n", GetHighResTime() - start ); start = GetHighResTime();

    m_clientToServer->OpenConnections();

    m_userInput         = new UserInput();
	m_camera            = new Camera();

    strcpy( m_gameDataFile, "game.txt");

#ifdef SOUND_EDITOR
	m_effectProcessor   = new EffectProcessor();
#endif // SOUND_EDITOR

	AppDebugOut("Inits 3: %f\n", GetHighResTime() - start ); start = GetHighResTime();

    SetProfileName( g_prefsManager->GetString("UserProfile", "none") );

#ifdef TARGET_OS_VISTA
    if( strlen( g_saveFile ) > 0 )
    {
        SetProfileName( g_saveFile );
    }
#endif

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    m_helpSystem        = new HelpSystem();
#endif
	m_particleSystem	= new ParticleSystem();
    m_gesture           = new Gesture("gestures.txt");

    m_script            = new Script();
	m_shamanInterface   = new ShamanInterface();
#ifdef ATTRACTMODE_ENABLED
	m_attractMode		= new AttractMode();
#endif
    m_controlHelpSystem = new ControlHelpSystem();

	AppDebugOut("Inits 4: %f\n", GetHighResTime() - start ); start = GetHighResTime();

#if defined(DEMOBUILD) && defined(USE_SEPULVEDA_HELP_TUTORIAL)
    #ifdef DEMO2
        m_tutorial = new Demo2Tutorial();
    #else
        m_tutorial = new Demo1Tutorial();
    #endif
#endif

    int menuOption = g_prefsManager->GetInt( OTHER_LARGEMENUS, 0 );
	if( menuOption == 2 ) // (todo) or is running in media center and tenFootMode == -1
	{
		m_largeMenus = true;
	}
#ifdef TARGET_OS_VISTA
    else if( menuOption == 0 &&
             g_mediaCenter == true )
    {
        m_largeMenus = true;
    }
#endif

	AppDebugOut("Inits 5: %f\n", GetHighResTime() - start ); start = GetHighResTime();

    m_achievementTracker = new AchievementTracker();
    m_multiwinia = new Multiwinia();
    m_gameMenu   = new GameMenu();
    
	AppDebugOut("Inits 6: %f\n", GetHighResTime() - start ); start = GetHighResTime();

#ifdef BENCHMARK_AND_FTP
    m_benchMark  = new BenchMark();
    m_benchMark->RequestDXDiag();
#endif

	AppDebugOut("Inits 7: %f\n", GetHighResTime() - start ); start = GetHighResTime();

	//
    // Load mods

#ifdef USE_DARWINIA_MOD_SYSTEM
    const char *modName = g_prefsManager->GetString("Mod", "none" );
    if( stricmp( modName, "none" ) != 0 )
    {
        g_app->m_resource->LoadMod( modName );
    }
#endif

	AppDebugOut("Inits 8: %f\n", GetHighResTime() - start ); start = GetHighResTime();

    //
    // Load save games
    
    // bool profileLoaded = LoadProfile();
	m_globalWorld = new GlobalWorld;	

	AppDebugOut("Inits 10: %f\n", GetHighResTime() - start ); start = GetHighResTime();

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
	m_sepulveda         = new Sepulveda();
#endif

    TaskManagerInterface::CreateTaskManager();

	AppDebugOut("Inits 11: %f\n", GetHighResTime() - start ); start = GetHighResTime();
	
	GameMenuWindow::PreloadTextures();

	AppDebugOut("Inits 11b: %f\n", GetHighResTime() - start ); start = GetHighResTime();
}

void App::InitMetaServer()
{
	char key[256], path[512];

	strcpy( path, GetProfileDirectory() );
	char fullFileName[512];
	sprintf( fullFileName, "%sauthkey.dev", path );

	Authentication_LoadKey( key, fullFileName );
	Authentication_SetKey( key );

	Authentication_RequestStatus( key, METASERVER_GAMETYPE_MULTIWINIA );
	const char *metaServerLocation = "metaserver-mwdev.introversion.co.uk";

	MetaServer_Initialise();
    MetaServer_Connect( metaServerLocation, PORT_METASERVER_CLIENT_LISTEN );
	MatchMaker_LocateService( metaServerLocation, PORT_METASERVER_LISTEN );
}


void App::DeleteOldLangTable()
{
	if( m_oldLangTable != NULL )
	{
		delete m_oldLangTable;
		m_oldLangTable = NULL;
	}
}

void App::LoadSounds()
{
	m_resource->ParseArchive( "sounds.dat", NULL );
	m_resource->ParseArchive( "sounds2.dat", NULL );

	m_soundsLoaded = true;

	if( m_soundSystem )
	{
#ifdef TARGET_OS_MACOSX
		m_soundSystem->LoadEffects();
		m_soundSystem->LoadBlueprints();
		m_soundSystem->PreloadSounds();
#endif
		m_soundSystem->TriggerInitialize();
	}          
}

void App::CheckSounds()
{
	if( !m_soundSystem->IsInitialized() )
	{
		m_soundSystem->Initialise();

		/*if( g_app->m_atMainMenu )
		{
			g_app->m_soundSystem->TriggerOtherEvent( NULL, "LobbyAmbience", SoundSourceBlueprint::TypeMultiwiniaInterface );
		}*/
	}

	if( m_soundSystem->IsInitialized() )
		g_cachedSampleManager.CleanUp();
}


bool App::IsGameModePermitted ( int gameMode )
{
	if( !IsFullVersion() )
	{
		DemoLimitations d;
		m_clientToServer->GetDemoLimitations( d );
		if( !d.GameModePermitted( gameMode ) )
			return false;
	}

    switch( gameMode )
    {
        #ifdef INCLUDEGAMEMODE_DOMINATION
                case Multiwinia::GameTypeSkirmish:              return true;
        #endif

        #ifdef INCLUDEGAMEMODE_KOTH
                case Multiwinia::GameTypeKingOfTheHill:         return true;
        #endif

        #ifdef INCLUDEGAMEMODE_CTS
                case Multiwinia::GameTypeCaptureTheStatue:      return true;
        #endif

        #ifdef INCLUDEGAMEMODE_ROCKET
                case Multiwinia::GameTypeRocketRiot:            return true;
        #endif

        #ifdef INCLUDEGAMEMODE_ASSAULT
                case Multiwinia::GameTypeAssault:               return true;
        #endif

        #ifdef INCLUDEGAMEMODE_BLITZ
                case Multiwinia::GameTypeBlitzkreig:            return true;
        #endif

		#ifdef INCLUDEGAMEMODE_TANKBATTLE
				case Multiwinia::GameTypeTankBattle:			return true;					
		#endif
    }

    return false;
}


App::~App()
{
#ifdef BENCHMARK_AND_FTP
	SAFE_DELETE(m_benchMark);
#endif
	SAFE_DELETE(m_gameMenu);
	SAFE_DELETE(m_multiwinia);
	SAFE_DELETE(m_globalWorld);
	SAFE_DELETE(m_langTable);
#if defined(DEMOBUILD) && defined(USE_SEPULVEDA_HELP_TUTORIAL)
	SAFE_DELETE(m_tutorial);
#endif
	SAFE_DELETE(m_taskManagerInterface);
	SAFE_DELETE(m_shamanInterface);
	SAFE_DELETE(m_controlHelpSystem);
#ifdef ATTRACTMODE_ENABLED
	SAFE_DELETE(m_attractMode);
#endif
	SAFE_DELETE(m_script);
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
	SAFE_DELETE(m_sepulveda);
#endif
	SAFE_DELETE(m_gesture);
	SAFE_DELETE(m_particleSystem);
	SAFE_DELETE(m_markerSystem);
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
	SAFE_DELETE(m_helpSystem);
#endif
	SAFE_DELETE(m_multiwiniaHelp);
#ifdef SOUND_EDITOR
	SAFE_DELETE(m_effectProcessor);
#endif // SOUND_EDITOR
	SAFE_DELETE(m_camera);
	SAFE_DELETE(m_userInput);
	SAFE_DELETE(m_clientToServer);
	SAFE_DELETE(m_soundSystem);
	SAFE_DELETE(m_gameCursor);
	SAFE_DELETE(m_renderer);
	SAFE_DELETE(g_prefsManager);
	SAFE_DELETE(m_soundsWorkQueue);
	SAFE_DELETE(m_resource);
	SAFE_DELETE(m_netLib);
	SAFE_DELETE(m_achievementTracker);
}

void App::UpdateDifficultyFromPreferences()
{
	// This method is called to make sure that the difficulty setting
	// used to control the game play (g_app->m_difficultyLevel) is 
	// consistent with the user preferences. 
	
	// Preferences value is 1-based, m_difficultyLevel is 0-based.
	m_difficultyLevel = 0;//g_prefsManager->GetInt(OTHER_DIFFICULTY, 1) - 1;
	if( m_difficultyLevel < 0 )	
	{
		m_difficultyLevel = 0;
	}
}


void App::SetLanguage( const char *_language, bool _test )
{
	AppDebugOut("Setting language to %s (Test = %d)\n", _language, _test);

    //
    // Load the language text file

    char langFilename[256];
#if defined(TARGET_OS_LINUX) && defined(TARGET_DEMOGAME)
	sprintf( langFilename, "language/%s_demo.txt", _language );
#else
    sprintf( langFilename, "language/%s.txt", _language );
#endif

	LangTable* newLangTable = new LangTable(langFilename);
    
    if( _test )
    {
        newLangTable->TestAgainstEnglish();
    }

	// Set the locale so that Uppercasing works correctly
	setlocale(  LC_CTYPE, _language );

    //
    // Load the MOD language file if it exists

    sprintf( langFilename, "strings_%s.txt", _language );
    TextReader *modLangFile = g_app->m_resource->GetTextReader(langFilename);
    if( !modLangFile )
    {
        sprintf( langFilename, "strings_default.txt" );
        modLangFile = g_app->m_resource->GetTextReader(langFilename);
    }

    if( modLangFile )
    {
        delete modLangFile;
        newLangTable->ParseLanguageFile( langFilename );
    }
    //
    // Load fonts if they exist
#ifdef TARGET_MSVC
	void PrintAvailableMemory();
	PrintAvailableMemory();
#endif
	
	AppDebugOut( "Loading fonts\n");

	static bool initedFonts = false;
	if( !initedFonts )
	{
		g_gameFont.Initialise( "speccy" );
		g_editorFont.Initialise( "editor" );
		g_titleFont.Initialise( "square" );
        
        g_editorFont.SetHorizSpacingFactor( 0.91f );
        g_titleFont.SetHorizSpacingFactor( 1.03f );

		initedFonts = true;
	}

	if ( strcmp(_language, "chinese-trad") == 0 ||
		 strcmp(_language, "chinese-simp") == 0 ||
		 strcmp(_language, "korean") == 0 ||
		 strcmp(_language, "japanese") == 0 )
	{
		g_gameFont.SetHorizScaleFactor(1.0f);
		g_editorFont.SetHorizScaleFactor(1.0f);
		g_titleFont.SetHorizScaleFactor(1.0f);

		g_oldEditorFont = g_editorFont;
		g_oldGameFont = g_gameFont;

		g_editorFont = g_titleFont;
		g_gameFont = g_titleFont;
		m_usingFontCopies = true;
	}
	else
	{
		if( m_usingFontCopies )
		{
			g_editorFont = g_oldEditorFont;
		}
		if( m_usingFontCopies )
		{
			g_gameFont = g_oldGameFont;
		}

		g_editorFont.SetHorizScaleFactor(0.6f);
		g_gameFont.SetHorizScaleFactor(0.6f);
		g_titleFont.SetHorizScaleFactor(0.6f);

		m_usingFontCopies = false;
	}

	AppDebugOut("Fonts loaded.\n");
#ifdef TARGET_OS_MSVC
	PrintAvailableMemory();
#endif

	if ( g_inputManager )
	{
		newLangTable->RebuildTables();
	}

	// Delete the old table, if it exists (which it doesn't when Multiwinia is first run)
	if( m_langTable != NULL )
	{
		m_oldLangTable = m_langTable;
		Method<App> *m = new Method<App>( &App::DeleteOldLangTable, this );
		DelayedJob *dJob = new DelayedJob(m,5);
		AddDelayedJob(dJob);
	}

	m_langTable = newLangTable;
}

void App::HandleDelayedJobs()
{
	if( m_delayedJobListMutex.TryLock() )
	{
		DelayedJob* dJob = m_delayedJobs.GetData(0);

		if( dJob &&	dJob->ReadyToRun() )
		{
			dJob->Run();
			m_delayedJobs.RemoveData(0);
			delete dJob;
		}
		m_delayedJobListMutex.Unlock();
	}
}

void App::AddDelayedJob(DelayedJob* _dJob)
{
	m_delayedJobListMutex.Lock();
	m_delayedJobs.PutDataAtEnd(_dJob);
	m_delayedJobListMutex.Unlock();
}

void App::SetProfileName( const char *_profileName )
{
    strcpy( m_userProfileName, _profileName );

	AppDebugOut("Setting ProfileName to %s\n", _profileName);

	if( stricmp( _profileName, "AttractMode" ) != 0 )
	{
		g_prefsManager->SetString( "UserProfile", m_userProfileName );
		g_prefsManager->Save();
	}
}


#if defined(TARGET_OS_LINUX) || defined(TARGET_OS_MACOSX)
#include <sys/types.h>
#include <sys/stat.h>
#endif 

void App::UseProfileDirectory( bool _profileDirectory )
{
	s_profileDirectory = _profileDirectory;
}

const char *App::GetProfileDirectory()
{
#if defined(TARGET_OS_LINUX)
	
	static char userdir[256];
	const char *home = getenv("HOME");
	if (s_profileDirectory && home != NULL) {
		sprintf(userdir, "%s/.%s", home, (APP_NAME));
		mkdir(userdir, 0777);

		sprintf(userdir, "%s/.%s/%s/", home, (APP_NAME), DARWINIA_GAMETYPE);
		mkdir(userdir, 0777);
		return userdir;
	}
	else // Current directory if no home
		return "";
		
#elif defined(TARGET_OS_MACOSX) 
	
	static char userdir[256];
	const char *home = getenv("HOME");
	if (s_profileDirectory && home != NULL) {
		sprintf(userdir, "%s/Library", home);
		mkdir(userdir, 0777);
		
		sprintf(userdir, "%s/Library/Application Support", home);
		mkdir(userdir, 0777);

		sprintf(userdir, "%s/Library/Application Support/" APP_NAME, home);
		mkdir(userdir, 0777);

		sprintf(userdir, "%s/Library/Application Support/" APP_NAME "/%s/", 
				home, DARWINIA_GAMETYPE);
		mkdir(userdir, 0777);

		return userdir;
	}
	else // Current directory if no home
		return "";

#else
#if !defined(TARGET_DEBUG) 
    if( s_profileDirectory && IsRunningAtLeast2k() )
    {
        static char userdir[512];    
        SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, userdir );

		const char *subdir = "\\Introversion\\";

        strncat(userdir, subdir, sizeof(userdir) );
        CreateDirectory( userdir );

		strncat(userdir, "Multiwinia\\", sizeof(userdir) );
		CreateDirectory( userdir );

        return userdir;
    }
    else
#endif
    {
        return "";
    }
#endif
}

const char *App::GetPreferencesPath()
{
	static char *path = NULL;
	if (path == NULL)
	{
#if defined(TARGET_OS_MACOSX)
		const char *home = getenv("HOME");
		if (home != NULL)
		{
			path = new char[strlen(home) + 256];
			sprintf(path, "%s/Library/Preferences/uk.co.introversion.%s.txt", home, APP_NAME);
		}
#else
		const char *profileDir = GetProfileDirectory();
		path = new char[strlen(profileDir) + 32];
		sprintf( path, "%spreferences.txt", profileDir );
#endif
	}

	return path;
}

const char *App::GetScreenshotDirectory()
{
#if defined(TARGET_OS_VISTA)
    static char dir[MAX_PATH];
    SHGetFolderPath( NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, dir );
    sprintf( dir, "%s\\", dir );
    return dir;
#elif defined(TARGET_OS_MACOSX)
	FSRef ref;
	static char dir[1024] = "";
	
	if (strlen(dir) == 0)
	{
		if (FSFindFolder(kUserDomain, kPictureDocumentsFolderType, kCreateFolder, &ref) != noErr)
			return NULL;
		if (FSRefMakePath(&ref, (UInt8 *)dir, sizeof(dir)-32) != noErr)
			return NULL;
		strcat(dir, "/Multiwinia/");
		// note that it's okay for the directory to not exist yet
	}
		
	return dir;
#else
    static char *path = NULL;

    if (path == NULL) {
        const char *profileDir = GetProfileDirectory();
        path = new char[strlen(profileDir) + 32];
        sprintf( path, "%sscreenshots/", profileDir );
    }

    return path;
#endif
}

const char *App::GetMapDirectory()
{
    static char *directory = NULL;
    if( !directory )
    {
        const char *profileDir = GetProfileDirectory();
		directory = new char[strlen(profileDir) + 32];
		sprintf( directory, "%smaps/", profileDir );
    }

    return directory;
}

bool App::LoadProfile()
{    
	bool newProfile = m_globalWorld->m_loadingNewProfile;
    if( IsFullVersion() &&
		strncmp( m_gameDataFile, "game_demo", 9 ) != 0 &&
		stricmp( m_userProfileName, "AccessAllAreas" ) == 0 ||
		stricmp( m_userProfileName, "AttractMode" ) == 0 )
    {
        // Cheat username that opens all locations
        // aimed at beta testers who've completed the game already

        if( m_globalWorld )
        {
            delete m_globalWorld;
            m_globalWorld = NULL;
        }
    
        m_globalWorld = new GlobalWorld();
		m_globalWorld->m_loadingNewProfile = newProfile;
        m_globalWorld->LoadGame( "game_unlockall.txt" );
        for( int i = 0; i < m_globalWorld->m_buildings.Size(); ++i )
        {
            GlobalBuilding *building = m_globalWorld->m_buildings[i];
            if( building && building->m_type == Building::TypeTrunkPort )
            {
                building->m_online = true;
            }
        }
        for( int i = 0; i < m_globalWorld->m_locations.Size(); ++i )
        {
            GlobalLocation *loc = m_globalWorld->m_locations[i];
            loc->m_available = true;
        }        
    }
    else
    {
        if( m_globalWorld )
        {
            delete m_globalWorld;
            m_globalWorld = NULL;
        }
    
        m_globalWorld = new GlobalWorld();
		m_globalWorld->m_loadingNewProfile = newProfile;
		AppDebugOut("We are %sloading a new profile\n", newProfile ? "" : "not ");
        m_globalWorld->LoadGame( m_gameDataFile );    
    }

    return true;
}

bool App::SaveProfile( bool _global, bool _local )
{
    if( !IsFullVersion()  ||
		!g_app->IsSinglePlayer() ||
		stricmp( m_userProfileName, "none" ) == 0 || 
		stricmp( m_userProfileName, "AccessAllAreas" ) == 0 ||
		stricmp( m_userProfileName, "AttractMode" ) == 0 /*||
		strnicmp( m_gameDataFile, "game_demo", 9 ) == 0 */) 
	{
		AppDebugOut("Returning false from saving profile\n");
		return false;
	}
    
	bool canWrite = true;

    char folderName[512];
    sprintf( folderName, "%susers/", GetProfileDirectory() );
    bool success = CreateDirectory( folderName );        
    if( !success ) 
    {
        AppDebugOut( "failed to create folder %s\n", folderName );
        return false;
    }

    sprintf( folderName, "%susers/%s", GetProfileDirectory(), m_userProfileName );
    success = CreateDirectory( folderName );
    if( !success ) 
    {
        AppDebugOut( "failed to create folder %s\n", folderName );
        return false;
    }

#ifdef TARGET_OS_VISTA
	if( _global )
	{
		SaveRichHeader();
	}
#endif

    if( canWrite && _global )
    {
        m_globalWorld->SaveGame( m_gameDataFile );
    }

	bool returnVal = true;

    if( canWrite && _local && g_app->m_location )
    {
        if( m_levelReset )
        {
            m_levelReset = false;
            returnVal =  false;
        }
		else
		{
			g_app->m_location->m_levelFile->GenerateInstantUnits();
			g_app->m_location->m_levelFile->GenerateDynamicBuildings();
			char *missionFilename = m_location->m_levelFile->m_missionFilename;
			m_location->m_levelFile->SaveMissionFile( missionFilename );
		}
    }

	return returnVal;
}

void App::StartNetwork( bool _iAmAServer, const char *_serverIp, int _serverPort )
{

    if (!g_app->m_editing)
    {
		char serverIp[16];

		if( _iAmAServer )
        {
			delete m_server;
			m_server = new Server();
			m_server->Initialise();
            m_server->m_noAdvertise = true;

			if( _serverPort != -1 )
			{
				GetLocalHostIP( serverIp, sizeof(serverIp) );
				_serverIp = serverIp;
				_serverPort = g_app->m_server->m_listener->GetPort();
			}
			else
			{
				_serverIp = "127.0.0.1";
			}
		}

	    m_clientToServer->ClientJoin( _serverIp, _serverPort );
    }
}

bool App::StartSinglePlayerServer()
{
	AppDebugOut("Starting single player server.\n");
	if( !m_multiwiniaTutorial ) m_multiwinia->m_aiType = Multiwinia::AITypeStandard;
	g_gameTimer.Reset();
	NetLockMutex lock( m_networkMutex );
	g_app->StartNetwork( true, NULL, -1 );

	return true;
}

HRESULT App::StartMultiPlayerServer()
{
	AppDebugOut("Starting multi-player server.\n");
	if( !m_multiwiniaTutorial ) m_multiwinia->m_aiType = Multiwinia::AITypeStandard;
	g_app->StartNetwork( true, NULL, NULL ); 
	return 0; // = S_OK = success
}

void App::ShutdownCurrentGame()
{
	SaveProfile( false, true );
    
    if( g_prefsManager->GetInt( "RecordDemo" ) == 1 )
    {
        if( m_server ) m_server->SaveHistory( "serverHistory.dat" );
        //g_inputManager->StopLogging( "inputLog.dat" );
    }
        
	g_explosionManager.Reset();

	if (m_location)
	{
		m_globalWorld->TransferSpirits( m_locationId );
	}

	m_clientToServer->ClientLeave();

	if (m_location)
	{
		m_location->Empty();
	}

	m_particleSystem->Empty();
	m_markerSystem->ClearAllMarkers();
	m_multiwiniaHelp->Reset();


//	m_inLocation = false;
//	m_requestedLocationId = false;

	delete m_location;

	m_location = NULL;
	m_locationId = -1;

	delete m_locationInput;
	m_locationInput = NULL;

	delete m_server;
    m_server = NULL;

	m_multiwinia->Reset();
    
	m_globalWorld->m_myTeamId = 255;
	m_globalWorld->EvaluateEvents();

	m_userRequestsPause = false;

    SaveProfile( true, false );
}

bool App::ToggleGamePaused()
{
	bool allowPause = true;
	for( int i = 0; i < NUM_TEAMS; ++i )
	{
		if( m_multiwinia->m_teams[i].m_teamType == TeamTypeRemotePlayer )
		{
			allowPause = false;
			break;
		}
	}
	if( m_multiwinia->GameInGracePeriod() || m_multiwinia->GameOver() )
	{
		allowPause = false;
	}
	if( allowPause )
	{
		m_clientToServer->RequestPause();
	}
	
	return allowPause;
}

bool App::GamePaused() const
{
	return 
		m_location && (
            g_gameTimer.IsPaused() ||
			/*m_userRequestsPause || */
			m_clientToServer->m_outOfSyncClients.Size() > 0 ||
			(m_lostFocusPause && g_app->IsSinglePlayer()));
}


void App::ResetLevel( bool _global )
{
    if( m_location )
    {
        m_requestedLocationId = -1;
	    m_requestedMission[0] = '\0';
	    m_requestedMap[0] = '\0';		

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
        if( g_app->m_tutorial ) g_app->m_tutorial->Restart();
#endif
        

        //
        // Delete the saved mission file

        char *missionFilename = m_location->m_levelFile->m_missionFilename;
        char saveFilename[256];
        sprintf( saveFilename, "%susers/%s/%s", GetProfileDirectory(), m_userProfileName, missionFilename );
    
        DeleteThisFile( saveFilename );

        m_levelReset = true;


        //
        // Delete the game file if required

        if( _global )
        {
            sprintf( saveFilename, "%susers/%s/%s", GetProfileDirectory(), m_userProfileName, m_gameDataFile );

            DeleteThisFile( saveFilename );

            if( m_globalWorld )
            {
                delete m_globalWorld;
                m_globalWorld = NULL;
            }
    
            m_globalWorld = new GlobalWorld();
            m_globalWorld->LoadGame( m_gameDataFile );     
        }
    }    
}

bool App::IsSinglePlayer()
{
	return (g_app->m_gameMode == App::GameModeCampaign || g_app->m_gameMode  == App::GameModePrologue);
}

bool App::Multiplayer()
{
	return g_app->m_gameMode == GameModeMultiwinia;
}


void App::CheckMasterAchievement()
{
}


bool App::EarnedAchievement( int _achievementId )
{
    return false;
}

void App::GiveAchievement(int _achievementId)
{
    // achievements should only be available in games without AI players
	if( _achievementId != DarwiniaAchievementMasterPlayer &&
		_achievementId != DarwiniaAchievementCarnage &&
		_achievementId != DarwiniaAchievementGenocide )
    {
		for( int i = 0; i < g_app->m_location->m_levelFile->m_numPlayers; ++i )
        {
            if( m_multiwinia->m_teams[i].m_teamType == TeamTypeCPU )
            {
                return;
            }
        }
    }

    if( EarnedAchievement( _achievementId ) ) return;
}

bool App::IsFullVersion()
{
#ifdef MULTIWINIA_DEMOONLY
    return false;
#endif

#ifdef GIVE_ALL_LEVELS
	return true;
#else
    // this needs to be updated once metaserver fullgame checks are implemented for the PC version
	// It's the full game if we're not a demo key

    char authKey[256];
    Authentication_GetKey( authKey );
    return !Authentication_IsDemoKey( authKey );
#endif
}

bool App::MultiplayerPermitted()
{
#ifdef MULTIPLAYER_DISABLED
	return false;
#else 
	if( IsFullVersion() )
		return true;

	DemoLimitations d;
	m_clientToServer->GetDemoLimitations( d );
	return d.m_maxDemoPlayers > 0;
#endif
}

bool App::HostGamePermitted()
{
#ifdef MULTIPLAYER_DISABLED
	return false;
#else 
	if( IsFullVersion() )
		return true;

	DemoLimitations d;
	m_clientToServer->GetDemoLimitations( d );
	return d.m_allowDemoServers > 0;
#endif

}

bool App::IsMapAvailableInDemo( int gameMode, int mapId )
{       
	DemoLimitations d;
	m_clientToServer->GetDemoLimitations( d );

	return d.GameModePermitted( gameMode) && d.MapPermitted( mapId );
}


bool App::IsMapPermitted( int gameMode, int mapId )
{
    if( !IsGameModePermitted(gameMode) )
    {
        return false;
    }

    if( !IsMapPermitted( mapId ) )
    {
        return false;
    }

    return true;
}


bool App::IsMapPermitted( int mapId )
{

#ifdef PERMITTED_MAPS
    int permittedMaps[] = PERMITTED_MAPS;

    for( int i = 0; permittedMaps[i]; i++ )
    {
        if( mapId == permittedMaps[i] )
        {
            return true;
        }
    }

    return false;
#else

	if( !IsFullVersion() )
	{
		DemoLimitations d;
		m_clientToServer->GetDemoLimitations( d );
		return d.MapPermitted( mapId );
	}

    return true;
#endif

}


int App::GetMapID( int gameMode, int mapCrcId )
{
	if( gameMode < 0 || gameMode >= MAX_GAME_TYPES )
		return -1;

	DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[ gameMode ];
	
	for( int i = 0; i < maps.Size(); i++ )
	{
		if( maps.ValidIndex( i ) && maps[ i ]->m_mapId == mapCrcId )
		{
			return i;
		}
	}

	return -1;
}

bool App::UseChristmasMode()
{
#ifdef DEMOBUILD
    return false;
#endif
    
    if( g_app->m_editing ) return false;
    
    // Last 2 weeks in December only
    // Also allow user to disable if he wishes

	time_t now = time(NULL);
    tm *theTime = localtime(&now);
    
#ifdef CHRISTMAS_DEMO
    if( theTime->tm_mon == 10 && theTime->tm_mday >= 27 ) return true;
    if( theTime->tm_mon == 11 ) return true;
#else
    if( theTime->tm_mon == 11 )
    {
        if(theTime->tm_mday == 24 || theTime->tm_mday == 25 || theTime->tm_mday == 26) 
        {
            return true;
        }
    }
#endif

    return false;
}


#ifdef TARGET_OS_VISTA
void App::SaveRichHeader()
{
	if( g_app->m_editing ) return;
	if( !m_thumbnailScreenshot ) return;

	char _filename[256];
	sprintf( _filename, "%s.dsg", m_userProfileName );

	char fullFilename[256];
    sprintf( fullFilename, "%susers/%s", g_app->GetProfileDirectory(), _filename );        

	RICH_GAME_MEDIA_HEADER header;
	ZeroMemory( &header, sizeof(RICH_GAME_MEDIA_HEADER) );
	header.dwMagicNumber = RM_MAGICNUMBER;
	header.dwHeaderVersion = 1;
	header.dwHeaderSize = sizeof(RICH_GAME_MEDIA_HEADER);
	header.liThumbnailOffset.QuadPart = 0;
	if( m_thumbnailScreenshot )
	{
		unsigned int size = (m_thumbnailScreenshot->m_height * m_thumbnailScreenshot->m_width * 3 ) + 54;
		header.dwThumbnailSize = size;
	}
	else
	{
		header.dwThumbnailSize = 0;
	}

	header.guidGameId.Data1 = 0xF58175C7;
	header.guidGameId.Data2 = 0xE99C;
	header.guidGameId.Data3 = 0x4151;

	header.guidGameId.Data4[0] = 0x80;
	header.guidGameId.Data4[1] = 0x0D;
	header.guidGameId.Data4[2] = 0x7B;
	header.guidGameId.Data4[3] = 0xB5;
	header.guidGameId.Data4[4] = 0xE8;
	header.guidGameId.Data4[5] = 0x9E;
	header.guidGameId.Data4[6] = 0x49;
	header.guidGameId.Data4[7] = 0x60;

	WCHAR saveName[256];
	char filePath[256];
	sprintf( filePath, "%susers\\%s.dsg", GetProfileDirectory(), m_userProfileName );

	mbstowcs( saveName, filePath, 256 );

	wcscpy(header.szGameName,  L"Darwinia");
	wcscpy(header.szSaveName, saveName );
	wcscpy(header.szLevelName, L"");
	wcscpy(header.szComments, L"");

	FILE *file = fopen(fullFilename, "wb");
	fwrite( &header, sizeof(RICH_GAME_MEDIA_HEADER), 1, file );

	m_thumbnailScreenshot->WritePng( file );
	// m_thumbnailScreenshot->SavePng( "ss.bmp" );
	delete m_thumbnailScreenshot;
	m_thumbnailScreenshot = NULL;

	fclose(file);

}

void App::SaveThumbnailScreenshot()
{
	PosterMaker pm(g_app->m_renderer->ScreenW(), g_app->m_renderer->ScreenH());
	pm.AddFrame();

	float newWidth = 256.0f;
	float newHeight = 192.0f;

	BitmapRGBA scaled(newWidth, newHeight );

	scaled.Blit(0, 0, pm.GetBitmap()->m_width, pm.GetBitmap()->m_height, pm.GetBitmap(), 
							   0, 0, newWidth, newHeight, true);

	m_thumbnailScreenshot = new BitmapRGBA( scaled );

	m_saveThumbnail = false;
}

#endif

int App::GetMaxNumberofPlayers()
{

	   int gameType = m_multiwinia->m_gameType;

       MapData *mapData = NULL;

		// Determine how many players can join this game
		if( gameType != -1 && m_requestedMap )
		{
			DArray<MapData *> &maps = m_gameMenu->m_maps[gameType];			

			for( int i = 0; i < maps.Size(); i++)
			{
				if( maps.ValidIndex( i ) )
				{
					MapData *m = maps[i];
					if( stricmp( g_app->m_requestedMap, m->m_fileName ) == 0 )
					{
                        mapData = m;
						return m->m_numPlayers;
					}
				}
			}
		}	

		return 0;
}

const char *App::GetBuyNowURL() const
{
	return "http://store.introversion.co.uk";
}

#ifdef TESTBED_ENABLED
//
// Testbed functions
//

TESTBED_MODE App::GetTestBedMode() 
{
	return m_eTestBedMode; 
}

void App::SetTestBedMode(TESTBED_MODE eMode) 
{
	m_eTestBedMode = eMode; 
}

const UnicodeString &App::GetTestBedServerName()
{
	return m_testbedServerName;
}

TESTBED_TYPE App::GetTestBedType() 
{ 
	return m_eTestBedType; 
}

void App::SetTestBedType(TESTBED_TYPE eType) 
{ 
	m_eTestBedType = eType; 
}

TESTBED_STATE App::GetTestBedState() 
{ 
	return m_eTestBedState; 
}

void App::SetTestBedState(TESTBED_STATE eState) 
{ 
	m_eTestBedState = eState; 
}

int App::GetTestBedGameMode() 
{ 
	return m_iGameMode; 
}

void App::SetTestBedGameMode(int iGameMode) 
{ 
	m_iGameMode = iGameMode; 
}

int App::GetTestBedGameMap() 
{ 
	return m_iGameMap; 
}

void App::SetTestBedGameMap(int iGameMap) 
{ 
	m_iGameMap = iGameMap; 
}

int App::GetTestBedGameCount() 
{ 
	return m_iGameCount; 
}

void App::IncrementGameCount() 
{ 
	m_iGameCount++; 
}

void App::ResetGameCount() 
{ 
	m_iGameCount = 0; 
}

int App::GetTestBedNumSyncErrors() 
{ 
	return m_iNumSyncErrors; 
}

void App::IncremementNumSyncErrors() 
{ 
	m_iNumSyncErrors++; 
}

void App::ResetNumSyncErrors() 
{ 
	m_iNumSyncErrors = 0; 
}

void App::SetTestBedClientRandomSeed(int iRandomSeed) 
{ 
	m_iClientRandomSeed = iRandomSeed; 
}

int App::GetTestBedClientRandomSeed() 
{ 
	return m_iClientRandomSeed; 
}

void App::SetSequenceIDDelay(int iSequenceID, TESTBED_STATE eNextState) 
{
	m_iDesiredSequenceID = iSequenceID; 
	m_eNextState = eNextState;
	SetTestBedState(TESTBED_SEQUENCEID_DELAY);
}

void App::ToggleTestBedRendering()
{
	m_bRenderOn = !m_bRenderOn;
}

void App::SetTestBedRenderOn()
{
	m_bRenderOn = true;
}

void App::SetTestBedRenderOff()
{
	m_bRenderOn = false;
}

bool App::GetTestBedRendering()
{
	return m_bRenderOn;
}

static 
std::string Chop( const std::string &_src )
{
	return _src.substr( 0, _src.length() - 1 );
}

static 
std::string StripPrefix( const std::string &_src )
{
	return _src.substr( 2 );
}

void App::TestBedLogWrite()
{
	char logFile[1500];
	
	// On OS X, we don't want to use the current directory, since that's inside the app
	snprintf(logFile, sizeof(logFile), "%sTestBedLog.txt",
#ifdef TARGET_OS_MACOSX
			GetProfileDirectory());
#else
			"");
#endif
	bool writeHeaders = !DoesFileExist( logFile );

	std::ofstream out( logFile, std::ios::app );
	if( !out )
		return;

	// Write out the log
	time_t now = time( NULL );				

	std::map< std::string, std::string > fields;
	fields["0 Date"] = Chop( asctime( localtime(&now) ) );
	fields["1 NumSyncErrors"] = ToString( m_iNumSyncErrors );
	fields["2 GameMode"] = m_cGameMode;
	fields["3 GameMap"] = m_cGameMap;
	fields["4 GameIndex" ] = ToString( m_iGameIndex );
	fields["5 ClientRandomSeed" ] = ToString( m_iClientRandomSeed );
	fields["6 LastGameIndex" ] = ToString( m_iLastGameIndex );
	fields["7 GameTimer" ] = ToString( g_gameTimer.GetGameTime() );
	fields["8 SyncSeqId" ] = ToString( m_clientToServer->m_syncErrorSeqId );

	if( writeHeaders )
	{
		std::vector< std::string > keys = Keys( fields );
		std::transform( keys.begin(), keys.end(), keys.begin(), StripPrefix );
		out << Join( keys, ", " ) << "\n";
	}

	out << Join( Values( fields ), ", " ) << "\n";
	out.close();
}

int App::GetTestBedGameIndex()
{
	return m_iGameIndex;
}

void App::SetTestBedGameIndex(int iGameIndex)
{
	m_iLastGameIndex = m_iGameIndex;
	m_iGameIndex = iGameIndex;
}


#endif
