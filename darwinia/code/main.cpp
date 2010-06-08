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
#include "lib/text_stream_readers.h"
#include "lib/language_table.h"

#include "lib/input/win32_eventhandler.h"
#include "lib/input/inputdriver_win32.h"
#include "lib/input/inputdriver_xinput.h"
#include "lib/input/inputdriver_prefs.h"
#include "lib/input/inputdriver_alias.h"
#include "lib/input/inputdriver_conjoin.h"
#include "lib/input/inputdriver_chord.h"
#include "lib/input/inputdriver_invert.h"
#include "lib/input/inputdriver_idle.h"
#include "lib/input/inputdriver_value.h"

#include "interface/prefs_other_window.h"

#include "sound/sound_library_2d.h"
#include "sound/sound_library_3d_software.h"

#ifdef HAVE_DSOUND
#include "sound/sound_library_3d_dsound.h"
#endif

#include "app.h"
#include "camera.h"
#include "location_editor.h"
#include "explosion.h"
#include "global_world.h"
#include "helpsystem.h"
#include "landscape.h"
#include "location.h"
#include "location_input.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "script.h"
#include "sepulveda.h"
#include "sound/soundsystem.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "taskmanager_interface_icons.h"
#include "taskmanager_interface_gestures.h"
#include "team.h"
#include "user_input.h"
#include "testharness.h"
#include "startsequence.h"
#include "tutorial.h"
#include "gamecursor.h"
#include "unit.h"
#include "attract.h"
#include "control_help.h"
#include "water.h"
#include "game_menu.h"
#include "demoendsequence.h"

#include "loaders/loader.h"

#include "interface/pokey_window.h"
#include "interface/globalworldeditor_window.h"
#include "interface/mainmenus.h"
#include "interface/updateavailable_window.h"
#include "interface/debugmenu.h"

#include "network/clienttoserver.h"
#include "network/server.h"
#include "network/servertoclientletter.h"

#include "worldobject/darwinian.h"

#define TARGET_FRAME_RATE_INCREMENT 0.25f

static void Finalise();


// ******************
//  Global Variables
// ******************

double g_startTime = DBL_MAX;
double g_gameTime = 0.0;
float g_advanceTime;
double g_lastServerAdvance;
float g_predictionTime;
float g_targetFrameRate = 20.0f;
int g_lastProcessedSequenceId = -1;
int g_sliceNum;							// Most recently advanced slice
#ifdef TARGET_OS_VISTA
char g_saveFile[128];       // The profile name extracted from the save file that was used to launch darwinia
bool g_mediaCenter = false;
#endif

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
        g_advanceTime = 1.0f / (float)demoFrameRate;
        IncrementFakeTime( 1.0f / (double)demoFrameRate );
        //g_gameTime += g_advanceTime;
        g_gameTime = GetHighResTime();
        g_predictionTime = float(g_gameTime - g_lastServerAdvance) - 0.07f;
    }
    else
    {
        double realTime = GetHighResTime();
	    g_advanceTime = float(realTime - g_gameTime);
	    if (g_advanceTime > 0.25f)
	    {
		    g_advanceTime = 0.25f;
	    }
	    g_gameTime = realTime;

        float prevPredictionTime = g_predictionTime;
        g_predictionTime = float(realTime - g_lastServerAdvance) - 0.07f;

        //DebugOut( "Change = %6.3f\n", g_predictionTime - prevPredictionTime );
    }
}


double GetNetworkTime()
{
    return g_lastProcessedSequenceId * 0.1f;
}


