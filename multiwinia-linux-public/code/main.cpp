#include "lib/universal_include.h"

#include <stdio.h>
#include <math.h>
#include <exception>
#include <memory>
#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef TARGET_OS_VISTA
#include <wpc.h>
#include <shellapi.h>
#endif

#include <eclipse.h>

#include "lib/unicode/unicode_text_stream_reader.h"

#include "lib/avi_generator.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/input/input.h"
#include "lib/input/file_paths.h"
#include "lib/targetcursor.h"
#include "lib/math_utils.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/system_info.h"
#include "lib/text_renderer.h"
#include "lib/vector3.h"
#include "lib/window_manager.h"
#include "lib/avi_generator.h"
#include "lib/resource.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/language_table.h"
#include "lib/math/random_number.h"
#include "lib/filesys/filesys_utils.h"

#ifdef TARGET_MSVC
	#include "lib/input/win32_eventhandler.h"
	#include "lib/input/inputdriver_win32.h"
#endif
#ifdef _WIN32
	#include "lib/input/inputdriver_xinput.h"
#endif
#if (defined TARGET_OS_LINUX) || (defined TARGET_OS_MACOSX)
	#include "lib/input/sdl_eventhandler.h"
	#include "lib/input/inputdriver_sdl_key.h"
	#include "lib/input/inputdriver_sdl_mouse.h"
	#include "lib/window_manager_sdl.h"
#endif
#include "lib/input/inputdriver_prefs.h"
#include "lib/input/inputdriver_alias.h"
#include "lib/input/inputdriver_conjoin.h"
#include "lib/input/inputdriver_chord.h"
#include "lib/input/inputdriver_invert.h"
#include "lib/input/inputdriver_idle.h"
#include "lib/input/inputdriver_value.h"

#include "interface/prefs_other_window.h"
#include "interface/controllerunplugged_window.h"

#include "sound/sound_library_2d.h"
#include "sound/sound_library_3d_software.h"
#include "sound/sample_cache.h"

#ifdef HAVE_DSOUND
#include "sound/sound_library_3d_dsound.h"
#endif

#include "app.h"
#include "camera.h"
#include "location_editor.h"
#include "explosion.h"
#include "global_world.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "helpsystem.h"
#endif
#include "landscape.h"
#include "location.h"
#include "location_input.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "script.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "sound/soundsystem.h"
#include "sound/sample_cache.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "taskmanager_interface_icons.h"
#include "taskmanager_interface_gestures.h"
#include "taskmanager_interface_multiwinia.h"
#include "team.h"
#include "user_input.h"
#include "testharness.h"
#include "startsequence.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "tutorial.h"
#endif
#include "gamecursor.h"
#include "unit.h"
#include "attract.h"
#include "control_help.h"
#include "water.h"
#include "game_menu.h"
#include "shaman_interface.h"
#include "markersystem.h"
#include "multiwiniahelp.h"
#ifdef BENCHMARK_AND_FTP
    #include "benchmark.h"
#endif
#include "loading_screen.h"
#include "level_file.h"
#include "entity_grid.h"

#include "loaders/loader.h"
#include "loaders/egames_loader.h"
#include "loaders/oberon_loader.h"

#include "interface/globalworldeditor_window.h"
#include "interface/mainmenus.h"
#include "interface/updateavailable_window.h"
#include "interface/debugmenu.h"

#include "network/clienttoserver.h"
#include "network/server.h"
#include "network/servertoclientletter.h"
#include "network/network_defines.h"
#include "network/iframe.h"
#include "network/ftp_manager.h"

#include "worldobject/gunturret.h"
#include "lib/metaserver/metaserver.h"

#include "worldobject/darwinian.h"
#include "achievement_tracker.h"

#define TARGET_FRAME_RATE_INCREMENT 0.25




static void Finalise();


// ******************
//  Global Variables
// ******************

//double g_startTime = DBL_MAX;
double g_gameTime = 0.0;
double g_advanceTime;
//double g_lastServerAdvance;
//double g_predictionTime;
int g_lastProcessedSequenceId = -1;
int g_sliceNum;							// Most recently advanced slice
bool g_RenderingFirstTimeBL = false;
#ifdef TARGET_OS_VISTA
char g_saveFile[128];       // The profile name extracted from the save file that was used to launch darwinia
bool g_mediaCenter = false;
#endif

bool requestBootloader = false;

void SwitchTaskManagerForX360Controller();


// ******************
//  Local Functions
// ******************

void UpdateAdvanceTime()
{
	int recordDemo = g_prefsManager->GetInt("RecordDemo");
    if( recordDemo == 1 || recordDemo == 2 )
    {
		int demoFrameRate = g_prefsManager->GetInt("DemoFrameRate", 25);
        g_advanceTime = 1.0 / (double)demoFrameRate;
        IncrementFakeTime( 1.0 / (double)demoFrameRate );
        //g_gameTime += g_advanceTime;
        g_gameTime = GetHighResTime();
        //g_predictionTime = g_gameTime - g_lastServerAdvance - 0.07;
    }
    else
    {
        if( !g_app->GamePaused() )
        {
            double realTime = GetHighResTime();
	        g_advanceTime = realTime - g_gameTime;
	        if (g_advanceTime > 0.25)
	        {
		        g_advanceTime = 0.25;
	        }
	        g_gameTime = realTime;

            //double prevPredictionTime = g_predictionTime;
            //g_predictionTime = double(realTime - g_lastServerAdvance) - 0.07;
        }
        else
        {
            //g_predictionTime = 0.0;
        }

        //AppDebugOut( "Change = %6.3f\n", g_predictionTime - prevPredictionTime );
    }
}


//double GetNetworkTime()
//{
//    static double s_gameTime = 0.0f;
//
//    if( !g_app->GamePaused() )
//    {
//        s_gameTime = g_lastProcessedSequenceId * 0.1f;
//    }
//       
//    return s_gameTime;   
//}


bool WindowsOnScreen()
{
	return EclGetWindows()->Size() > 0;
}


