#include "lib/universal_include.h"

#include <stdarg.h>

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/text_stream_readers.h"
#include "lib/language_table.h"
#include "lib/filesys_utils.h"
#include "lib/preferences.h"
#include "lib/window_manager.h"
#include "lib/shape.h"

#include "app.h"
#include "camera.h"
#include "global_world.h"
#include "level_file.h"
#include "location.h"
#include "main.h"
#include "renderer.h"
#include "script.h"
#include "sepulveda.h"
#include "testharness.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "demoendsequence.h"
#include "tutorial.h"
#include "team.h"
#include "unit.h"

#include "loaders/credits_loader.h"

#include "sound/soundsystem.h"

#include "worldobject/constructionyard.h"
#include "worldobject/goddish.h"
#include "worldobject/rocket.h"
#include "worldobject/centipede.h"



//*****************************************************************************
// Private Functions
//*****************************************************************************

#ifdef SCRIPT_TEST_ENABLED
void Script::ReportError(LevelFile const *_levelFile, char *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);

	char location[128];

	if (_levelFile)
	{
		sprintf(location, " (%s %s line: %d)",
			_levelFile->m_mapFilename+12, _levelFile->m_missionFilename+12, m_in->m_lineNum);
	}
	else
	{
		sprintf(location, " (no map, no mission, line: %d)", m_in->m_lineNum);
	}

	strcat(buf, location);

	if (g_app->m_testHarness)
	{
		g_app->m_testHarness->PrintError(buf);
	}
	else
	{
		DarwiniaDebugAssert(false); // Error message is in buf
	}
}
#endif // SCRIPT_TEST_ENABLED


//*****************************************************************************
// Public Functions
//*****************************************************************************

Script::Script()
:   m_in(NULL),
    m_waitUntil(-1.0f),
    m_waitForSpeech(false),
	m_waitForCamera(false),
	m_waitForFade(false),
    m_requestedLocationId(-1),
    m_waitForRocket(false),
    m_permitEscape(false),
    m_waitForPlayerNotBusy(false)
{
}


bool Script::IsRunningScript()
{
	if (g_app->m_testHarness) return false;

    return ( m_in != NULL );
}


void Script::RunCommand_CamCut(char const *_mountName)
{
    if( !g_app->m_location ) return;

	bool mountFound = g_app->m_camera->SetTarget(_mountName);
    DarwiniaDebugAssert( mountFound );
	g_app->m_camera->CutToTarget();
}


void Script::RunCommand_CamMove(char const *_mountName, float _duration)
{
    if( !g_app->m_location ) return;

	if (g_app->m_camera->SetTarget(_mountName))
	{
		g_app->m_camera->SetMoveDuration(_duration);

        g_app->m_camera->RequestMode(Camera::ModeMoveToTarget);
	}
}


void Script::RunCommand_CamAnim(char const *_animName)
{
    if( !g_app->m_location ) return;

	int animId = g_app->m_location->m_levelFile->GetCameraAnimId(_animName);
	DarwiniaReleaseAssert(animId != -1, "Invalid camera animation requested %s", _animName);
	CameraAnimation *camAnim = g_app->m_location->m_levelFile->m_cameraAnimations[animId];
	g_app->m_camera->PlayAnimation(camAnim);
}


void Script::RunCommand_CamFov(float _fov, bool _immediate)
{
    if( _immediate )
    {
        g_app->m_camera->SetFOV( _fov );
    }
    else
    {
        g_app->m_camera->SetTargetFOV( _fov );
    }
}


void Script::RunCommand_CamBuildingFocus(int _buildingId, float _range, float _height)
{
    if( !g_app->m_location ) return;

    Building *building = g_app->m_location->GetBuilding( _buildingId );

    if( building )
    {
        g_app->m_camera->RequestBuildingFocusMode( building, _range, _height );
    }
    else
    {
        DebugOut( "SCRIPT ERROR : Tried to target non-existent building %d", _buildingId );
    }
}


void Script::RunCommand_CamBuildingApproach (int _buildingId, float _range, float _height, float _duration )
{
    if( !g_app->m_location ) return;

    Building *building = g_app->m_location->GetBuilding( _buildingId );

    if( building )
    {
        g_app->m_camera->SetTarget( building->m_centrePos, _range, _height );
        g_app->m_camera->SetMoveDuration( _duration );
        g_app->m_camera->RequestMode( Camera::ModeMoveToTarget );
    }
    else
    {
        DebugOut( "SCRIPT ERROR : Tried to target non-existent building %d", _buildingId );
    }
}