void UpdateTargetFrameRate(int _currentSlice)
{
    int numUpdatesToProcess = g_app->m_clientToServer->m_lastValidSequenceIdFromServer - g_lastProcessedSequenceId;
    int numSlicesPending = numUpdatesToProcess * NUM_SLICES_PER_FRAME - _currentSlice;
	float timeSinceStartOfAdvance = g_gameTime - g_lastServerAdvance;
	int numSlicesThatShouldBePending = 10 - timeSinceStartOfAdvance * 10.0f;

	// Increase or lower the target frame rate, depending on how far behind schedule
	// we are
//	if( numSlicesPending > NUM_SLICES_PER_FRAME/2 )
	float amountBehind = numSlicesPending - numSlicesThatShouldBePending;
	g_targetFrameRate -= 0.1f * amountBehind * TARGET_FRAME_RATE_INCREMENT;

	// Make sure the target frame rate is within sensible bounds
	if( g_targetFrameRate < 2.0f )
	{
     	g_targetFrameRate = 2.0f;
	}
	else if( g_targetFrameRate > 85.0f)
	{
		g_targetFrameRate = 85.0f;
	}
}


/*
int GetNumSlicesToAdvance()
{
	int slicesPerSecond = SERVER_ADVANCE_FREQ * NUM_SLICES_PER_FRAME;
	float ratio = (float)slicesPerSecond / (float)g_targetFrameRate;

#ifdef AVI_GENERATOR
	if (g_app->m_aviGenerator)
	{
		ratio = (float)slicesPerSecond / (float)g_app->m_aviGenerator->GetRecordingFrameRate();
	}
#endif
	
	static float accumulator = 0.0f;
	accumulator += ratio;
	
	int returnVal = floorf(accumulator);

	accumulator -= (float)returnVal;

	return returnVal;
}*/



int GetNumSlicesToAdvance()
{
    int numUpdatesToProcess = g_app->m_clientToServer->m_lastValidSequenceIdFromServer - g_lastProcessedSequenceId;
    int numSlicesPending = numUpdatesToProcess * NUM_SLICES_PER_FRAME;
    if( g_sliceNum != -1 ) numSlicesPending -= g_sliceNum;
    else if( g_sliceNum == -1 ) numSlicesPending -= 10;
    
    float timeSinceStartOfAdvance = g_gameTime - g_lastServerAdvance;
	//int numSlicesThatShouldBePending = 10 - timeSinceStartOfAdvance * 10.0f;

    int numSlicesToAdvance = timeSinceStartOfAdvance * 100;
    if( g_sliceNum != -1 ) numSlicesToAdvance -= g_sliceNum;
    if( g_sliceNum == -1 ) numSlicesToAdvance -= 10;

    //DarwiniaDebugAssert( numSlicesToAdvance >= 0 );
    numSlicesToAdvance = max( numSlicesToAdvance, 0 );
    numSlicesToAdvance = min( numSlicesToAdvance, 10 );
    
    return numSlicesToAdvance;
}


bool ProcessServerLetters( ServerToClientLetter *letter )
{
    switch( letter->m_type )
    {
        case ServerToClientLetter::HelloClient:
            if( letter->m_ip == g_app->m_clientToServer->GetOurIP_Int() )
            {
                DebugOut( "CLIENT : Received HelloClient from Server\n" );
            }
            return true;

        case ServerToClientLetter::GoodbyeClient:
            //g_app->m_location->RemoveTeam( letter->m_teamId );
            return true;

        case ServerToClientLetter::TeamAssign:    
            
            if( letter->m_ip == g_app->m_clientToServer->GetOurIP_Int() )
            {
                g_app->m_location->InitialiseTeam(letter->m_teamId, letter->m_teamType );
            }
            else
            {
                g_app->m_location->InitialiseTeam(letter->m_teamId, Team::TeamTypeRemotePlayer );
            }
            return true;

        default:
            return false;
    }
}


bool WindowsOnScreen()
{
	return EclGetWindows()->Size() > 0;
}


void RemoveAllWindows()
{
	LList<EclWindow *> *windows = EclGetWindows();
	while (windows->Size() > 0) {
		EclWindow *w = windows->GetData(0);
		EclRemoveWindow(w->m_name);
	}
}