void RemoveAllWindows()
{
    if( g_app->m_location )
    {
        int x = g_app->m_renderer->ScreenW() / 2;
        int y = g_app->m_renderer->ScreenH() / 2;
        g_target->SetMousePos( x, y );
    }

	LList<EclWindow *> *windows = EclGetWindows();
	while (windows->Size() > 0) {
		EclWindow *w = windows->GetData(0);
		EclRemoveWindow(w->m_name);
	}
}

#ifdef TARGET_MSVC
void PrintAvailableMemory()
{
	const DWORD MB = 1024 * 1024;
	MEMORYSTATUS stat;
	
	// Get the memory status.
	GlobalMemoryStatus( &stat );

	unsigned totalMemory, unusedMemory;
	g_cachedSampleManager.GetMemoryUsage( totalMemory, unusedMemory );

	AppDebugOut( "Available Memory: %4u MB Physical, %4u MB Virtual. Sample Cache = %d MB (%d%% in use)\n", 
		stat.dwAvailPhys / MB,
		stat.dwAvailVirtual / MB,
		totalMemory / MB,
		totalMemory == 0 ? 0 : (totalMemory - unusedMemory) * 100 / totalMemory );
}
#endif

void AdvanceAvailableMemoryCheck()
{
#ifdef TARGET_MSVC
	static double nextMemoryCheckTime = 0;
	if (GetHighResTime() > nextMemoryCheckTime)
	{
		PrintAvailableMemory();
		nextMemoryCheckTime = GetHighResTime() + 60;
	}	
#endif
}

bool HandleCommonConditions()
{
	g_app->HandleDelayedJobs();

	bool curWindowHasFocus = g_eventHandler->WindowHasFocus();

    static bool controllerPlugged = true;
#ifdef TARGET_MSVC
	if( controllerPlugged && g_inputManager->controlEvent( ControlControllerUnplugged ) )
	{
		ControllerUnpluggedWindow *dialog = new ControllerUnpluggedWindow();
		EclRegisterWindow( dialog );
        controllerPlugged = false;
	}
#endif

	if( !controllerPlugged && g_inputManager->controlEvent( ControlControllerPlugged ) )
	{
		EclRemoveWindow( "controller_unplugged" );
        controllerPlugged = true;
	}

	// Pretend we're not focused
    if( !controllerPlugged && g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
        curWindowHasFocus = false;

	g_app->m_lostFocusPause = !curWindowHasFocus;

	AdvanceAvailableMemoryCheck();

	if( g_inputManager->controlEvent( ControlGameDebugQuit ) )
	{
		g_app->m_requestQuit = true;
	}

	g_app->CheckSounds();
	
	if (g_app->m_requestQuit)
	{
		//g_app->SaveProfile( true, false );
		//g_app->SaveProfile( true, g_app->m_location != NULL );
		Finalise();

#ifdef TRACK_MEMORY_LEAKS
		AppPrintMemoryLeaks("leaks.txt");
#endif

		exit(0);
	}

	return false;
}


void SendTeamControlsToNetwork( TeamControls &teamControls )
{
	// Read the current teamControls from the inputManager				

	bool chatLog = g_app->m_camera->ChatLogVisible();
    bool entityUnderMouse = false;
    int numMouseButtons = g_prefsManager->GetInt( "ControlMouseButtons", 3 );

    Team *team = g_app->m_location->GetMyTeam();
    if( team )
    {
        bool checkMouse = false;
        if( teamControls.m_unitMove ) checkMouse = true;

        bool orderGiven = false;
        if( g_inputManager->getInputMode() == INPUT_MODE_KEYBOARD &&
            teamControls.m_primaryFireTarget )  orderGiven = true;
        if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD &&
            teamControls.m_secondaryFireDirected ) orderGiven = true;

        if( team->GetMyEntity() && 
            team->GetMyEntity()->m_type == Entity::TypeOfficer &&
            orderGiven )
                checkMouse = true;

        //if( checkMouse )
        {
            // We don't actually want to pass any left-clicks to the network system
            // If the user has left-clicked on another of his entities, because that 
            // entity is about to be selected.  We don't want our original entity 
            // walking up to him.
            WorldObjectId idUnderMouse;
            bool objectUnderMouse = g_app->m_locationInput->GetObjectUnderMouse( idUnderMouse, g_app->m_globalWorld->m_myTeamId );                    

            bool isCurrentEntity = ( objectUnderMouse && idUnderMouse.GetUnitId() == -1 && idUnderMouse.GetIndex() == team->m_currentEntityId );
            bool isCurrentUnit = ( objectUnderMouse && idUnderMouse.GetUnitId() != -1 && idUnderMouse.GetUnitId() == team->m_currentUnitId );
        
	        entityUnderMouse = ( 
                objectUnderMouse && 
                idUnderMouse.GetUnitId() != UNIT_BUILDINGS &&
                !isCurrentEntity && !isCurrentUnit );

            if( objectUnderMouse )
            {
                Entity *entity = g_app->m_location->GetEntity( idUnderMouse );
                if( entity )
                {
                    teamControls.m_mousePos = entity->m_pos;
                }
            }

            if( idUnderMouse.GetUnitId() == UNIT_BUILDINGS )
            {
                // Focus the mouse on a Radar Dish if one exists under the mouse
                Building *building = g_app->m_location->GetBuilding( idUnderMouse.GetUniqueId() );
                if( building && building->m_type == Building::TypeRadarDish )
                {
                    teamControls.m_mousePos = building->m_pos;
                }
            }
        }
    }

    if( g_app->m_taskManagerInterface->IsVisible()  ||
        EclGetWindows()->Size() != 0 || 
        chatLog /*entityUnderMouse || */ ) 
	{
		teamControls.ClearFlags();
	}
    
    g_app->m_clientToServer->SendIAmAlive( g_app->m_globalWorld->m_myTeamId, teamControls );
	
	teamControls.Clear();
}