void Script::RunCommand_CamGlobalWorldFocus ()
{
    g_app->m_camera->RequestSphereFocusMode();
}


void Script::RunCommand_LocationFocus(char const *_locationName, float _fov)
{
    if( g_app->m_location ) return;

    Vector3 targetPos;

    if( stricmp( _locationName, "heaven" ) == 0 )
    {
        targetPos = g_zeroVector;
    }
    else
    {
        int locationId = g_app->m_globalWorld->GetLocationId( _locationName );
        if( locationId == -1 ) return;

        targetPos = g_app->m_globalWorld->GetLocationPosition(locationId);
    }

    if( !g_app->m_camera->IsInMode( Camera::ModeSphereWorldScripted ) )
    {
        g_app->m_camera->RequestMode( Camera::ModeSphereWorldScripted );
    }

    g_app->m_camera->SetTargetFOV( _fov );
    g_app->m_camera->SetTarget( targetPos, Vector3(0,0,1), g_upVector );
}


void Script::RunCommand_CamReset()
{
    if( g_app->m_camera->IsAnimPlaying() )
    {
        g_app->m_camera->StopAnimation();
    }

    if( g_app->m_location )
    {
        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
    }
    else
    {
        g_app->m_camera->RequestMode(Camera::ModeSphereWorld);
    }
}


void Script::RunCommand_EnterLocation(char *_name)
{
    g_app->m_requestedLocationId = g_app->m_globalWorld->GetLocationId(_name);

    m_requestedLocationId = g_app->m_requestedLocationId;

    GlobalLocation *loc = g_app->m_globalWorld->GetLocation(g_app->m_requestedLocationId);
    DarwiniaDebugAssert( loc );

    strcpy(g_app->m_requestedMission, loc->m_missionFilename);
    strcpy(g_app->m_requestedMap, loc->m_mapFilename);
}


void Script::RunCommand_ExitLocation()
{
    g_app->m_requestedLocationId = -1;
	g_app->m_requestedMission[0] = '\0';
	g_app->m_requestedMap[0] = '\0';

    m_requestedLocationId = g_app->m_requestedLocationId;
}


void Script::RunCommand_SetMission(char *_locName, char *_missionName )
{
    GlobalLocation *loc = g_app->m_globalWorld->GetLocation(_locName);
    DarwiniaDebugAssert(loc);
    strcpy(loc->m_missionFilename, _missionName);
    loc->m_missionCompleted = false;
}


void Script::RunCommand_Say(char *_stringId)
{
    g_app->m_sepulveda->Say( _stringId );
}


void Script::RunCommand_ShutUp()
{
    g_app->m_sepulveda->ShutUp();
}


void Script::RunCommand_Wait(double _time)
{
    m_waitUntil = max( m_waitUntil,
                       GetHighResTime() + _time );
}


void Script::RunCommand_WaitSay()
{
	m_waitForSpeech = true;
}


void Script::RunCommand_WaitCam()
{
	m_waitForCamera = true;
}


void Script::RunCommand_WaitFade()
{
	m_waitForFade = true;
}


void Script::RunCommand_WaitRocket( int _buildingId, char *_state, int _data )
{
    m_rocketId = _buildingId;
    m_rocketState = EscapeRocket::GetStateId( _state );
    m_rocketData = _data;
    m_waitForRocket = true;
}


void Script::RunCommand_WaitPlayerNotBusy()
{
    Demo2Tutorial::BeginPlayerBusyCheck();
    m_waitForPlayerNotBusy = true;
}


void Script::RunCommand_Highlight(int _buildingId)
{
    g_app->m_sepulveda->HighlightBuilding( _buildingId, "ScriptHighlight" );
}


void Script::RunCommand_ClearHighlights()
{
    g_app->m_sepulveda->ClearHighlights( "ScriptHighlight" );
}


void Script::RunCommand_TriggerSound( char const *_event )
{
    char eventName[256];
    sprintf( eventName, "Music %s", _event );

    if( g_app->m_soundSystem->NumInstancesPlaying( WorldObjectId(), eventName ) == 0 )
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, (char *) _event, SoundSourceBlueprint::TypeMusic );
    }
}


