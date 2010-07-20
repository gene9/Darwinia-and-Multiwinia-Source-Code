#include "lib/universal_include.h"

#include <stdlib.h>
#include <stdio.h>
#include <shlobj.h>

#ifdef TARGET_OS_VISTA
#include <knownfolders.h>
#endif

#include "network/clienttoserver.h"

#ifdef TARGET_OS_VISTA
#include "lib/poster_maker.h"
#endif
#include "lib/avi_generator.h"
#include "lib/hi_res_time.h"
#include "lib/language_table.h"
#include "lib/mouse_cursor.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/system_info.h"
#include "lib/text_renderer.h"
#include "lib/filesys_utils.h"
#include "lib/bitmap.h"
#include "lib/file_writer.h"
#include "interface/prefs_other_window.h"

#include "sound/sound_stream_decoder.h"
#include "sound/soundsystem.h"

#include "app.h"
#include "camera.h"
#include "effect_processor.h"
#include "gesture.h"
#include "global_world.h"
#include "helpsystem.h"
#include "tutorial.h"
#include "keygen.h"
#include "location.h"
#include "location_input.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "script.h"
#include "sepulveda.h"
#include "user_input.h"
#include "taskmanager.h"
#include "taskmanager_interface_gestures.h"
#include "taskmanager_interface_icons.h"
#include "gamecursor.h"
#include "level_file.h"
#include "demoendsequence.h"
#include "attract.h"
#include "control_help.h"
#include "game_menu.h"

void SetPreferenceOverrides(); // See main.cpp

App *g_app = NULL;


#ifdef DEMO2
    #define GAMEDATAFILE "game_demo2.txt"
#else
    #ifdef DEMOBUILD
        #define GAMEDATAFILE "game_demo.txt"
    #else
        #define GAMEDATAFILE "game.txt"
    #endif
#endif


App::App()
:   m_camera(NULL),
    m_location(NULL),
	m_locationId(-1),
    m_server(NULL),
    m_clientToServer(NULL),
    m_renderer(NULL),
    m_userInput(NULL),
    m_profiler(NULL),
    m_resource(NULL),
    m_soundSystem(NULL),
	m_locationInput(NULL),
	m_locationEditor(NULL),
    m_aviGenerator(NULL),
	m_effectProcessor(NULL),
    m_globalWorld(NULL),
    m_helpSystem(NULL),
    m_tutorial(NULL),
	m_particleSystem(NULL),
    m_taskManager(NULL),
    m_gesture(NULL),
    m_script(NULL),
    m_sepulveda(NULL),
    m_testHarness(NULL),
    m_paused(false),
	m_editing(false),
	m_requestedLocationId(-1),
	m_requestToggleEditing(false),
	m_requestQuit(false),
    m_negativeRenderer(false),
	m_difficultyLevel(0),
    m_levelReset(false),
    m_langTable(NULL),
    m_startSequence(NULL),
    m_demoEndSequence(NULL),
	m_largeMenus(false),
	m_attractMode(NULL),
    m_controlHelpSystem(NULL),
    m_atMainMenu(false),
    m_gameMenu(NULL),
    m_gameMode(GameModeNone)
#ifdef TARGET_OS_VISTA
	,m_thumbnailScreenshot(NULL),
	m_saveThumbnail(false)