void LocationGameLoop()
{
	g_app->m_loadingLocation = false;
	g_app->m_hideInterface = false;

    bool iAmAClient = true;
    bool iAmAServer = (g_app->m_server != NULL); 

    double nextIAmAliveMessage = GetHighResTime();
    double lastRenderTime = GetHighResTime();

	TeamControls teamControls;

	g_app->m_renderer->StartFadeIn(0.6f);
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "EnterLocation", SoundSourceBlueprint::TypeAmbience );


    //
    // Main loop

    bool multiwiniaCam = false;
	bool fadingOut = false;

	while(1)
	{
		if (!fadingOut)
		{
			if (g_app->m_requestedLocationId != g_app->m_locationId)
			{
				g_app->m_renderer->StartFadeOut();
				fadingOut = true;
			}
		}
		else
		{
			if (g_app->m_renderer->IsFadeComplete())
			{
				g_app->m_controlHelpSystem->Shutdown();
				break;
			}
		}

        if( !multiwiniaCam && g_app->m_gameMode == App::GameModeMultiwinia )
        {
            // this has to be done here because you cant guarentee the existance of the team before entering the loop
            if( g_app->m_location->GetMyTeam() )
            {
                char camname[64];
                sprintf(camname, "player%d", g_app->m_location->GetTeamPosition( g_app->m_globalWorld->m_myTeamId ) );
                g_app->m_camera->SetTarget( camname );
                g_app->m_camera->CutToTarget();
                multiwiniaCam = true;
            }
        }

		g_inputManager->PollForEvents(); 
        if( g_inputManager->controlEvent( ControlToggleInterface ) ) g_app->m_hideInterface = !g_app->m_hideInterface;
		if (g_inputManager->controlEvent( ControlMenuEscape ) && g_app->m_renderer->IsFadeComplete())
		{
			if (g_app->m_script && g_app->m_script->IsRunningScript())
			{
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
				g_app->m_sepulveda->PlayerSkipsMessage();
#endif
			}
			else
			{
				if (WindowsOnScreen())
                {
					RemoveAllWindows();
                    g_app->m_hideInterface = false;
                }
                else if( g_app->m_taskManagerInterface->IsVisible() )
                {
					g_app->m_taskManagerInterface->SetVisible( false );
                }
				else {
					g_app->m_camera->SetDebugMode(Camera::DebugModeAuto);
					EclRegisterWindow( new LocationWindow() );
                    g_app->m_hideInterface = true;
				}
			}
			g_app->m_userInput->Advance();				
		}

		// Clear out any display lists wanting to be created
		g_loadingScreen->RunJobs();

		if (HandleCommonConditions())
			continue;

        //
        // Get the time
        double timeNow = GetHighResTime();

		//
        // Advance the server
        if( iAmAServer )
		{
			g_app->m_server->AdvanceIfNecessary( timeNow );
		}

		if ( !WindowsOnScreen() )
			teamControls.Advance();

        if( iAmAClient )
		{            
            START_PROFILE( "Client Main Loop");
		
			//
            // Send Client input to Server
            if( timeNow > nextIAmAliveMessage )
            {
				SendTeamControlsToNetwork( teamControls );

                nextIAmAliveMessage += IAMALIVE_PERIOD;
				if (timeNow > nextIAmAliveMessage)
				{
					nextIAmAliveMessage = timeNow + IAMALIVE_PERIOD;
				}
            }

			g_app->m_clientToServer->AdvanceAndCatchUp();			

            if( g_app->m_multiwinia->m_resetLevel )
            {
                g_app->m_multiwinia->ResetLocation();
                multiwiniaCam = false;
            }

            END_PROFILE( "Client Main Loop");

			// Render
			UpdateAdvanceTime();
			lastRenderTime = GetHighResTime();
			if( g_profiler ) g_profiler->Advance();

			g_app->m_userInput->Advance();

			// Check Task Manager
			SwitchTaskManagerForX360Controller();

			// The following are candidates for running in parallel
			// using something like OpenMP
			g_app->m_location->m_water->Advance();
			bool soundSystemIsInitialized = g_app->m_soundSystem->IsInitialized();
			if( soundSystemIsInitialized )
				g_soundLibrary2d->TopupBuffer();

			if (g_eventHandler->WindowHasFocus())
				g_app->m_camera->Advance();

			g_app->m_locationInput->Advance();
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
			g_app->m_helpSystem->Advance();
#endif
			//g_app->m_taskManager->Advance();
			g_app->m_taskManagerInterface->Advance();
			g_app->m_script->Advance();
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
			g_app->m_sepulveda->Advance();
#endif
			g_explosionManager.Advance();
			if( soundSystemIsInitialized )
				g_app->m_soundSystem->Advance();
			g_app->m_controlHelpSystem->Advance();
			g_app->m_shamanInterface->Advance();
            g_app->m_markerSystem->Advance();
            g_app->m_multiwiniaHelp->Advance();
#ifdef BENCHMARK_AND_FTP
			g_app->m_benchMark->Advance();
#endif

#ifdef ATTRACTMODE_ENABLED
			if( g_app->m_attractMode->m_running )
			{
				g_app->m_attractMode->Advance();
			}
#endif // ATTRACTMODE_ENABLED

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
			if( g_app->m_tutorial ) 
				g_app->m_tutorial->Advance(); 
#endif

			// DELETEME: for debug purposes only
			g_app->m_globalWorld->EvaluateEvents();			

			g_app->m_renderer->Render();

			if (g_app->m_renderer->m_fps < 15) 
			{
				if( soundSystemIsInitialized )
					g_app->m_soundSystem->Advance();
			}
        }
    }

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Ambience EnterLocation" );
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "ExitLocation", SoundSourceBlueprint::TypeAmbience );
	g_app->ShutdownCurrentGame();
}