void Script::RunCommand_StopSound(char const *_event)
{
    char eventName[256];
    sprintf( eventName, "Music %s", _event );
    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), eventName );
}


void Script::RunCommand_DemoGesture(char const *_name)
{
    g_app->m_sepulveda->DemoGesture( _name, 2.0f );
}


void Script::RunCommand_GiveResearch(char const *_name)
{
    if( stricmp( _name, "modsystem" ) == 0 )
    {
        g_prefsManager->SetInt( "ModSystemEnabled", 1 );
        g_prefsManager->Save();

        g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageResearch, 999, 4.0f );
    }
    else if( stricmp( _name, "accessallareas" ) == 0 )
    {
        char folderName[512];
        sprintf( folderName, "%susers/", g_app->GetProfileDirectory() );
        bool success = CreateDirectory( folderName );
        if( !success )
        {
            DebugOut( "failed to create folder %s\n", folderName );
        }

        sprintf( folderName, "%susers/AccessAllAreas/", g_app->GetProfileDirectory() );
        success = CreateDirectory( folderName );
        if( !success )
        {
            DebugOut( "failed to create folder %s\n", folderName );
        }

        g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageResearch, 998, 4.0f );
    }
    else
    {
        int researchType = GlobalResearch::GetType( (char *) _name );
        if( researchType != -1 )
        {
            g_app->m_globalWorld->m_research->AddResearch( researchType );
            g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageResearch, researchType, 4.0f );
        }
    }
}


void Script::RunCommand_RunCredits()
{
    int loaderType = Loader::GetLoaderIndex("credits");
    if( loaderType != -1 )
    {
        Loader *loader = Loader::CreateLoader( loaderType );
        loader->Run();
        delete loader;
    }
}


void Script::RunCommand_GameOver()
{
    //
    // Go into the outro camera mode

    g_app->m_camera->RequestMode( Camera::ModeSphereWorldOutro );

    //
    // Kill global world ambiences

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Ambience EnterGlobalWorld" );
}


void Script::RunCommand_ResetResearch()
{
    m_darwinianResearchLevel = g_app->m_globalWorld->m_research->m_researchLevel[GlobalResearch::TypeDarwinian];
    g_app->m_globalWorld->m_research->m_researchLevel[GlobalResearch::TypeDarwinian] = 1;
}


void Script::RunCommand_RestoreResearch()
{
    g_app->m_globalWorld->m_research->m_researchLevel[GlobalResearch::TypeDarwinian] = m_darwinianResearchLevel;
}


GodDish *GetGodDish()
{
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building && building->m_type == Building::TypeGodDish )
            {
                GodDish *dish = (GodDish *) building;
                return dish;
            }
        }
    }

    return NULL;
}


void Script::RunCommand_GodDishActivate()
{
    GodDish *dish = GetGodDish();
    if( dish ) dish->Activate();
}


void Script::RunCommand_GodDishDeactivate()
{
    GodDish *dish = GetGodDish();
    if( dish ) dish->DeActivate();
}


void Script::RunCommand_GodDishSpawnSpam()
{
    GodDish *dish = GetGodDish();
    if( dish ) dish->SpawnSpam(false);
}


void Script::RunCommand_GodDishSpawnResearch()
{
    GodDish *dish = GetGodDish();
    if( dish ) dish->SpawnSpam(true);
}


void Script::RunCommand_SpamTrigger()
{
    GodDish *dish = GetGodDish();
    if( dish ) dish->TriggerSpam();
}


void Script::RunCommand_PurityControl()
{
    //
    // Delete the save game

    char saveDir[256];
    sprintf( saveDir, "users/%s/", g_app->m_userProfileName );
    LList<char *> *allFiles = ListDirectory( saveDir, "*.*" );

    for( int i = 0; i < allFiles->Size(); ++i )
    {
        char *filename = allFiles->GetData(i);
        DeleteThisFile( filename );
    }

    //
    // Open up our store website

    g_windowManager->OpenWebsite( "http://www.darwinia.co.uk/store/" );


    //
    // Shut down

    exit(0);
}


void Script::RunCommand_ShowDarwinLogo()
{
    g_app->m_renderer->m_renderDarwinLogo = GetHighResTime();
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "ShowLogo", SoundSourceBlueprint::TypeInterface );
}


void Script::RunCommand_ShowDemoEndSequence()
{
    g_app->m_demoEndSequence = new DemoEndSequence();
}