bool HandleCommonConditions()
{
	bool curWindowHasFocus = g_eventHandler->WindowHasFocus();
	static bool oldWindowFocus = true;

    static bool controllerPlugged = true;
	if( controllerPlugged && g_inputManager->controlEvent( ControlControllerUnplugged ) )
	{
		MessageDialog *dialog = new MessageDialog( LANGUAGEPHRASE("dialog_unplugged1"), LANGUAGEPHRASE("dialog_unplugged2") );
		EclRegisterWindow( dialog );
        controllerPlugged = false;
	}

	if( !controllerPlugged && g_inputManager->controlEvent( ControlControllerPlugged ) )
	{
		EclRemoveWindow( LANGUAGEPHRASE("dialog_unplugged1") );
        controllerPlugged = true;
	}

	// Pretend we're not focused
    if( !controllerPlugged && g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
        curWindowHasFocus = false;

	if( !curWindowHasFocus )
	{
        g_app->m_userInput->Advance();
        g_app->m_soundSystem->Advance();

		// Render twice to avoid double buffering artefacts
		g_app->m_renderer->Render();
		g_app->m_renderer->Render();
		return true;
	}
	
#ifdef TARGET_OS_MACOSX
	// Quit on Command-Q
	if (g_keyDeltas[KEY_Q] && g_keys[KEY_META]) { // TODO: This
		if (g_app->m_requestedLocationId != -1)
			DebugKeyBindings::ReallyQuitButton();
		else
			g_app->m_requestQuit = true;
	}
#endif
	
	if (g_app->m_requestQuit)
	{
		if( g_app->m_gameMode == App::GameModePrologue )
		{
			g_app->SaveProfile( true, true );
		}

		//g_app->SaveProfile( true, g_app->m_location != NULL );
		PrintMemoryLeaks();
		Finalise();
		exit(0);
	}

	return false;
}


unsigned char GenerateSyncValue()
{
#ifdef TARGET_DEBUG
    Vector3 unitPosition;
    Vector3 entityPosition;
    Vector3 laserPosition;
    Vector3 effectsPosition;

    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        Team *team = &g_app->m_location->m_teams[t];
        if( team->m_teamType != Team::TeamTypeUnused )
        {
            for( int u = 0; u < team->m_units.Size(); ++u )
            {
                if( team->m_units.ValidIndex(u) )
                {
                    Unit *unit = team->m_units[u];                    
                    unitPosition += unit->m_centrePos;
                    for( int e = 0; e < unit->m_entities.Size(); ++e )
                    {
                        if( unit->m_entities.ValidIndex(e) )
                        {
                            Entity *ent = unit->m_entities[e];
                            unitPosition += ent->m_pos;
                            unitPosition += ent->m_vel;
                        }
                    }
                }
            }

            for( int e = 0; e < team->m_others.Size(); ++e )
            {
                if( team->m_others.ValidIndex(e) )
                {
                    Entity *entity = team->m_others[e];
                    entityPosition += entity->m_pos;
                    entityPosition += entity->m_vel;
                }
            }
        }
    }

    for( int l = 0; l < g_app->m_location->m_lasers.Size(); ++l )
    {
        if( g_app->m_location->m_lasers.ValidIndex(l) )
        {
            Laser *laser = g_app->m_location->m_lasers.GetPointer(l);
            laserPosition += laser->m_pos;
            laserPosition += laser->m_vel;
        }
    }
    
    for( int e = 0; e < g_app->m_location->m_effects.Size(); ++e )
    {
        if( g_app->m_location->m_effects.ValidIndex(e) )
        {
            WorldObject *wobj = g_app->m_location->m_effects[e];
            effectsPosition += wobj->m_pos;
            effectsPosition += wobj->m_vel;
        }
    }


    Vector3 position = unitPosition + entityPosition + laserPosition + effectsPosition;
    float totalValue = position.x + position.y + position.z;

    totalValue -= int(totalValue);    
    DarwiniaDebugAssert( totalValue >= 0.0f && totalValue <= 1.0f );
    unsigned char syncValue = totalValue * 255;
    
//    float unitPositionSync = ( unitPosition.x + unitPosition.y + unitPosition.z );
//    float entityPositionSync = ( entityPosition.x + entityPosition.y + entityPosition.z );
//    float laserPositionSync = ( laserPosition.x + laserPosition.y + laserPosition.z );
//    float effectsPositionSync = ( effectsPosition.x + effectsPosition.y + effectsPosition.z );
//    
//    unitPositionSync -= (int) unitPositionSync;
//    entityPositionSync -= (int) entityPositionSync;
//    laserPositionSync -= (int) laserPositionSync;
//    effectsPositionSync -= (int) effectsPositionSync;
//
//    DebugOut( "Frame [%3d] Sync [%3d] unit[%3d] entity[%3d] laser[%3d] effects[%3d]\n",
//            g_lastProcessedSequenceId, syncValue,
//            (int)(unitPositionSync*255), 
//            (int)(entityPositionSync*255), 
//            (int)(laserPositionSync*255), 
//            (int)(effectsPositionSync*255) );
    
    return syncValue;

#else

    return 255 * syncfrand();

#endif
}