void SwitchTaskManagerForX360Controller()
{
	static int oldControlType = INPUT_MODE_KEYBOARD;

	if( oldControlType != INPUT_MODE_GAMEPAD &&
		g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD &&
		!g_app->m_taskManagerInterface->IsVisible() )
	{
		// user has just switched to the game pad
		TaskManagerInterface::CreateTaskManager();
		oldControlType = INPUT_MODE_GAMEPAD;
	}
	else if( oldControlType == INPUT_MODE_GAMEPAD &&
			 g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD &&
			 !g_app->m_taskManagerInterface->IsVisible() )
	{
		TaskManagerInterface::CreateTaskManager();
		oldControlType = g_inputManager->getInputMode();
	}
}


#ifdef LOCATION_EDITOR
#ifndef PURITY_CONTROL
void LocationEditorLoop()
{
	while( !g_inputManager->controlEvent( ControlMenuEscape ) )
    {
		g_inputManager->PollForEvents();

		if (HandleCommonConditions())
			continue;

        //
        // Get the time
        UpdateAdvanceTime();
        double timeNow = GetHighResTime();

        g_app->m_userInput->Advance();
        g_app->m_camera->Advance();            
		g_app->m_locationEditor->Advance();
		if( g_app->m_soundSystem->IsInitialized() )
			g_app->m_soundSystem->Advance();
        if( g_profiler ) g_profiler->Advance();

        g_app->m_renderer->Render();

    }

    RemoveAllWindows();

    if( strcmp( GetExtensionPart( g_app->m_location->m_levelFile->m_mapFilename ), "mwm" ) == 0 )
    {
        char filename[512];
        sprintf( filename, "%s%s", g_app->GetMapDirectory(), g_app->m_location->m_levelFile->m_mapFilename );
        MapFile *mapFile = new MapFile( filename, true );
        TextReader *in = mapFile->GetTextReader();
        g_app->m_gameMenu->RemoveMap( g_app->m_location->m_levelFile->m_mapFilename );
        g_app->m_gameMenu->AddMap( in, g_app->m_location->m_levelFile->m_mapFilename  );
        delete in;
        delete mapFile;
    }

	delete g_app->m_locationEditor;
	g_app->m_locationEditor = NULL;

	g_app->m_location->Empty();
	delete g_app->m_location;
	g_app->m_location = NULL;
	g_app->m_locationId = -1;
	g_app->m_requestedLocationId = -1;

	delete g_app->m_locationInput;
	g_app->m_locationInput = NULL;

	g_app->m_editing = false;
	g_app->m_atMainMenu = true;

    if( g_prefsManager->GetInt( "RecordDemo" ) == 1 )
    {
        if( g_app->m_server ) g_app->m_server->SaveHistory( "serverHistory.dat" );
        //g_inputManager->StopLogging( "inputLog.dat" );
    }
}
#endif // PURITY_CONTROL
#endif // LOCATION_EDITOR


void GlobalWorldGameLoop()
{
	g_app->m_renderer->StartFadeIn(0.25f);

    g_app->m_soundSystem->TriggerOtherEvent( NULL, "EnterGlobalWorld", SoundSourceBlueprint::TypeAmbience );

    g_app->m_hideInterface = false;
    g_app->m_atMainMenu = true;
	while(g_app->m_requestedLocationId == -1 && !g_app->m_requestToggleEditing)
    {
        if( g_app->m_atMainMenu ) break;
	
		g_inputManager->PollForEvents();
        if( g_inputManager->controlEvent( ControlToggleInterface ) ) g_app->m_hideInterface = !g_app->m_hideInterface;
		
        if( g_inputManager->controlEvent( ControlMenuEscape ) && g_app->m_renderer->IsFadeComplete() )
        {
			if (WindowsOnScreen()) 
			{
				RemoveAllWindows();
			}
			else 
			{
				g_app->m_camera->SetDebugMode(Camera::DebugModeAuto);
                EclRegisterWindow( new MainMenuWindow() );
			}
			g_app->m_userInput->Advance();	
        }

		if (HandleCommonConditions())
			continue;

        // Get the time
        UpdateAdvanceTime();
        double timeNow = GetHighResTime();

		if( g_app->m_server )
		{
			g_app->m_server->AdvanceIfNecessary(timeNow);
		}
		g_app->m_clientToServer->AdvanceAndCatchUp();
		g_app->m_script->Advance();
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
		g_app->m_sepulveda->Advance();
#endif
		g_app->m_globalWorld->Advance();
        g_app->m_userInput->Advance();
		g_app->m_camera->Advance();
		if( g_app->m_soundSystem->IsInitialized() )
			g_app->m_soundSystem->Advance();

#ifdef ATTRACTMODE_ENABLED
		g_app->m_attractMode->Advance();
#endif
		if( g_profiler ) g_profiler->Advance();
		
		g_app->m_globalWorld->EvaluateEvents();

        g_app->m_renderer->Render();
    }

	if (g_app->m_requestToggleEditing)
	{
		g_app->m_editing = true;
		g_app->m_requestToggleEditing = false;
	}

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Ambience EnterGlobalWorld" );
}


// *** GlobalWorldEditorLoop
void GlobalWorldEditorLoop()
{
	g_app->m_camera->SetDebugMode(Camera::DebugModeAlways);
	
    GlobalWorldEditorWindow *gweWindow = new GlobalWorldEditorWindow();
    EclRegisterWindow( gweWindow );
    
	while(g_app->m_requestedLocationId == -1 && !g_app->m_requestToggleEditing)
    {
		g_inputManager->PollForEvents();

        if( true )
        {
			g_app->m_editing = false;
			return;
        }

		if (HandleCommonConditions())
			continue;
		
        //
        // Get the time
        UpdateAdvanceTime();
        double timeNow = GetHighResTime();

		g_app->m_globalWorld->Advance();
        g_app->m_userInput->Advance();
        g_app->m_camera->Advance();            
		if( g_app->m_soundSystem->IsInitialized() )
			g_app->m_soundSystem->Advance();
		if( g_profiler ) g_profiler->Advance();

        g_app->m_renderer->Render();
    }

	if (g_app->m_requestToggleEditing)
	{
		g_app->m_editing = false;
		g_app->m_requestToggleEditing = false;
	}
}