void Script::RunCommand_PermitEscape()
{
    m_permitEscape = true;
}


void Script::RunCommand_DestroyBuilding( int _buildingId, float _intensity )
{
	Building *b = g_app->m_location->GetBuilding( _buildingId );
	if( b )
	{
		b->Destroy( _intensity );
	}
}

void Script::RunCommand_ActivateTrunkPort( int _buildingId, bool _fullActivation )
{
	Building *b = g_app->m_location->GetBuilding( _buildingId );
	if( b &&
		b->m_type == Building::TypeTrunkPort )
	{
		if( _fullActivation )
		{
			b->ReprogramComplete();
		}
		else
		{
			GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( b->m_id.GetUniqueId(), g_app->m_locationId );
			gb->m_online = true;
		}
	}
}

void Script::RunCommand_SetTeamColour ( int _teamID, int _red, int _green, int _blue )
{
	if ( _teamID < 0 || _teamID >= NUM_TEAMS ) { return; }

	RGBAColour teamColour = RGBAColour(_red, _green, _blue);
	g_app->m_location->m_teams[_teamID].m_colour = teamColour;
	for ( int i = 0; i < g_app->m_location->m_teams[_teamID].m_units.Size(); i++ )
	{
		if ( g_app->m_location->m_teams[_teamID].m_units.ValidIndex(i) )
		{
			for ( int j = 0; j < g_app->m_location->m_teams[_teamID].m_units[i]->m_entities.Size(); j++ )
			{
				if ( g_app->m_location->m_teams[_teamID].m_units[i]->m_entities.ValidIndex(j) )
				{
					Entity *entity = g_app->m_location->m_teams[_teamID].m_units[i]->m_entities.GetData(j);
					if ( entity->m_shape ) { entity->m_shape->Recolour(teamColour); }
					if ( entity->m_type == Entity::TypeCentipede )
					{
						Centipede *centipede = (Centipede *) entity;
						centipede->s_shapeHead->Recolour(teamColour);
						centipede->s_shapeBody->Recolour(teamColour);
					}
				}
			}
		}
	}
	for ( int j = 0; j < g_app->m_location->m_teams[_teamID].m_others.Size(); j++ )
	{
		if ( g_app->m_location->m_teams[_teamID].m_others.ValidIndex(j) )
		{
			Entity *entity = g_app->m_location->m_teams[_teamID].m_others.GetData(j);
			if ( entity->m_shape ) { entity->m_shape->Recolour(teamColour); }
			if ( entity->m_type == Entity::TypeCentipede )
			{
				Centipede *centipede = (Centipede *) entity;
				centipede->s_shapeHead->Recolour(teamColour);
				centipede->s_shapeBody->Recolour(teamColour);
			}
		}
	}
}

// Opens a script file and returns. The script will only actually be run when
// Script::Advance gets called
void Script::RunScript(char *_filename)
{
    if( strstr( _filename, ".txt" ) )
    {
#ifdef SCRIPT_TEST_ENABLED
        TestScript(_filename);
#endif

        // Run a script, speficied by filename
        char fullFilename[256] = "scripts/";
        strcat(fullFilename, _filename);
        m_in = g_app->m_resource->GetTextReader(fullFilename);
        DarwiniaDebugAssert(m_in);
    }
    else
    {
        // This script is specified as a string id, eg "cutscenealpha"
        // Meaning we want to say all strings like "cutscenealpha_1", "cutscenealpha_2" etc
        // Simply dump all matching strings into Sepulveda's queue
        int stringIndex = 1;
        while( true )
        {
            char stringName[256];
            sprintf( stringName, "%s_%d", _filename, stringIndex );
            if( !ISLANGUAGEPHRASE_ANY(stringName) ) break;

            g_app->m_sepulveda->Say( stringName );
            ++stringIndex;
        }
    }
}

bool Script::Skip()
{
    m_waitUntil = g_gameTime;
    m_waitForCamera = false;
    m_waitForRocket = false;
    m_waitForPlayerNotBusy = false;
    g_app->m_renderer->m_renderDarwinLogo = -1.0f;

    if( m_permitEscape )
    {
        // Quick exit the entire cutscene
        delete m_in;
        m_in = NULL;
        g_app->m_sepulveda->ShutUp();
        g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Music" );
        m_permitEscape = false;
        if( g_app->m_location )
        {
            g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
        }
        else
        {
            g_app->m_camera->RequestMode(Camera::ModeSphereWorld);
        }
        return true;
    }

    return false;
}