#endif
{
    g_app = this;

	// Load resources

    m_resource = new Resource();
    m_resource->ParseArchive( "main.dat", NULL );
    m_resource->ParseArchive( "sounds.dat", NULL );
	m_resource->ParseArchive( "patch.dat", NULL );
    m_resource->ParseArchive( "language.dat", NULL );

	g_prefsManager = new PrefsManager(App::GetPreferencesPath());
	SetPreferenceOverrides();

    m_bypassNetworking  = g_prefsManager->GetInt("BypassNetwork") ? true : false;

    m_negativeRenderer  = g_prefsManager->GetInt("RenderNegative", 0) ? true : false;
    if( m_negativeRenderer )    m_backgroundColour.Set(255,255,255,255);
    else                        m_backgroundColour.Set(0,0,0,0);

	UpdateDifficultyFromPreferences();


#ifdef PROFILER_ENABLED
    m_profiler          = new Profiler();
#endif

    m_renderer          = new Renderer();
    m_renderer->Initialise();

	// Make sure that resources are now available - either the .dat files
	// or the data directory must exist

	SoundStreamDecoder *ssd = m_resource->GetSoundStreamDecoder("sounds/ablaster");
	DarwiniaReleaseAssert(ssd,
		"Couldn't find sound resources. This is probably because\n"
		"sounds.dat isn't in the working directory.");
	delete ssd;

    int textureId = m_resource->GetTexture("textures/editor_font_normal.bmp");

    m_gameCursor        = new GameCursor();
    m_soundSystem       = new SoundSystem();
    m_clientToServer    = new ClientToServer();
    m_userInput         = new UserInput();
//    m_location          = new Location();
//    m_locationInput		= new LocationInput();


	m_camera            = new Camera();
    m_gameMenu          = new GameMenu();

    strcpy( m_gameDataFile, "game.txt" );



    //
    // Determine default language if possible

    char *language = g_prefsManager->GetString("TextLanguage");
    if( stricmp(language, "unknown") == 0 )
    {
        char *defaultLang = g_systemInfo->m_localeInfo.m_language;
        char langFilename[512];
        sprintf( langFilename, "data/language/%s.txt", defaultLang );
        if( DoesFileExist(langFilename) )
        {
            g_prefsManager->SetString( "TextLanguage", defaultLang );
        }
        else
        {
            g_prefsManager->SetString( "TextLanguage", "english" );
        }
    }
    language = g_prefsManager->GetString("TextLanguage");

    SetLanguage( language, g_prefsManager->GetInt( "TextLanguageTest", 0 ) );



#ifdef SOUND_EDITOR
	m_effectProcessor   = new EffectProcessor();
#endif // SOUND_EDITOR

    SetProfileName( g_prefsManager->GetString("UserProfile", "none") );

#ifdef TARGET_OS_VISTA
    if( strlen( g_saveFile ) > 0 )
    {
        SetProfileName( g_saveFile );
    }
#endif

    m_helpSystem        = new HelpSystem();
	m_particleSystem	= new ParticleSystem();
    m_taskManager       = new TaskManager();
    m_gesture           = new Gesture("gestures.txt");
    m_sepulveda         = new Sepulveda();
    m_script            = new Script();
#ifdef ATTRACTMODE_ENABLED
	m_attractMode		= new AttractMode();
#endif
    m_controlHelpSystem = new ControlHelpSystem();

    if( g_prefsManager->GetInt( "ControlMethod" ) == 0 )
    {
        m_taskManagerInterface = new TaskManagerInterfaceGestures();
    }
    else
    {
        m_taskManagerInterface = new TaskManagerInterfaceIcons();
    }

	m_soundSystem->Initialise();

#ifdef DEMOBUILD
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

    //
    // Load mods

    char *modName = g_prefsManager->GetString("Mod", "none" );
    if( stricmp( modName, "none" ) != 0 )
    {
        g_app->m_resource->LoadMod( modName );
    }


    //
    // Load save games

    bool profileLoaded = LoadProfile();
}

App::~App()
{
	SAFE_DELETE(m_globalWorld);
	SAFE_DELETE(m_langTable);
#ifdef DEMOBUILD
	SAFE_DELETE(m_tutorial);
#endif
	SAFE_DELETE(m_taskManagerInterface);
	SAFE_DELETE(m_controlHelpSystem);
#ifdef ATTRACTMODE_ENABLED
	SAFE_DELETE(m_attractMode);
#endif
	SAFE_DELETE(m_script);
	SAFE_DELETE(m_sepulveda);
	SAFE_DELETE(m_gesture);
	SAFE_DELETE(m_taskManager);
	SAFE_DELETE(m_particleSystem);
	SAFE_DELETE(m_helpSystem);
#ifdef SOUND_EDITOR
	SAFE_DELETE(m_effectProcessor);
#endif // SOUND_EDITOR
	SAFE_DELETE(m_camera);
	SAFE_DELETE(m_userInput);
	SAFE_DELETE(m_clientToServer);
	SAFE_DELETE(m_soundSystem);
	SAFE_DELETE(m_gameCursor);
	SAFE_DELETE(m_renderer);
#ifdef PROFILER_ENABLED
	SAFE_DELETE(m_profiler);
#endif
	SAFE_DELETE(g_prefsManager);
	SAFE_DELETE(m_resource);
}