void TestHarnessLoop()
{
#ifdef TEST_HARNESS_ENABLED
    g_app->m_testHarness->RunTest();
#endif
}

void SetPreferenceOverrides()
{
	// Fog can be an issue on certain drivers (e.g. Mac OS X - can causes FIFO hangs)

#ifdef FOG_LOGGING
	extern bool g_fogLogging;
	g_fogLogging = g_prefsManager->GetInt("RenderFog", 1) == 2;
#endif

#ifdef FOG_PREFERENCE	
	extern bool g_fogEnabled;
	g_fogEnabled = g_prefsManager->GetInt("RenderFog", 1);
#endif


}


#ifdef TARGET_MSVC
XInputDriver *g_xInputDriver;
#endif

void InitialiseInputManager()
{
	g_inputManager = new InputManager;
	g_inputManager->addDriver( new ConjoinInputDriver() );
	g_inputManager->addDriver( new ChordInputDriver() );
	g_inputManager->addDriver( new InvertInputDriver() );
	g_inputManager->addDriver( new IdleInputDriver() );
#if defined(TARGET_MSVC)
	g_inputManager->addDriver( new W32InputDriver() );
	
	if (LoadLibrary("XINPUT1_3"))
	{		
		g_xInputDriver = new XInputDriver();
		g_inputManager->addDriver( g_xInputDriver );
	}

#elif defined (TARGET_OS_LINUX) || defined( TARGET_OS_MACOSX )
	g_inputManager->addDriver ( g_sdlMouseDriver = new SDLMouseInputDriver() );
	g_inputManager->addDriver ( new SDLKeyboardInputDriver() );

#endif
	g_inputManager->addDriver( new PrefsInputDriver() );
	g_inputManager->addDriver( new ValueInputDriver() );
	g_inputManager->addDriver( new AliasInputDriver() );
}

void ReadInputPreferences()
{
	// Read Darwinia default input preferences file
	TextReader *inputPrefsReader = g_app->m_resource->GetTextReader( InputPrefs::GetSystemPrefsPath() );
	if ( inputPrefsReader ) {
		AppReleaseAssert( inputPrefsReader->IsOpen(), "Couldn't open input preferences file: %s\n",
		                       InputPrefs::GetSystemPrefsPath().c_str() );
		g_inputManager->parseInputPrefs( *inputPrefsReader );
		delete inputPrefsReader;
	}

	// Override defaults with keyboard specific file, if applicable
	TextReader *localeInputPrefsReader = g_app->m_resource->GetTextReader( InputPrefs::GetLocalePrefsPath() );
	if ( localeInputPrefsReader ) {
		if ( localeInputPrefsReader->IsOpen() ) {
			g_inputManager->parseInputPrefs( *localeInputPrefsReader, true );
		}
		delete localeInputPrefsReader;
	}


	// Override again with user specified bindings
	TextReader *userInputPrefsReader = new UnicodeTextFileReader( InputPrefs::GetUserPrefsPath().c_str() ); 

	if (userInputPrefsReader && !userInputPrefsReader->IsUnicode())
	{
		delete userInputPrefsReader;
		userInputPrefsReader = new TextFileReader( InputPrefs::GetUserPrefsPath().c_str() ); 
	}
	
	if ( userInputPrefsReader ) {
		if ( userInputPrefsReader->IsOpen() ) {
			g_inputManager->parseInputPrefs( *userInputPrefsReader, true );
		}
		delete userInputPrefsReader;
	}
}

bool IsRunningVista()
{
#if defined(TARGET_MSVC)
    OSVERSIONINFOEX versionInfo;
	ZeroMemory(&versionInfo, sizeof(OSVERSIONINFOEX));

	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx( (OSVERSIONINFO *) &versionInfo );

	if ( versionInfo.dwMajorVersion < 6 || 
		 versionInfo.wProductType != VER_NT_WORKSTATION )
	{
		return false;
	}

    return true;
#endif

    return false;
}

bool IsRunningAtLeast2k()
{
#if defined(TARGET_MSVC)
    OSVERSIONINFOEX versionInfo;
	ZeroMemory(&versionInfo, sizeof(OSVERSIONINFOEX));

	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx( (OSVERSIONINFO *) &versionInfo );

	if ( versionInfo.dwMajorVersion < 5 )
	{
		return false;
	}

    return true;
#endif

    return false;
}

#if defined(TARGET_OS_VISTA)
void DoVistaChecks()
{
    // Check to make sure the game is running on Vista

	if( !IsRunningVista() )
	{
        int buttonId = MessageBox( NULL, LANGUAGEPHRASE("error_incompatible"), LANGUAGEPHRASE("darwinia_vistaedition"), MB_OKCANCEL|MB_ICONERROR );
        if( buttonId == IDOK )
        {
            g_windowManager->OpenWebsite( "http://www.multiwinia.co.uk/" );
        }
		exit(0);
	}


	// Check Parental Controls to make sure current user is allowed to run Darwinia

	HRESULT hr = CoInitialize(NULL);

	IWindowsParentalControls* wpc = NULL;
	hr = CoCreateInstance(__uuidof(WindowsParentalControls), 0, CLSCTX_INPROC_SERVER, 
                    __uuidof(IWindowsParentalControls), (LPVOID *)&wpc);

	if( FAILED(hr) )
	{
		wprintf(L"Info:  Parental Controls interface not detected.\n");
        wprintf(L"Info:   This is an error if on a supported SKU of Windows Vista.\n");
	}
	else
	{
		IWPCGamesSettings *wpcGamesSettings = NULL;
		hr = wpc->GetGamesSettings( NULL, &wpcGamesSettings );
		if( FAILED(hr) )
		{
			wprintf(L"Warning:  Unable to obtain the Parental Controls user\n");
            wprintf(L"          settings interface.  This is expected if the\n");
            wprintf(L"          current user is a Protected Administrator or\n");
            wprintf(L"          Built-In Administrator.\n");
		}
		else
		{
			GUID guidGameId;
			guidGameId.Data1 = 0xF58175C7;
			guidGameId.Data2 = 0xE99C;
			guidGameId.Data3 = 0x4151;
			guidGameId.Data4[0] = 0x80;
			guidGameId.Data4[1] = 0x0D;
			guidGameId.Data4[2] = 0x7B;
			guidGameId.Data4[3] = 0xB5;
			guidGameId.Data4[4] = 0xE8;
			guidGameId.Data4[5] = 0x9E;
			guidGameId.Data4[6] = 0x49;
			guidGameId.Data4[7] = 0x60;

			DWORD reasons = 0;
			hr = wpcGamesSettings->IsBlocked( guidGameId, &reasons );

			if( FAILED(hr) ||
                reasons != WPCFLAG_ISBLOCKED_NOTBLOCKED )
			{
                MessageBox(NULL, LANGUAGEPHRASE("error_parental"), LANGUAGEPHRASE("darwinia_vistaedition"), MB_OK|MB_ICONERROR );
				exit(0);
			}
			wpcGamesSettings->Release();
		}
		wpc->Release();
	}
}