void LocationGameLoop()
{
    bool iAmAClient = true;
    bool iAmAServer = g_prefsManager->GetInt("IAmAServer") ? true : false;

    double nextServerAdvanceTime = GetHighResTime();
    double nextIAmAliveMessage = GetHighResTime();
    double heavyWeightAdvanceStartTime = -1;
    double serverAdvanceStartTime = -1;
    double lastRenderTime = GetHighResTime();

	TeamControls teamControls;

    g_sliceNum = -1;

	g_app->m_renderer->StartFadeIn(0.6f);
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "EnterLocation", SoundSourceBlueprint::TypeAmbience );


    //
    // Main loop

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

		g_inputManager->PollForEvents(); 
		if (g_inputManager->controlEvent( ControlMenuEscape ) && g_app->m_renderer->IsFadeComplete())
		{
			if (g_app->m_script && g_app->m_script->IsRunningScript())
			{
				g_app->m_sepulveda->PlayerSkipsMessage();
			}
			else
			{
				if (WindowsOnScreen())
					RemoveAllWindows();
                else if( g_app->m_taskManagerInterface->m_visible )
                {
                    g_app->m_taskManagerInterface->m_visible = false;
                }
				else {
					g_app->m_camera->SetDebugMode(Camera::DebugModeAuto);
					EclRegisterWindow( new LocationWindow() );
				}
			}
			g_app->m_userInput->Advance();				
		}

		if (HandleCommonConditions())
			continue;

        //
        // Get the time
        double timeNow = GetHighResTime();

		//
        // Advance the server
        if( iAmAServer )
        {
            if( timeNow > nextServerAdvanceTime )
            {
                g_app->m_server->Advance();
                nextServerAdvanceTime += SERVER_ADVANCE_PERIOD;
                if (timeNow > nextServerAdvanceTime)
				{
					nextServerAdvanceTime = timeNow + SERVER_ADVANCE_PERIOD;
				}
            }
        }        		

		if ( !WindowsOnScreen() )
			teamControls.Advance();

        if( iAmAClient )
        {            
            START_PROFILE(g_app->m_profiler, "Client Main Loop");
		
            //
            // Send Client input to Server
            if( timeNow > nextIAmAliveMessage )
            {
				// Read the current teamControls from the inputManager				

				bool chatLog = g_app->m_sepulveda->ChatLogVisible();
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

                    if( checkMouse )
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

                if( g_app->m_taskManagerInterface->m_visible ||
                    EclGetWindows()->Size() != 0 || 
					chatLog || entityUnderMouse ) 
				{
					teamControls.ClearFlags();
				}
                
                g_app->m_clientToServer->SendIAmAlive( g_app->m_globalWorld->m_myTeamId, teamControls );

                nextIAmAliveMessage += IAMALIVE_PERIOD;
				if (timeNow > nextIAmAliveMessage)
				{
					nextIAmAliveMessage = timeNow + IAMALIVE_PERIOD;
				}

				teamControls.Clear();
            }

			g_app->m_clientToServer->Advance();

            //UpdateTargetFrameRate(g_sliceNum);

			int slicesToAdvance = GetNumSlicesToAdvance();

            END_PROFILE(g_app->m_profiler, "Client Main Loop");

			// Do our heavy weight physics
			for (int i = 0; i < slicesToAdvance; ++i)
			{
				if( g_sliceNum == -1 )
				{
					// Read latest update from Server
					ServerToClientLetter *letter = g_app->m_clientToServer->GetNextLetter();

					if( letter )
					{
						DarwiniaDebugAssert( letter->GetSequenceId() == g_lastProcessedSequenceId + 1 );
                        //g_app->m_clientToServer->m_lastServerLetterReceivedTime = GetHighResTime();

						//DebugOut( "CLIENT : Processed update %d\n", letter->GetSequenceId() );
						//g_app->m_clientToServer->m_lastKnownSequenceIdFromServer = letter->GetSequenceId();
						bool handled = ProcessServerLetters(letter);
						if (handled == false)
						{
							g_app->m_clientToServer->ProcessServerUpdates( letter );
						}
                
						g_sliceNum = 0;
						heavyWeightAdvanceStartTime = timeNow;
						g_lastServerAdvance = (float)letter->GetSequenceId() * SERVER_ADVANCE_PERIOD + g_startTime;
						g_lastProcessedSequenceId = letter->GetSequenceId();
						delete letter;

                        unsigned char sync = GenerateSyncValue();
                        g_app->m_clientToServer->SendSyncronisation( g_lastProcessedSequenceId, sync );
					}
				}
				
				if( g_sliceNum != -1 )
				{               
        			g_app->m_location->Advance( g_sliceNum );
					g_app->m_particleSystem->Advance( g_sliceNum );
               
					if (g_sliceNum < NUM_SLICES_PER_FRAME-1)
					{
						g_sliceNum++;
					}
					else
					{
						g_sliceNum = -1;
						heavyWeightAdvanceStartTime = -1.0;
					}
				}
			}

			// Render
			UpdateAdvanceTime();
			lastRenderTime = GetHighResTime();
#ifdef PROFILER_ENABLED
			g_app->m_profiler->Advance();
#endif // PROFILER_ENABLED

			g_app->m_userInput->Advance();

			// Check Task Manager
			SwitchTaskManagerForX360Controller();

			// The following are candidates for running in parallel
			// using something like OpenMP
			g_app->m_location->m_water->Advance(); 
			g_soundLibrary2d->TopupBuffer(); 
			g_app->m_camera->Advance(); 
			g_app->m_locationInput->Advance(); 
			g_app->m_helpSystem->Advance(); 
			g_app->m_taskManager->Advance();
			g_app->m_taskManagerInterface->Advance(); 
			g_app->m_script->Advance(); 
			g_app->m_sepulveda->Advance(); 
			g_explosionManager.Advance(); 
			g_app->m_soundSystem->Advance(); 
			g_app->m_controlHelpSystem->Advance(); 
			
#ifdef ATTRACTMODE_ENABLED
			if( g_app->m_attractMode->m_running )
			{
				g_app->m_attractMode->Advance();
			}
#endif // ATTRACTMODE_ENABLED

			if( g_app->m_tutorial ) 
				g_app->m_tutorial->Advance(); 

			// DELETEME: for debug purposes only
			g_app->m_globalWorld->EvaluateEvents();			

			g_app->m_renderer->Render();

			if (g_app->m_renderer->m_fps < 15) 
			{
				g_app->m_soundSystem->Advance();
			}
        }
    }
    
    g_app->SaveProfile( false, true );

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Ambience EnterLocation" );
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "ExitLocation", SoundSourceBlueprint::TypeAmbience );
    
    if( g_prefsManager->GetInt( "RecordDemo" ) == 1 )
    {
        if( g_app->m_server ) g_app->m_server->SaveHistory( "serverHistory.dat" );
        //g_inputManager->StopLogging( "inputLog.dat" );
    }
        
	g_explosionManager.Reset();

	if( g_app->m_globalWorld->GetLocationName( g_app->m_locationId ) )
	{
		g_app->m_globalWorld->TransferSpirits( g_app->m_locationId );
	}

	g_app->m_clientToServer->ClientLeave();
	g_app->m_location->Empty();
	g_app->m_particleSystem->Empty();