void App::UpdateDifficultyFromPreferences()
{
	// This method is called to make sure that the difficulty setting
	// used to control the game play (g_app->m_difficultyLevel) is
	// consistent with the user preferences.

	// Preferences value is 1-based, m_difficultyLevel is 0-based.
	m_difficultyLevel = g_prefsManager->GetInt(OTHER_DIFFICULTY, 1) - 1;
	if( m_difficultyLevel < 0 )
	{
		m_difficultyLevel = 0;
	}
}


void App::SetLanguage( char *_language, bool _test )
{
    //
    // Delete existing language data

    if( m_langTable )
    {
        delete m_langTable;
        m_langTable = NULL;
    }


    //
    // Load the language text file

    char langFilename[256];
#if defined(TARGET_OS_LINUX) && defined(TARGET_DEMOGAME)
	sprintf( langFilename, "language/%s_demo.txt", _language );
#else
    sprintf( langFilename, "language/%s.txt", _language );
#endif

	m_langTable = new LangTable(langFilename);

    if( _test )
    {
        m_langTable->TestAgainstEnglish();
    }

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
        m_langTable->ParseLanguageFile( langFilename );
    }

    //
    // Load localised fonts if they exist

    char fontFilename[256];
    sprintf( fontFilename, "textures/speccy_font_%s.bmp", _language );
    if( !g_app->m_resource->DoesTextureExist( fontFilename ) )
    {
        sprintf( fontFilename, "textures/speccy_font_normal.bmp" );
    }
    g_gameFont.Initialise( fontFilename );


    sprintf( fontFilename, "textures/editor_font_%s.bmp", _language );
    if( !g_app->m_resource->DoesTextureExist( fontFilename ) )
    {
        sprintf( fontFilename, "textures/editor_font_normal.bmp" );
    }
    g_editorFont.Initialise( fontFilename );

	if ( g_inputManager )
		m_langTable->RebuildTables();
}