#endif

// Set the floating point mantissa size to 53 bits on Intel. The exponent
// size will still differ from PowerPC, but that shouldn't be a problem
// in practice.
void StandardiseCPUPrecision()
{
#if defined(_WIN32)
	_control87(_PC_53, _MCW_PC);
#elif defined(TARGET_OS_MACOSX) && defined(__i386__)
	// fesetenv() doesn't provide control over precision, so we use
	// inline assembly instructions to get and set the floating point
	// flags. The structure definition is taken from a Darwin header.
	struct {
		unsigned short __control;
		unsigned short __reserved1;
		unsigned short __status;
		unsigned short __reserved2;
		unsigned int __private3;
		unsigned int __private4;
		unsigned int __private5;
		unsigned int __private6;
		unsigned int __private7;
	} state;
	
    asm volatile ("fnstenv %0" : "=m" (state));
	state.__control &= 0xFEFF; // mantissa precision -> 53 bits
	asm volatile ("fldenv %0" : : "m" (state));
#elif defined (TARGET_OS_LINUX) && defined(__i386__)
	unsigned int mode = 0x27F;
	asm volatile ("fldcw %0" : : "m" (*&mode));
#elif defined (TARGET_OS_LINUX) && defined(TARGET_ARCH_AMD64)
	//We're in trouble, x87 is deprecated on amd64
	//It should still work (I hope) but SSE is apparently, the way of
	//the future. SSE should be compatible (double precision) but sin(),
	//cos() and other functions are not supported and must be done on
	//x87. We'll try setting the x87 mode regarless.
	unsigned int mode = 0x27F;
	asm volatile ("fldcw %0" : : "m" (*&mode));
#endif
}


void Initialise( const char *_cmdLine )
{
	{
		bool noProfile = ( _cmdLine && strstr( _cmdLine, "--NoProfile" ) );
		if( noProfile )
		{
			g_app->UseProfileDirectory( false );
		}

		const char *profileDir = g_app->GetProfileDirectory();
		if( profileDir[0] != '\0' )
		{
			CreateDirectoryRecursively( profileDir );

			AppDebugLogPathPrefix( profileDir );
		}
		else if( noProfile )
		{
			AppDebugLogPathPrefix( "" );
		}
	}

	#ifdef DUMP_DEBUG_LOG
		AppDebugOutRedirect("debug.txt");
	#endif
	AppDebugOut( "%s %s built %s\n", APP_NAME, APP_VERSION, __DATE__ );

	//
    // Initialise all our basic objects
	
	StandardiseCPUPrecision();

    g_systemInfo = new SystemInfo;
    InitialiseHighResTime();

	double startInit = GetHighResTime();

#if defined(TARGET_MSVC)
	g_eventHandler = new W32EventHandler();
#else
	g_eventHandler = new SDLEventHandler();
#endif
    g_loadingScreen = new LoadingScreen();

    InitialiseInputManager();
	g_app = new App();

	double start = GetHighResTime();
    ReadInputPreferences();
	AppDebugOut("Inits 12: %f\n", GetHighResTime() - start ); start = GetHighResTime();


#if defined(TARGET_OS_VISTA)
    DoVistaChecks();	
#endif

	g_target = new TargetCursor();
	//if( g_prefsManager->GetInt("ControlMethod")==0 ) getW32EventHandler()->BindAltTab();
    EntityBlueprint::Initialise();
	g_windowManager->HideMousePointer();

	//
	// Start on a specific level if the prefs file tells us to

	const char *startMap = g_prefsManager->GetString("StartMap");
	if (startMap)
	{
		g_app->m_requestedLocationId = g_app->m_globalWorld->GetLocationId(startMap);
		GlobalLocation *gloc = g_app->m_globalWorld->GetLocation(g_app->m_requestedLocationId);
		AppReleaseAssert(gloc, "Couldn't find location %s", startMap);
		strcpy(g_app->m_requestedMap, gloc->m_mapFilename);
		strcpy(g_app->m_requestedMission, gloc->m_missionFilename);
	}

    g_app->m_renderer->SetOpenGLState();

	
	AppDebugOut("Inits 13: %f\n", GetHighResTime() - start ); start = GetHighResTime();

	AppDebugOut("Initialisation took: %f seconds\n", GetHighResTime() - startInit );
}