void Script::Advance()
{
    if( g_inputManager->controlEvent( ControlSkipCutscene ) )
	{
        if( Skip() ) return;
    }

    if( m_permitEscape ) g_app->m_taskManagerInterface->SetVisible(false);

	if (m_waitForFade && !g_app->m_renderer->IsFadeComplete())          return;
    if (m_waitUntil > g_gameTime)                                       return;
    if (m_waitForSpeech && g_app->m_sepulveda->IsTalking())             return;
    if (m_waitForCamera && g_app->m_camera->IsAnimPlaying())        	return;
    if (m_waitForPlayerNotBusy && Demo2Tutorial::IsPlayerBusy())        return;

    if (m_waitForRocket)
    {
        EscapeRocket *rocket = (EscapeRocket *) g_app->m_location->GetBuilding( m_rocketId );
        if( !rocket || rocket->m_type != Building::TypeEscapeRocket )
        {
            m_waitForRocket = false;
            return;
        }

        if( rocket->m_state < m_rocketState )
        {
            return;
        }

        if( rocket->m_state == m_rocketState &&
            m_rocketState == EscapeRocket::StateCountdown &&
            (int) rocket->m_countdown > m_rocketData )
        {
            return;
        }
    }

    if (m_requestedLocationId != -1)
    {
        if( g_app->m_locationId != m_requestedLocationId )
        {
            return;
        }
        else
        {
            m_requestedLocationId = -1;
        }
    }

    m_waitForSpeech = false;
    m_waitForCamera = false;
    m_waitForFade = false;
    m_waitForRocket = false;
    m_waitForPlayerNotBusy = false;

    if (m_in)
    {
        if (m_in->ReadLine())
        {
			AdvanceScript();
        }
        else
        {
            delete m_in;
            m_in = NULL;
            m_permitEscape = false;
        }
    }
}