void App::SetProfileName( char *_profileName )
{
    strcpy( m_userProfileName, _profileName );

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

const char *App::GetProfileDirectory()
{
#if defined(TARGET_OS_LINUX)

	static char userdir[256];
	const char *home = getenv("HOME");
	if (home != NULL) {
		sprintf(userdir, "%s/.darwinia", home);
		mkdir(userdir, 0777);

		sprintf(userdir, "%s/.darwinia/%s/", home, DARWINIA_GAMETYPE);
		mkdir(userdir, 0777);
		return userdir;
	}
	else // Current directory if no home
		return "";

#elif defined(TARGET_OS_MACOSX)

	static char userdir[256];
	const char *home = getenv("HOME");
	if (home != NULL) {
		sprintf(userdir, "%s/Library", home);
		mkdir(userdir, 0777);

		sprintf(userdir, "%s/Library/Application Support", home);
		mkdir(userdir, 0777);

		sprintf(userdir, "%s/Library/Application Support/Darwinia", home);
		mkdir(userdir, 0777);

		sprintf(userdir, "%s/Library/Application Support/Darwinia/%s/",
				home, DARWINIA_GAMETYPE);
		mkdir(userdir, 0777);

		return userdir;
	}
	else // Current directory if no home
		return "";

#else
#ifdef TARGET_OS_VISTA
    if( IsRunningVista() )
    {
        static char userdir[256];

		PWSTR path;
		SHGetKnownFolderPath( FOLDERID_SavedGames, 0, NULL, &path );
		wcstombs( userdir, path, sizeof(userdir) );
		CoTaskMemFree( path );

#ifdef TARGET_VISTA_DEMO2
		const char *subdir = "\\Darwinia Demo 2\\";
#else
		const char *subdir = "\\Darwinia\\";
#endif
        strncat(userdir, subdir, sizeof(userdir) );
        CreateDirectory( userdir );

        return userdir;
    }
    else
#endif // TARGET_OS_VISTA
	{
        return "";
    }
#endif
}

const char *App::GetPreferencesPath()
{
	// good leak #1
	static char *path = NULL;

	if (path == NULL) {
		const char *profileDir = GetProfileDirectory();
		path = new char[strlen(profileDir) + 32];
		sprintf( path, "%spreferences.txt", profileDir );
	}

	return path;
}

const char *App::GetScreenshotDirectory()
{
#ifdef TARGET_OS_VISTA
    static char dir[MAX_PATH];
    SHGetFolderPath( NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, dir );
    sprintf( dir, "%s\\", dir );
    return dir;
#else
    return "";
#endif
}

bool App::LoadProfile()
{
    DebugOut( "Loading profile %s\n", m_userProfileName );

    if( (stricmp( m_userProfileName, "AccessAllAreas" ) == 0 ||
		stricmp( m_userProfileName, "AttractMode" ) == 0 ) &&
        g_app->m_gameMode != GameModePrologue )
    {
        // Cheat username that opens all locations
        // aimed at beta testers who've completed the game already

        if( m_globalWorld )
        {
            delete m_globalWorld;
            m_globalWorld = NULL;
        }

        m_globalWorld = new GlobalWorld();
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
        m_globalWorld->LoadGame( m_gameDataFile );
    }

    return true;
}

bool App::SaveProfile( bool _global, bool _local )
{
    if( stricmp( m_userProfileName, "none" ) == 0 ) return false;
    if( stricmp( m_userProfileName, "AccessAllAreas" ) == 0 ) return false;
	if( stricmp( m_userProfileName, "AttractMode" ) == 0 ) return false;

    DebugOut( "Saving profile %s\n", m_userProfileName );

    char folderName[512];
    sprintf( folderName, "%susers/", GetProfileDirectory() );
    bool success = CreateDirectory( folderName );
    if( !success )
    {
        DebugOut( "failed to create folder %s\n", folderName );
        return false;
    }

    sprintf( folderName, "%susers/%s", GetProfileDirectory(), m_userProfileName );
    success = CreateDirectory( folderName );
    if( !success )
    {
        DebugOut( "failed to create folder %s\n", folderName );
        return false;
    }

#ifdef TARGET_OS_VISTA
	if( _global )
	{
		SaveRichHeader();
	}
#endif

    if( _global )
    {
        m_globalWorld->SaveGame( m_gameDataFile );
        DebugOut( "Saved global data for profile %s\n", m_userProfileName );
    }

    if( _local && g_app->m_location )
    {
        if( m_levelReset )
        {
            m_levelReset = false;
            return false;
        }

        g_app->m_location->m_levelFile->GenerateInstantUnits();
        g_app->m_location->m_levelFile->GenerateDynamicBuildings();
        char *missionFilename = m_location->m_levelFile->m_missionFilename;
        m_location->m_levelFile->SaveMissionFile( missionFilename );
        DebugOut( "Saved level %s for profile %s\n", missionFilename, m_userProfileName );
    }
    return true;
}


void App::ResetLevel( bool _global )
{
    if( m_location )
    {
        m_requestedLocationId = -1;
	    m_requestedMission[0] = '\0';
	    m_requestedMap[0] = '\0';

        if( g_app->m_tutorial ) g_app->m_tutorial->Restart();


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

bool App::HasBoughtGame()
{
#if defined(DEMOBUILD)
	return false;
#else
	return true;
#endif
}

void App::LoadPrologue()
{
    m_gameMode = App::GameModePrologue;

    m_soundSystem->StopAllSounds(WorldObjectId(), "Music");

    strcpy( m_gameDataFile, "game_demo2.txt" );
    LoadProfile();

    m_requestedLocationId = m_globalWorld->GetLocationId("launchpad");
    GlobalLocation *gloc = m_globalWorld->GetLocation(m_requestedLocationId);
    strcpy(m_requestedMap, gloc->m_mapFilename);
    strcpy(m_requestedMission, gloc->m_missionFilename);

    m_tutorial = new Demo2Tutorial();

    m_atMainMenu = false;

    g_prefsManager->SetInt( "RenderSpecialLighting", 1 );
	g_prefsManager->SetInt( "CurrentGameMode", 0 );
	g_prefsManager->Save();
}

void App::LoadCampaign()
{
    if( m_tutorial )
    {
        delete m_tutorial;
        m_tutorial = NULL;
    }

    m_soundSystem->StopAllSounds(WorldObjectId(), "Music");

    //m_atMainMenu = false;

    strcpy( m_gameDataFile, "game.txt" );
    LoadProfile();
    m_gameMode = App::GameModeCampaign;
    m_requestedLocationId = -1;
    g_prefsManager->SetInt( "RenderSpecialLighting", 0 );
    g_prefsManager->SetInt( "CurrentGameMode", 1 );
    g_prefsManager->Save();
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