void Finalise()
{
	
	if( g_app->m_clientToServer)
	{
		g_app->m_clientToServer->ClientLeave();
	}

	MetaServer_Disconnect();

#if TARGET_OS_MACOSX
	g_loadingScreen->m_workQueue->Empty();
	g_loadingScreen->m_workQueue->Wait();
	
	g_app->m_soundSystem->m_PreloadQueue->Empty();
	g_app->m_soundSystem->m_PreloadQueue->Wait();
#endif
	
    if( g_app->m_soundSystem->IsInitialized() && g_soundLibrary2d )
    {
        g_soundLibrary2d->Stop();
        if( g_soundLibrary3d )
            delete g_soundLibrary3d;
        delete g_soundLibrary2d;
        g_soundLibrary3d = NULL;
        g_soundLibrary2d = NULL;
    }

	EclShutdown();

	if( !g_app->m_usingFontCopies )
	{
		g_gameFont.Shutdown();
	}
	else
	{
		g_oldGameFont.Shutdown();
	}

	if( !g_app->m_usingFontCopies )
	{
		g_editorFont.Shutdown();
	}
	else
	{
		g_oldEditorFont.Shutdown();
	}

	g_titleFont.Shutdown();

    delete g_windowManager;

// Always clean on exit, this helps detect crashes and memory problems!
//#ifdef TRACK_MEMORY_LEAKS	

	delete g_target; g_target = NULL;
	delete g_systemInfo; g_systemInfo = NULL;
	delete g_app; g_app = NULL;

	g_cachedSampleManager.EmptyCache();
	delete g_inputManager; g_inputManager = NULL;	
	delete g_eventHandler; g_eventHandler = NULL;
	
	Profiler::Stop();
//#endif


#ifdef TARGET_OS_VISTA
	// Skip if not running on a Media Center
    if( g_mediaCenter ) 
	{
		// Get the path to Media Center
		WCHAR szExpandedPath[MAX_PATH];
		if( ExpandEnvironmentStringsW( L"%SystemRoot%\\ehome\\ehshell.exe", szExpandedPath, MAX_PATH) )
		{
			// Skip if ehshell.exe doesn't exist
			if( GetFileAttributesW( szExpandedPath ) != 0xFFFFFFFF )
			{		 
				// Launch ehshell.exe 
				INT_PTR result = (INT_PTR)ShellExecuteW( NULL, L"open", szExpandedPath, NULL, NULL, SW_SHOWNORMAL);
			}
		}
	}
#endif

}

bool IsFirsttimeSequencing()
{
	return g_RenderingFirstTimeBL;
}

void RequestBootloaderSequence()
{
	requestBootloader = true;
	g_RenderingFirstTimeBL = true;
}

#ifdef USE_LOADERS
void RunBootloader()
{
	if( g_loadingScreen )
	{
		g_loadingScreen->Render();
	}

	if( g_prefsManager )
	{
		// Show firsttime bootloader if this is the first time we've run Darwinia 
		// If it's anything other than "firsttime", do nothing more!
		const char *loaderName = g_prefsManager->GetString("BootLoader", "none");
		if( stricmp( loaderName, "firsttime" ) == 0 )
		{
			g_app->m_startSequence = new StartSequence();

			while( true )
			{
				UpdateAdvanceTime();
				bool amIDone = g_app->m_startSequence->Advance();
				if( amIDone ) break;
			}

			delete g_app->m_startSequence;
			g_app->m_startSequence = NULL;

			g_app->m_camera->SetTarget(Vector3(1000,500,1000), Vector3(0,-0.5,-1));
			g_app->m_camera->CutToTarget();

			g_prefsManager->SetString( "BootLoader", "random" );
			g_prefsManager->Save();
		}
		else 
		{
			int loaderType = Loader::GetLoaderIndex((char*)loaderName);
			if( loaderType != -1 )
			{
				Loader *loader = Loader::CreateLoader( loaderType );
				loader->Run();
				delete loader;
			}
		}
	}
	
	//}
	g_RenderingFirstTimeBL = false;
}
#endif // USE_LOADERS

static void DoEnterLocation()
{	
	// If we're in Darwinia mode, we should start the network now.
	// In Multiwinia mode, we have already started the network much earlier than this.
	if( !g_app->Multiplayer() )
	{
		if( !g_app->StartSinglePlayerServer() )
		{
			return;
		}
	}

	bool iAmAServer = g_app->m_server != NULL;

    //
    // Start new task manager

    TaskManagerInterface::CreateTaskManager();


	double t = GetHighResTime();

	WorldObjectId::ResetUniqueIdSeed();	
	bool loadingNew = true;
	g_app->m_location = new Location();

	g_app->m_locationInput = new LocationInput();

#if defined(PERMITTED_MAPS)
    bool mapPermitted = g_app->IsMapPermitted( LevelFile::CalculeCRC( g_app->m_requestedMap ) );
#endif

    if( loadingNew )
	{
		g_app->m_location->Init( g_app->m_requestedMission, g_app->m_requestedMap );
	}
	
	AppDebugOut("Location Init: %f seconds\n", GetHighResTime() - t );

	GunTurret::SetupTeamShapes();

    g_app->m_locationId = g_app->m_requestedLocationId;

    g_app->m_camera->UpdateEntityTrackingMode();
                
    if (!g_app->m_editing)
    {
        if( g_prefsManager->GetInt( "RecordDemo" ) == 2 )
        {
            if( g_app->m_server )
            {
                g_app->m_server->LoadHistory( "ServerHistory.dat" );
            }
        }
    }

    double const borderSize = 200.0;
    double minX = -borderSize;
    double maxX = g_app->m_location->m_landscape.GetWorldSizeX() + borderSize;
    g_app->m_camera->SetBounds(minX, maxX, minX, maxX);
    g_app->m_camera->SetTarget(Vector3(maxX,1000,maxX), Vector3(-1,-0.7,-1)); // In case start doesn't exist
    g_app->m_camera->SetTarget("start");
    g_app->m_camera->CutToTarget();

#if defined(PERMITTED_MAPS)
    if( !mapPermitted )
    {
        g_app->m_requestedLocationId = -1;
        if( g_app->m_gameMode != App::GameModeCampaign )
        {
            g_app->m_atMainMenu = true;
        }
    }
#endif

}