void Script::AdvanceScript()
{
	if (!m_in->TokenAvailable()) return;

	int opCode = GetOpCode(m_in->GetNextToken());
	char *nextWord = NULL;
	float nextFloat = 0.0f;
	if (m_in->TokenAvailable())
	{
		nextWord = m_in->GetNextToken();
		nextFloat = atof(nextWord);
	}

	switch (opCode)
	{
		case OpCamMove:
		{
			float duration = atof(m_in->GetNextToken());
			RunCommand_CamMove(nextWord, duration);
			break;
		}
		case OpCamCut:			    RunCommand_CamCut(nextWord);						break;
		case OpCamAnim:			    RunCommand_CamAnim(nextWord);						break;
        case OpCamFov:
        {
            int immediate = m_in->TokenAvailable() ?
                            atoi( m_in->GetNextToken() ) :
                            true;
            RunCommand_CamFov(nextFloat, immediate);
            break;
        }

        case OpCamBuildingFocus:
        {
            float range = atof(m_in->GetNextToken());
            float height = atof(m_in->GetNextToken());
            RunCommand_CamBuildingFocus((int)nextFloat, range, height);
            break;
        }

        case OpCamBuildingApproach:
        {
            float range = atof(m_in->GetNextToken());
            float height = atof(m_in->GetNextToken());
            float duration = atof(m_in->GetNextToken());
            RunCommand_CamBuildingApproach((int)nextFloat, range, height, duration);
            break;
        }

        case OpCamLocationFocus:
        {
            float fov = atof(m_in->GetNextToken());
            RunCommand_LocationFocus(nextWord,fov);
            break;
        }

        case OpCamGlobalWorldFocus:
        {
            RunCommand_CamGlobalWorldFocus();
            break;
        }

        case OpCamReset:            RunCommand_CamReset();                              break;

		case OpEnterLocation:	    RunCommand_EnterLocation(nextWord);		            break;
		case OpExitLocation:	    RunCommand_ExitLocation();							break;

		case OpSay:				    RunCommand_Say(nextWord);							break;
        case OpShutUp:              RunCommand_ShutUp();                                break;
		case OpWait:			    RunCommand_Wait(nextFloat);							break;
		case OpWaitSay:			    RunCommand_WaitSay();								break;
		case OpWaitCam:			    RunCommand_WaitCam();								break;
        case OpWaitFade:            RunCommand_WaitFade();                              break;

        case OpWaitRocket:
        {
            char *state = m_in->GetNextToken();
            int data = atoi( m_in->GetNextToken() );
            RunCommand_WaitRocket( (int)nextFloat, state, data );
            break;
        }

        case OpWaitPlayerNotBusy:   RunCommand_WaitPlayerNotBusy();                     break;

        case OpHighlight:           RunCommand_Highlight((int)nextFloat);               break;
        case OpClearHighlights:     RunCommand_ClearHighlights();                       break;

        case OpTriggerSound:        RunCommand_TriggerSound(nextWord);                  break;
        case OpStopSound:           RunCommand_StopSound(nextWord);                     break;

        case OpDemoGesture:         RunCommand_DemoGesture(nextWord);                   break;
        case OpGiveResearch:        RunCommand_GiveResearch(nextWord);                  break;

        case OpSetMission:
        {
            char *missionName = m_in->GetNextToken();
            RunCommand_SetMission(nextWord, missionName);
            break;
        }

        case OpResetResearch:       RunCommand_ResetResearch();                         break;
        case OpRestoreResearch:     RunCommand_RestoreResearch();                       break;

        case OpGameOver:            RunCommand_GameOver();                              break;
        case OpRunCredits:          RunCommand_RunCredits();                            break;

        case OpSetCutsceneMode:
        {
            int cutsceneMode = atoi( nextWord );
            g_app->m_sepulveda->SetCutsceneMode( cutsceneMode );
            break;
        }

        case OpGodDishActivate:     RunCommand_GodDishActivate();                       break;
        case OpGodDishDeactivate:   RunCommand_GodDishDeactivate();                     break;
        case OpGodDishSpawnSpam:    RunCommand_GodDishSpawnSpam();                      break;
        case OpGodDishSpawnResearch:RunCommand_GodDishSpawnResearch();                  break;
        case OpTriggerSpam:         RunCommand_SpamTrigger();                           break;
        case OpPurityControl:       RunCommand_PurityControl();                         break;

        case OpShowDarwinLogo:      RunCommand_ShowDarwinLogo();                        break;
        case OpShowDemoEndSequence: RunCommand_ShowDemoEndSequence();                   break;

        case OpPermitEscape:        RunCommand_PermitEscape();                          break;

		case OpDestroyBuilding:
		{
			float intensity = atof(m_in->GetNextToken());
			RunCommand_DestroyBuilding( (int)nextFloat, intensity );
			break;
		}

		case OpActivateTrunkPort:
		case OpActivateTrunkPortFull:
		{
			RunCommand_ActivateTrunkPort((int)nextFloat, opCode == OpActivateTrunkPortFull );
			break;
		}
		case OpSetTeamColour:
		{
			int teamID = (int) nextFloat;
			int red = atoi(m_in->GetNextToken());
			int green = atoi(m_in->GetNextToken());
			int blue = atoi(m_in->GetNextToken());

			RunCommand_SetTeamColour(teamID, red, green, blue);
			break;
		}
		default:				    DarwiniaDebugAssert(false);									break;
	}
}