//	g_app->m_inLocation = false;
//	g_app->m_requestedLocationId = false;

	delete g_app->m_location;
	g_app->m_location = NULL;
	g_app->m_locationId = -1;

	delete g_app->m_locationInput;
	g_app->m_locationInput = NULL;

    delete g_app->m_server;
    g_app->m_server = NULL;

    g_app->m_taskManager->StopAllTasks();
    
    g_app->m_globalWorld->m_myTeamId = 255;
    g_app->m_globalWorld->EvaluateEvents();
    g_app->SaveProfile( true, false );
}

void SwitchTaskManagerForX360Controller()
{
	static int oldControlType = INPUT_MODE_KEYBOARD;

	if( oldControlType != INPUT_MODE_GAMEPAD &&
		g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD &&
		!g_app->m_taskManagerInterface->m_visible )
	{
		// user has just switched to the game pad
		if( g_prefsManager->GetInt( "ControlMethod" ) == 0 )
		{
			delete g_app->m_taskManagerInterface;
			g_app->m_taskManagerInterface = new TaskManagerInterfaceIcons();
		}
		oldControlType = INPUT_MODE_GAMEPAD;
	}
	else if( oldControlType == INPUT_MODE_GAMEPAD &&
			 g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD &&
			 !g_app->m_taskManagerInterface->m_visible )
	{
		if( g_prefsManager->GetInt( "ControlMethod" ) == 0 )
		{
			delete g_app->m_taskManagerInterface;
			g_app->m_taskManagerInterface = new TaskManagerInterfaceGestures();
		}
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
        g_app->m_soundSystem->Advance();
        #ifdef PROFILER_ENABLED
            g_app->m_profiler->Advance();
        #endif

        g_app->m_renderer->Render();

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

	while(g_app->m_requestedLocationId == -1 && !g_app->m_requestToggleEditing)
    {
        if( g_app->m_atMainMenu ) break;

		g_inputManager->PollForEvents();

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

		g_app->m_script->Advance();
		g_app->m_sepulveda->Advance();
		g_app->m_globalWorld->Advance();
        g_app->m_userInput->Advance();
		g_app->m_camera->Advance();  		
        g_app->m_soundSystem->Advance();

#ifdef ATTRACTMODE_ENABLED
		g_app->m_attractMode->Advance();
#endif
#ifdef PROFILER_ENABLED
		g_app->m_profiler->Advance();
#endif // PROFILER_ENABLED
		
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

        if( g_inputManager->controlEvent( ControlMenuEscape ) )
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
        g_app->m_soundSystem->Advance();
#ifdef PROFILER_ENABLED
		g_app->m_profiler->Advance();
#endif // PROFILER_ENABLED

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
#ifdef TARGET_OS_MACOSX
	// Mac OS X 10.2.8 and I presume earlier require textures to be scaled.
	long version = 0;
	Gestalt(gestaltSystemVersion, &version);
	if (version < 0x1030) 
		g_prefsManager->SetInt("ManuallyScaleTextures", 1);
	DarwiniaReleaseAssert(version >= 0x1020, "Darwinia requires at least Mac OS X version 10.2.x to run");
#endif

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


void InitialiseInputManager()
{
	g_inputManager = new InputManager;
	g_inputManager->addDriver( new ConjoinInputDriver() );
	g_inputManager->addDriver( new ChordInputDriver() );
	g_inputManager->addDriver( new InvertInputDriver() );
	g_inputManager->addDriver( new IdleInputDriver() );
#ifdef TARGET_MSVC
	g_inputManager->addDriver( new W32InputDriver() );
	
	if (LoadLibrary("XINPUT1_3"))
		g_inputManager->addDriver( new XInputDriver() );
#endif
	g_inputManager->addDriver( new PrefsInputDriver() );
	g_inputManager->addDriver( new ValueInputDriver() );
	g_inputManager->addDriver( new AliasInputDriver() );
	{
		// Read Darwinia default input preferences file
		TextReader *inputPrefsReader = g_app->m_resource->GetTextReader( InputPrefs::GetSystemPrefsPath() );
		if ( inputPrefsReader ) {
			DarwiniaReleaseAssert( inputPrefsReader->IsOpen(), "Couldn't open input preferences file: %s\n",
			                       InputPrefs::GetSystemPrefsPath() );
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
		TextReader *userInputPrefsReader = new TextFileReader( InputPrefs::GetUserPrefsPath() );
		if ( userInputPrefsReader ) {
			if ( userInputPrefsReader->IsOpen() ) {
				g_inputManager->parseInputPrefs( *userInputPrefsReader, true );
			}
			delete userInputPrefsReader;
		}

	}
}

bool IsRunningVista()
{
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
}

#if defined(TARGET_OS_VISTA)
void DoVistaChecks()
{
    // Check to make sure the game is running on Vista

	if( !IsRunningVista() )
	{
		return;
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

void Initialise()
{
	//
    // Initialise all our basic objects

    g_systemInfo = new SystemInfo;
    InitialiseHighResTime();

#ifdef TARGET_MSVC
	g_eventHandler = new W32EventHandler();
#endif
	g_app = new App();

    InitialiseInputManager();

#if defined(TARGET_OS_VISTA)
    DoVistaChecks();	
#endif

	g_target = new TargetCursor();
	//if( g_prefsManager->GetInt("ControlMethod")==0 ) getW32EventHandler()->BindAltTab();
    EntityBlueprint::Initialise();
	g_windowManager->HideMousePointer();

	//
	// Start on a specific level if the prefs file tells us to

	char *startMap = g_prefsManager->GetString("StartMap");
	if( startMap && g_app->HasBoughtGame() )
	{
		int requestedLocationId = g_app->m_globalWorld->GetLocationId(startMap);
		GlobalLocation *gloc = g_app->m_globalWorld->GetLocation(requestedLocationId);
		
		if( gloc ) 
		{
			g_app->m_requestedLocationId = requestedLocationId;
			strcpy(g_app->m_requestedMap, gloc->m_mapFilename);
			strcpy(g_app->m_requestedMission, gloc->m_missionFilename);
		}
	}

    g_app->m_renderer->SetOpenGLState();
}


void Finalise()
{
    g_soundLibrary2d->Stop();
	delete g_soundLibrary3d; g_soundLibrary3d = NULL;
	delete g_soundLibrary2d; g_soundLibrary2d = NULL;

    delete g_app->m_resource;
    delete g_windowManager;

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

void RunBootLoaders()
{   
#ifndef DEMOBUILD
	if (g_app->HasBoughtGame() && g_prefsManager->GetInt("CurrentGameMode", 1 ) == 1) {
		char *loaderName = g_prefsManager->GetString("BootLoader", "none");

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

			g_app->m_camera->SetTarget(Vector3(1000,500,1000), Vector3(0,-0.5f,-1));
			g_app->m_camera->CutToTarget();

			g_prefsManager->SetString( "BootLoader", "random" );
			g_prefsManager->Save();
		}
		else
		{
			int loaderType = Loader::GetLoaderIndex(loaderName);
			if( loaderType != -1 )
			{
				Loader *loader = Loader::CreateLoader( loaderType );
				loader->Run();
				delete loader;
			}
		}	

		g_inputManager->Advance();          // clears g_keyDeltas[KEY_ESC]
		g_inputManager->Advance();
	}
#endif
}

void EnterLocation()
{
    bool iAmAServer = g_prefsManager->GetInt("IAmAServer") ? true : false;
    if (!g_app->m_editing)
    {
        if( iAmAServer )
        {
	        g_app->m_server = new Server();
	        g_app->m_server->Initialise();
        }

	    g_app->m_clientToServer->ClientJoin();
    }

    g_app->m_location = new Location();
    g_app->m_locationInput = new LocationInput();
    g_app->m_location->Init( g_app->m_requestedMission, g_app->m_requestedMap );
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
        else
        {
            if( iAmAServer )
            {
                g_app->m_clientToServer->RequestTeam( Team::TeamTypeCPU, -1 );
                g_app->m_clientToServer->RequestTeam( Team::TeamTypeCPU, -1 );
            }
            g_app->m_clientToServer->RequestTeam( Team::TeamTypeLocalPlayer, -1 );
        }
    }

    float const borderSize = 200.0f;
    float minX = -borderSize;
    float maxX = g_app->m_location->m_landscape.GetWorldSizeX() + borderSize;
    g_app->m_camera->SetBounds(minX, maxX, minX, maxX);
    g_app->m_camera->SetTarget(Vector3(maxX,1000,maxX), Vector3(-1,-0.7,-1)); // Incase start doesn't exist
    g_app->m_camera->SetTarget("start");
    g_app->m_camera->CutToTarget();

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

	    LocationGameLoop();
#ifdef DEMOBUILD
#ifndef DEMO2
        PrintMemoryLeaks();
        g_windowManager->OpenWebsite( "http://www.darwinia.co.uk/demoend/" );
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
        //g_app->m_atMainMenu = true;
        g_app->m_requestedLocationId = g_app->m_globalWorld->GetLocationId("launchpad");
        GlobalLocation *gloc = g_app->m_globalWorld->GetLocation(g_app->m_requestedLocationId);
		strcpy(g_app->m_requestedMap, gloc->m_mapFilename);
		strcpy(g_app->m_requestedMission, gloc->m_missionFilename);
    }

    // Put the camera in a sensible place
    g_app->m_camera->SetDebugMode(Camera::DebugModeAuto);
    g_app->m_camera->RequestMode(Camera::ModeSphereWorld);
    g_app->m_camera->SetHeight( 50.0f );
    
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
    g_app->m_camera->RequestMode( Camera::ModeMainMenu );
    while( g_app->m_atMainMenu )
    {
        UpdateAdvanceTime();
        g_app->m_renderer->Render();
        g_app->m_userInput->Advance();
        g_app->m_camera->Advance();  
        g_app->m_soundSystem->Advance();
        HandleCommonConditions();

        if( !g_app->m_gameMenu->m_menuCreated )
        {
            if( g_app->m_renderer->IsFadeComplete() )
            {
                g_app->m_gameMenu->CreateMenu();
            }
        }
    }

    g_app->m_gameMenu->DestroyMenu();
}

void RunTheGame()
{    
	Initialise();
    RunBootLoaders();
    

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

	if( g_prefsManager->GetInt( "CurrentGameMode", 1 ) == 0 )
	{
		g_app->LoadPrologue();
	}
	else
	{
		g_app->LoadCampaign();
	}

    while (true)
    {
        if( g_app->m_demoEndSequence )
        {
            delete g_app->m_demoEndSequence;
            g_app->m_demoEndSequence = NULL;
        }

        if (g_app->m_requestedLocationId != -1)
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

#ifdef TARGET_DEBUG
#include "lib/exception_handler.h"
#endif

// Main Function
void AppMain()
{
#ifdef USE_CRASHREPORTING
    __try
#endif
    {
        RunTheGame();
    }
#ifdef USE_CRASHREPORTING
	__except( RecordExceptionInfo( GetExceptionInformation(), "AppMain", "0A88BB32", DARWINIA_VERSION ) ) {}
#endif
}