void EnterLocation()
{
	// m_loadingLocation is also set earlier when the game start message
	// is received, and prevents updates from being processed by the client
	g_app->m_loadingLocation = true;
	AppDebugOut( "Time at call to EnterLocation() = %f\n", GetHighResTime() );

	g_loadingScreen->m_workQueue->Add( &DoEnterLocation );
	g_loadingScreen->Render();

    if (g_app->m_editing)
    {
#ifdef LOCATION_EDITOR
#ifndef PURITY_CONTROL
	    g_app->m_locationEditor = new LocationEditor();
	    g_app->m_camera->SetDebugMode(Camera::DebugModeAlways);
				
	    LocationEditorLoop();
#endif  // PURITY_CONTROL
#endif // LOCATION_EDITOR
    }
    else
    {
	    g_app->m_camera->SetDebugMode(Camera::DebugModeAuto);
	    g_app->m_camera->RequestMode(Camera::ModeFreeMovement);

#ifdef BENCHMARK_AND_FTP
        g_app->m_benchMark->Begin();
#endif

	    LocationGameLoop();

#ifdef BENCHMARK_AND_FTP
        g_app->m_benchMark->End();
        g_app->m_benchMark->Upload();
#endif

#ifdef DEMOBUILD
#ifndef DEMO2
        //PrintMemoryLeaks();
        //g_windowManager->OpenWebsite( "http://www.darwinia.co.uk/demoend/" );
        g_windowManager->OpenWebsite( "http://www.multiwinia.co.uk/" );
	    Finalise();
	    exit(0);
#endif // DEMO2
#endif // DEMOBUILD
    }
}

void EnterGlobalWorld()
{
    if( g_app->m_gameMode == App::GameModePrologue &&
        !g_app->m_script->IsRunningScript() )
    {
        // the only time you should see the world in prologue is during the cutscene
        g_app->m_atMainMenu = true;
    }

    // Put the camera in a sensible place
    g_app->m_camera->SetDebugMode(Camera::DebugModeAuto);
    g_app->m_camera->RequestMode(Camera::ModeSphereWorld);
    g_app->m_camera->SetHeight( 50.0 );
    
    if (g_app->m_editing)
    {
	    GlobalWorldEditorLoop();
    }
    else
    {                
	    GlobalWorldGameLoop();
    }
}


void MainMenuLoop()
{
	while( g_app->m_atMainMenu && !g_app->m_requestToggleEditing )
    {
		CHECK_OPENGL_STATE();
        UpdateAdvanceTime();
		CHECK_OPENGL_STATE();
        g_app->m_renderer->Render();
        g_app->m_userInput->Advance();
        //g_app->m_camera->Advance();  
        if( g_app->m_soundSystem->IsInitialized() )
            g_app->m_soundSystem->Advance();
        HandleCommonConditions();

		if( g_app->m_achievementTracker &&
			!g_app->m_achievementTracker->HasLoaded() )
		{
			g_app->m_achievementTracker->Load();
		}

        if( !g_app->m_gameMenu->m_menuCreated )
        {
            if( g_app->m_renderer->IsFadeComplete() )
            {
                g_app->m_gameMenu->CreateMenu();
            }
        }

		//
		// Advance the server
		if( g_app->m_server )
			g_app->m_server->AdvanceIfNecessary( GetHighResTime() );

		g_app->m_clientToServer->AdvanceAndCatchUp();
    }

	if (g_app->m_requestToggleEditing)
	{
		g_app->m_editing = true;
		g_app->m_requestToggleEditing = false;
		g_app->m_atMainMenu = false;
	}

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "MultiwiniaInterface LobbyAmbience" );
    g_app->m_gameMenu->DestroyMenu();
	g_app->m_requireSoundsLoaded = true;
	CHECK_OPENGL_STATE();
}

void RunTheGame( const char *_cmdLine )
{    
	Initialise( _cmdLine );

#ifdef TEST_HARNESS_ENABLED
    if( g_prefsManager->GetInt("TestHarness") == 1 )
    {
        g_app->m_testHarness = new TestHarness();
		TestHarnessLoop();
		Finalise();
		exit(0);
    }
#endif // TEST_HARNESS_ENABLED

	// 
	// Do whatever mode was requested


	g_loadingScreen->m_initialLoad = false;
    //g_app->m_gameMenu->CreateMenu();



	while (true)
    {
		if (requestBootloader)
		{
#ifdef USE_LOADERS
			RunBootloader();
#endif
			requestBootloader = false;
		}
		
		if( g_app->m_atMainMenu )
        {
            MainMenuLoop();
        }
        else if (g_app->m_requestedLocationId != -1)
        {
            EnterLocation();
        }
        else // Not in location
        {
            EnterGlobalWorld();
        }        

		g_inputManager->Advance();
    }
}

#if defined(USE_CRASHREPORTING) && defined(TARGET_MSVC) 
#include "lib/exception_handler.h"
#endif

#ifdef TARGET_DEBUG
double ToDouble( unsigned long long rep )
{
	union {
		unsigned long long word;
		char data[16];
		double d;
	} x;

	x.word = rep;
	for(int i = 0; i < 8; i++)
	{
		std::swap( x.data[i], x.data[15-i] );
	}

	return x.d;
}
#endif

// Main Function
void AppMain( const char *_cmdLine )
{
	//AppDebugOut( "a999999999999ef3 as double = %16.16f\n", ToDouble( 0xa999999999999ef3LL ) );

#if defined(USE_CRASHREPORTING) && defined(TARGET_MSVC) 
    char profile[512];
    memset( profile, 0, sizeof(profile) );

    __try
#endif
    {
		// write the version
		if( _cmdLine && strstr( _cmdLine, "--write-version" ) != 0 )
		{
			FILE *fp = fopen("version", "w");
			if( fp )
			{
				fprintf( fp, "%s", DARWINIA_VERSION );
				fclose( fp );
				return;
			}
		}

#if defined(USE_CRASHREPORTING) && defined(TARGET_MSVC) 
        strncpy( profile, g_app->GetProfileDirectory(), sizeof(profile) );
        profile[ sizeof(profile) - 1 ] = '\0';
#endif

        RunTheGame( _cmdLine );

    }
#if defined(USE_CRASHREPORTING) && defined(TARGET_MSVC) 
    __except( RecordExceptionInfo( GetExceptionInformation(), profile, "AppMain", "0B9432AF", DARWINIA_VERSION ) ) {}
#endif
}