#ifdef SCRIPT_TEST_ENABLED
void Script::TestScript(char *_filename)
{
	char fullFilename[256] = "scripts/";
    strcat(fullFilename, _filename);
    m_in = g_app->m_resource->GetTextReader(fullFilename);
    if (!m_in)
	{
		ReportError(NULL, "Script file not found");
	}

	LevelFile *levelFile = NULL;

	while (m_in->ReadLine())
	{
		if (!m_in->TokenAvailable()) continue;

		char *firstWord = m_in->GetNextToken();
		int opCode = GetOpCode(firstWord);
		char *nextWord = NULL;
		float nextFloat = 0.0f;
		if (m_in->TokenAvailable())
		{
			nextWord = m_in->GetNextToken();
			nextFloat = atof(nextWord);
		}

		switch (opCode)
		{
			case OpCamMove:
			{
				if (!nextWord)
				{
					ReportError(levelFile, "CamMove called with no mount specified");
					break;
				}
				if (!levelFile)
				{
					ReportError(levelFile, "CamMove called when not in a location");
					break;
				}
				if (!levelFile->GetCameraMount(nextWord))
				{
					ReportError(levelFile, "CamMove called with invalid mount name: %s", nextWord);
					break;
				}
				if (!m_in->TokenAvailable())
				{
					ReportError(levelFile, "CamMove called with no duration parameter");
					break;
				}
				float duration = atof(m_in->GetNextToken());
				if (duration < 0.01f || duration > 20.0f)
				{
					ReportError(levelFile, "CamMove called with invalid duration parameter: %f", duration);
					break;
				}
				break;
			}
			case OpCamCut:
				if (!nextWord)
				{
					ReportError(levelFile, "CamCut called with no mount specified");
					break;
				}
				if (!levelFile)
				{
					ReportError(levelFile, "CamCut called when not in a location");
					break;
				}
				if (!levelFile->GetCameraMount(nextWord))
				{
					ReportError(levelFile, "CamCut called with invalid mount name: %s", nextWord);
					break;
				}
				break;
			case OpCamAnim:
				if (!nextWord)
				{
					ReportError(levelFile, "CamAnim called with no anim name specified");
					break;
				}
				if (g_app->m_location->m_levelFile->GetCameraAnimId(nextWord) == -1)
				{
					ReportError(levelFile, "CamAnim called with bad anim name specified");
					break;
				}
				break;
			case OpEnterLocation:
			{
				if (!nextWord)
				{
					ReportError(levelFile, "EnterLocation called with no location name specified");
					break;
				}
				GlobalLocation *globalLoc = g_app->m_globalWorld->GetLocation(nextWord);
				if (!globalLoc)
				{
					ReportError(levelFile, "EnterLocation called with an invalid location name: %s", nextWord);
					break;
				}
				delete levelFile;
				levelFile = new LevelFile(globalLoc->m_missionFilename, globalLoc->m_mapFilename);
				break;
			}
			case OpExitLocation:
				delete levelFile;
				levelFile = NULL;
				break;
			case OpWaitSay:
			case OpWaitCam:
				break;
			case OpSay:
				if (!nextWord)
				{
					ReportError(levelFile, "Say called with no language symbol specified");
					break;
				}
				if (!ISLANGUAGEPHRASE_ANY(nextWord))
				{
					ReportError(levelFile, "Say called with an invalid language symbol: %s", nextWord);
					break;
				}
				break;
			case OpWait:
				if (nextFloat < 0.01f || nextFloat > 20.0f)
				{
					ReportError(levelFile, "Wait called with invalid duration: %f", nextFloat);
					break;
				}
				break;

            case OpHighlight:
                if( !g_app->m_location->GetBuilding( (int) nextFloat ) )
                {
                    ReportError(levelFile, "HighlightBuilding called with invalid buildingId: %d", (int) nextFloat );
                }
                break;

            case OpClearHighlights:
                break;

			default:
				ReportError(levelFile, "Invalid script command: %s", firstWord);
				break;
		}
	}

	delete m_in;
	m_in = NULL;
}
#endif // SCRIPT_TEST_ENABLED


static char *g_opCodeNames[] =
{
	"CamCut",
	"CamMove",
    "CamAnim",
    "CamFov",
    "CamBuildingFocus",
    "CamBuildingApproach",
    "CamLocationFocus",
    "CamGlobalWorldFocus",
    "CamReset",
	"EnterLocation",
	"ExitLocation",
	"Say",
    "ShutUp",
	"Wait",
	"WaitSay",
	"WaitCam",
	"WaitFade",
    "WaitRocket",
    "WaitPlayerNotBusy",
    "Highlight",
    "ClearHighlights",
    "TriggerSound",
    "StopSound",
    "DemoGesture",
    "GiveResearch",
    "SetMission",
    "GameOver",
    "ResetResearch",
    "RestoreResearch",
    "RunCredits",
    "SetCutsceneMode",
    "GodDishActivate",
    "GodDishDeactivate",
    "GodDishSpawnSpam",
    "GodDishSpawnResearch",
    "TriggerSpam",
    "PurityControl",
    "ShowDarwinLogo",
    "ShowDemoEndSequence",
    "PermitEscape",
	"DestroyBuilding",
	"ActivateTrunkPort",
	"ActivateTrunkPortFull",
	"SetTeamColour"
};


int Script::GetOpCode(char const *_word)
{
	DarwiniaDebugAssert(sizeof(g_opCodeNames) / sizeof(char *) == OpNumOps);

	for (unsigned int i = 0; i < OpNumOps; ++i)
	{
		if (stricmp(_word, g_opCodeNames[i]) == 0)
		{
			return i;
		}
	}

	return -1;
}
