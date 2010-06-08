 #include "lib/universal_include.h"

#include <float.h>

#include <eclipse.h>

#include "lib/hi_res_time.h"
#include "lib/matrix34.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/targetcursor.h"
#include "lib/input/input.h"
#include "lib/input/input_types.h"
#include "lib/math/random_number.h"
#include "lib/preferences.h"
#include "lib/debug_render.h"

#include "network/clienttoserver.h"

#include "sound/soundsystem.h"

#include "interface/chatinput_window.h"

#include "worldobject/building.h"
#include "worldobject/switch.h"
#include "worldobject/engineer.h"
#include "worldobject/radardish.h"
#include "worldobject/officer.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/armour.h"
#include "worldobject/darwinian.h"

#include "app.h"
#include "camera.h"
#include "global_world.h"
#include "location.h"
#include "location_input.h"
#include "main.h"
#include "unit.h"
#include "user_input.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "team.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "helpsystem.h"
    #include "sepulveda.h"
#endif
#include "taskmanager_interface.h"
#include "entity_grid.h"
#include "particle_system.h"
#include "multiwiniahelp.h"
#include "level_file.h"
#include "multiwinia.h"
#include "markersystem.h"
#include "gametimer.h"

#ifdef TESTBED_ENABLED
#include <time.h>
#include "lib/string_utils.h"
#endif


extern int g_lastProcessedSequenceId;

LocationInput::LocationInput()
:   m_routeId(-1),
    m_chatToggledThisUpdate(false)
{
}



// *** AdvanceTeleportControl
void LocationInput::AdvanceRadarDishControl(Building *_building)
{
	if( g_inputManager->controlEvent( ControlUnitSelect ) )
    {
        Vector3 rayStart;
        Vector3 rayDir;
        g_app->m_camera->GetClickRay(g_target->X(), g_target->Y(),
									 &rayStart, &rayDir);

        int buildId = g_app->m_location->GetBuildingId(rayStart, rayDir, 255);
        Building *building = g_app->m_location->GetBuilding( buildId );
        if( building && 
            building->m_type == Building::TypeRadarDish &&
            building != _building )
        {
            RadarDish *dish = (RadarDish *) building;
            g_app->m_clientToServer->RequestAimBuilding( g_app->m_globalWorld->m_myTeamId,
                                                         _building->m_id.GetUniqueId(),
                                                         dish->GetStartPoint() );
        }
        else
        {
            g_app->m_clientToServer->RequestAimBuilding( g_app->m_globalWorld->m_myTeamId, 
													     _building->m_id.GetUniqueId(), 
													     g_app->m_userInput->GetMousePos3d() );
        }
    }
}


// *** GetObjectUnderMouse
bool LocationInput::GetObjectUnderMouse( WorldObjectId &_id, int _teamId )
{
    if( _teamId == 255 ) return false;
    
    //
    // If we are controlling a turret, dont snap to anything

    Team *team = g_app->m_location->m_teams[_teamId];
    if( team->m_currentBuildingId != -1 )
    {
        Building *building = g_app->m_location->GetBuilding(team->m_currentBuildingId);
        if( building && building->m_type == Building::TypeGunTurret )
        {
            return false;
        }
    }

    //
    // If we are highlighting a Marker Icon, that takes precedence

    if( g_app->m_markerSystem->m_markerUnderMouse.IsValid() )
    {
        _id = g_app->m_markerSystem->m_markerUnderMouse;
        return true;
    }


    Vector3 rayStart;
    Vector3 rayDir;
    g_app->m_camera->GetClickRay( g_target->X(), g_target->Y(),
								  &rayStart, &rayDir );

    // Find any objects the ray intersects
 	double buildDist = FLT_MAX;
	double unitDist = FLT_MAX;
	double entDist = FLT_MAX;
   
    int buildId = g_app->m_location->GetBuildingId(rayStart, rayDir, _teamId, FLT_MAX, &buildDist );
	int unitId = g_app->m_location->GetUnitId( rayStart, rayDir, _teamId, &unitDist );
    WorldObjectId entId = g_app->m_location->GetEntityId( rayStart, rayDir, _teamId, &entDist, true );
        
    //
    // Make sure the building is a valid selectable type

    Building *building = g_app->m_location->GetBuilding(buildId);
    if( building ) 
    {
        bool selectableType = ( building->m_type == Building::TypeRadarDish ||
                                building->m_type == Building::TypeGunTurret ||
                                building->m_type == Building::TypeFenceSwitch );

        if( !selectableType )
        {
            buildId = -1;
        }
    }


    //
    // Look for Darwinians 
    if( !entId.IsValid() && _teamId != 255 )
    {
	    Task *task = g_app->m_location->m_teams[ _teamId ]->m_taskManager->GetCurrentTask();
        if( task && 
            task->m_state == Task::StateStarted && 
            task->m_type == GlobalResearch::TypeOfficer )        
        {
            Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
            entId = Task::FindDarwinian( mousePos, _teamId );
            entDist = 0.0;
        }
    }

    //
    // Now find which object is nearest
	
    if( entId.IsValid() && entDist < unitDist && entDist < buildDist )
    {
        // Entity is nearest
        _id = entId;
        return true;
    }

    if( unitId != -1 && unitDist < entDist && unitDist < buildDist )
    {
        // Unit is nearest
        _id.Set( g_app->m_globalWorld->m_myTeamId, unitId, -1, -1 );
        return true;
    }

    Building *b = g_app->m_location->GetBuilding( buildId );
    if( buildId != -1 && buildDist < entDist && buildDist < unitDist && 
        ((b && b->m_id.GetTeamId() == _teamId ) ||
        ( _teamId == 255 ) || (b && b->m_id.GetTeamId() == 255) ) )
    {
        // Building is nearest
        _id.Set( g_app->m_globalWorld->m_myTeamId, UNIT_BUILDINGS, -1, buildId );
        return true;
    }

    return false;
}


void LocationInput::SelectObjectUnderMouse( WorldObjectId &id )
{
    Team *team = g_app->m_location->GetMyTeam();
    Task *currentTask = g_app->m_location->GetMyTaskManager()->GetCurrentTask();

    //
    // Under the old control mechanism, we can't select anything if we are already controlling something

    bool somethingSelected = currentTask || 
                            team->GetMyEntity() || 
                            team->GetMyUnit() ||
                            team->m_numDarwiniansSelected > 0;

    bool cantSelectBecauseSomethingAlreadySelected = false;

    if( g_prefsManager->GetInt( "ControlRemap" ) == 0 )
    {
        if( somethingSelected )
        {
            cantSelectBecauseSomethingAlreadySelected = true;
        }
    }

    //
    // We can't select something new if we are controlling a radar dish or gun turret

    if( team->m_currentBuildingId != -1 )
    {
        Building *building = g_app->m_location->GetBuilding( team->m_currentBuildingId );
        if( building )
        {
            if( building->m_type == Building::TypeRadarDish ||
                building->m_type == Building::TypeGunTurret )
            {
                cantSelectBecauseSomethingAlreadySelected = true;
            }
        }
    }


    if( !cantSelectBecauseSomethingAlreadySelected )
    {
        bool doneIt = false;

        if( id.IsValid() )
        {
            if( id.GetUnitId() == UNIT_BUILDINGS )
            {            
                Building *building = g_app->m_location->GetBuilding(id.GetUniqueId());

                if( building && building->m_id.GetTeamId() == team->m_teamId )
                {
                    if( building->m_type == Building::TypeRadarDish )
                    {
                        if( g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionNoRadarControl] != 1 )
                        {
                            g_app->m_clientToServer->RequestSelectUnit( id.GetTeamId(), -1, -1, id.GetUniqueId() );
                            g_app->m_camera->RequestRadarAimMode( building );
                            doneIt = true;
                        }
                    }
                    else if( building->m_type == Building::TypeGunTurret ||
                             building->m_type == Building::TypeChessPiece )
                    {
                        if( building->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
                        {
                            g_app->m_clientToServer->RequestSelectUnit( id.GetTeamId(), -1, -1, id.GetUniqueId() );
                            g_app->m_camera->RequestTurretAimMode( building );
                            doneIt = true;
                        }
                    }
                    else if( building->m_type == Building::TypeFenceSwitch )
                    {
                        FenceSwitch *fs = (FenceSwitch *)building;
                        fs->Switch();
                        doneIt = true;
                    }
                }            
            }
            else
            {
                Entity *entity = g_app->m_location->GetEntity( id );
                Unit* unit = g_app->m_location->GetUnit( id );

                if( (entity && 
					 entity->m_type != Entity::TypeDarwinian &&
					 entity->IsSelectable() &&
					 entity != team->GetMyEntity()) ||
					(unit &&
					 unit->IsSelectable() &&
					 unit != team->GetMyUnit()) )
                {
                    g_app->m_clientToServer->RequestSelectUnit( id.GetTeamId(), id.GetUnitId(), id.GetIndex(), -1 );
                    doneIt = true;
                }
            }

            if( doneIt )
            {
                static double s_lastSelectTime = 0.0f;
                double timeNow = GetHighResTime();

                if( timeNow > s_lastSelectTime + 0.5f )
                {
                    g_app->m_soundSystem->TriggerOtherEvent( NULL, "SelectUnit", SoundSourceBlueprint::TypeMultiwiniaInterface );
                    s_lastSelectTime = timeNow;
                }
            }
        }
        else
        {
            // Clicked on nothing
            // This can be a move order (under Darwinia controls) or an error (under RTS controls)
            
            int mwControlMethod = g_prefsManager->GetInt( "MultiwiniaControlMethod", 1 );
            if( mwControlMethod == 1 )
            {
				Entity* myEntity = team->GetMyEntity();
                bool placingTask = ( currentTask && currentTask->m_state == Task::StateStarted );
                bool controllingSquad = ( team->GetMyUnit() && team->GetMyUnit()->m_troopType == Entity::TypeInsertionSquadie );
				bool gamepadMove = ( INPUT_MODE_GAMEPAD == g_inputManager->getInputMode() &&
									myEntity &&
									(myEntity->m_type == Entity::TypeOfficer || 
									 myEntity->m_type == Entity::TypeArmour) );
				bool foo = g_inputManager->controlEvent(ControlUnitSetTarget);


                if( somethingSelected && !placingTask && !controllingSquad && !gamepadMove && !foo)
                {
                    g_app->m_soundSystem->TriggerOtherEvent( NULL, "MouseError", SoundSourceBlueprint::TypeMultiwiniaInterface );
                    g_app->m_multiwiniaHelp->ShowSpaceDeselectHelp();
                }
            }
        }
    }
}


// *** AdvanceNoSelection
void LocationInput::AdvanceNoSelection()
{
    if( !g_app->m_taskManagerInterface->IsVisible() )
    {
	    if(g_inputManager->controlEvent( ControlSelectLocation ) )
        {
            WorldObjectId id;
            bool objectFound = GetObjectUnderMouse( id, g_app->m_globalWorld->m_myTeamId );

            SelectObjectUnderMouse( id );        
        }

        //
        // Pressing C to terminate object under mouse

        if( g_app->m_taskManagerInterface->ControlEvent( TaskManagerInterface::TMTerminate ) )
        {
            WorldObjectId id;
            bool objectFound = GetObjectUnderMouse( id, g_app->m_globalWorld->m_myTeamId );

            if( objectFound )
            {
                Entity *entity = g_app->m_location->GetEntity( id );
                if( entity && entity->m_type != Entity::TypeDarwinian )
                {
                    g_app->m_clientToServer->RequestTerminateEntity( id.GetTeamId(), id.GetIndex() );
                    g_app->m_clientToServer->RequestSelectUnit( id.GetTeamId(), -1, -1, -1 );
                }
            }
	    }
    }
}


int CountSelectedDarwiniansCarryingWallBuster()
{
    Team *team = g_app->m_location->GetMyTeam();

    int count = 0;

    for( int i = 0; i < team->m_others.Size(); ++i )
    {
        if( team->m_others.ValidIndex(i) )
        {
            Entity *entity = team->m_others[i];

            if( entity && 
                entity->m_type == Entity::TypeDarwinian )
            {
                Darwinian *darwinian = (Darwinian *)entity;

                if( darwinian->m_underPlayerControl &&
                    darwinian->m_state == Darwinian::StateCarryingBuilding )
                {
                    Building *building = g_app->m_location->GetBuilding( darwinian->m_buildingId );
                    if( building &&
                        building->m_type == Building::TypeWallBuster )
                    {
                        ++count;
                    }
                }
            }
        }
    }

    return count;
}


// *** AdvanceTeamControl
void LocationInput::AdvanceTeamControl()
{    
    Team *team = g_app->m_location->GetMyTeam();
    bool taskStarted = false;
    if( team->m_taskManager )
    {
        Task *task = team->m_taskManager->GetCurrentTask();
        taskStarted = task && task->m_state == Task::StateStarted;

        if( team->m_taskManager->m_mostRecentTaskId != -1 &&
            g_prefsManager->GetInt( "HelpEnabled", 1 ) )
        {
            // We are looking at info on the most recently received task
            // So dont do anything
            //return;
        }
    }

	// Using quick run units
	
	if( !taskStarted &&
		!g_app->m_taskManagerInterface->IsVisible() &&
		!g_app->m_camera->IsInMode(Camera::ModeEntityTrack) &&
		!g_app->m_location->GetMyTeam()->GetMyEntity() && 
		!g_app->m_location->GetMyTeam()->GetMyUnit() &&
		g_app->IsSinglePlayer() )
	{
		if( g_inputManager->controlEvent(ControlIconsTaskManagerQuickUnitLeft) && g_app->m_globalWorld->m_research->HasResearch(GlobalResearch::TypeEngineer) )
		{
			g_app->m_clientToServer->RequestRunProgram(g_app->m_globalWorld->m_myTeamId, GlobalResearch::TypeEngineer);
		}
		else if( g_inputManager->controlEvent(ControlIconsTaskManagerQuickUnitRight) && g_app->m_globalWorld->m_research->HasResearch(GlobalResearch::TypeArmour) )
		{
			g_app->m_clientToServer->RequestRunProgram(g_app->m_globalWorld->m_myTeamId, GlobalResearch::TypeArmour);
		}
		else if( g_inputManager->controlEvent(ControlIconsTaskManagerQuickUnitUp) && g_app->m_globalWorld->m_research->HasResearch(GlobalResearch::TypeSquad) )
		{
			g_app->m_clientToServer->RequestRunProgram(g_app->m_globalWorld->m_myTeamId, GlobalResearch::TypeSquad);
		}
	}

    WorldObjectId idUnderMouse;
    bool objectUnderMouse = GetObjectUnderMouse( idUnderMouse, g_app->m_globalWorld->m_myTeamId );
    Entity *entityUnderMouse = g_app->m_location->GetEntity( idUnderMouse );

    bool officerUndermouse = ( entityUnderMouse && entityUnderMouse->m_type == Entity::TypeOfficer );

    bool controlUnitSelect = g_inputManager->controlEvent( ControlUnitSelect );
    bool controlUnitToggle = g_inputManager->controlEvent( ControlDarwinianPromote );

	bool increasingSelectionCircle = false;


    //
    // Have we clicked to select something?

    if ( controlUnitSelect && 
		!g_app->m_taskManagerInterface->IsVisible() )
	{
        SelectObjectUnderMouse( idUnderMouse );     
    }


    //
    // Are we right clicking on a unit?
    // This should toggle orders

    if( controlUnitToggle &&
        !g_app->m_taskManagerInterface->IsVisible() &&
        !taskStarted &&
        team->m_numDarwiniansSelected == 0 && 
        !g_app->m_multiwinia->GameInGracePeriod() && 
        !g_app->m_multiwinia->GameOver() && 
        !g_app->GamePaused() )
    {
        if( entityUnderMouse )
        {
            bool allowToggle = team->m_currentUnitId == -1 &&
                               team->m_currentEntityId == -1 &&
                               team->m_currentBuildingId == -1;

            // Also allow us to toggle the state of our selected unit
            if( team->m_currentEntityId == entityUnderMouse->m_id.GetIndex() )
            {
                allowToggle = true;
            }

            if( allowToggle )
            {
               if( entityUnderMouse->m_type == Entity::TypeArmour )
               {
                   bool togglingConversionOrders = ( g_app->m_markerSystem->m_isOrderMarkerUnderMouse );
                   
                   g_app->m_clientToServer->RequestToggleEntity( team->m_teamId, entityUnderMouse->m_id.GetIndex(), togglingConversionOrders );
                   g_app->m_soundSystem->TriggerOtherEvent( NULL, "ToggleUnitState", SoundSourceBlueprint::TypeMultiwiniaInterface );

                   if( togglingConversionOrders && 
                       team->GetMyEntity() == entityUnderMouse )
                   {
                       // Right clicking on the movement marker for armour also deselects the armour
                       g_app->m_clientToServer->RequestSelectUnit( team->m_teamId, -1, -1, -1 );
                   }

                   return;
               }
               else if( entityUnderMouse->m_type == Entity::TypeOfficer )
               {
                    g_app->m_clientToServer->RequestToggleEntity( team->m_teamId, entityUnderMouse->m_id.GetIndex(), false );
                    g_app->m_soundSystem->TriggerOtherEvent( NULL, "ToggleUnitState", SoundSourceBlueprint::TypeMultiwiniaInterface );
                    return;
               }            
            }
        }
    }


    //
    // Click on a darwinian = promote to officer
    
    bool controlDarwinianPromote = g_inputManager->controlEvent( ControlDarwinianPromote );

    if( controlDarwinianPromote && !taskStarted &&
        ((	team->m_numDarwiniansSelected == 0 && !g_app->m_multiwinia->GameInGracePeriod() && 
			!g_app->m_multiwinia->GameOver() && !g_app->GamePaused() ) ||
			g_app->IsSinglePlayer()))
	{        
        if( team->m_currentUnitId == -1 &&
            team->m_currentEntityId == -1 &&
            team->m_currentBuildingId == -1 )
        {
            Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
            WorldObjectId id = Task::FindDarwinian( mousePos, g_app->m_globalWorld->m_myTeamId );
            Darwinian *darwinian = (Darwinian *)g_app->m_location->GetEntitySafe( id, Entity::TypeDarwinian );
            if( darwinian && darwinian->IsSelectable() )
            {
				g_app->m_clientToServer->RequestPromoteDarwinian( id.GetTeamId(), id.GetIndex() );

                if( id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
                {
                    g_app->m_multiwiniaHelp->NotifyMadeOfficer();
                }
                return;
            }
        }
        else
        {
            //WorldObjectId id;
            //bool objectUnderMouse = GetObjectUnderMouse( id, g_app->m_globalWorld->m_myTeamId );
            //Entity *entity = g_app->m_location->GetEntity( id );
            //if( entity &&
            //    entity->m_type == Entity::TypeOfficer &&
            //    id.GetIndex() == team->m_currentEntityId )
            //{                
            //    // Clicked on an officer that is already selected - demote
            //    g_app->m_clientToServer->RequestPromoteDarwinian( id.GetTeamId(), id.GetIndex() );
            //    return;
            //}            
        }
    }
      

    //
    // Handle darwinian selection circle

    bool circleBlocked = false;
    if( idUnderMouse.GetUnitId() == UNIT_BUILDINGS )
    {
        Building *b = g_app->m_location->GetBuilding( idUnderMouse.GetUniqueId() );
        if( b )
        {
            circleBlocked = true;
        }
    }
    else
    {
        if( idUnderMouse.GetTeamId() == team->m_teamId )
        {
            if( idUnderMouse.GetUnitId() == -1 )
            {
                Entity *e = g_app->m_location->GetEntity( idUnderMouse );
                if( e )
                {
                    if( e->m_type != Entity::TypeDarwinian )
                    {
                        circleBlocked = true;
                    }
                }
            }
            else
            {
                circleBlocked = true;
            }
        }
    }


    Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
    Vector3 highlightedObjectPos = mousePos;

    //
    // Allow certain buildings and objects to override our mouse pos

    {
        if( idUnderMouse.GetUnitId() == UNIT_BUILDINGS )
        {
            Building *building = g_app->m_location->GetBuilding( idUnderMouse.GetUniqueId() );
            if( building )
            {
                if( building->m_type == Building::TypeRadarDish )
                {
                    Vector3 front;
                    ((RadarDish *)building)->GetEntrance( highlightedObjectPos, front );
                }
                else if( building->m_type == Building::TypeCrate )
                {
                    highlightedObjectPos = building->m_pos;
                }
            }
        }
        else 
        {
            Entity *entity = g_app->m_location->GetEntity( idUnderMouse );
            if( entity )
            {
                if( entity->m_type == Entity::TypeArmour &&
                    ((Armour *)entity)->GetNumPassengers() < ((Armour *)entity)->Capacity() )
                {
                    highlightedObjectPos = entity->m_pos;
                }
                else if( entity->m_type == Entity::TypeOfficer )
                {
                    highlightedObjectPos = entity->m_pos;
                }
            }
        }
    }


   
	if( !g_app->IsSinglePlayer() &&
		!g_app->m_taskManagerInterface->IsVisible() &&
        !taskStarted &&
        team->m_currentUnitId == -1 &&
        team->m_currentEntityId == -1 &&
        team->m_currentBuildingId == -1 )
    {
        if( team->m_numDarwiniansSelected > 0 &&
            team->m_selectionCircleCentre == g_zeroVector )
        {
            // Some darwinians are selected, but we're not expanding the selection circle
            // They are ready for our orders

            if( g_inputManager->controlEvent( ControlIssueDarwinianOrders ) )             
            {
                double timeNow = GetNetworkTime();
                if( timeNow > team->m_lastOrdersSet + 1.0 )
                {
                    team->m_lastOrdersSet = timeNow;
                    bool shiftPressed = g_inputManager->controlEvent( ControlOrderDirectRoute );                   
                    bool directRoute = shiftPressed;

                    if( CountSelectedDarwiniansCarryingWallBuster() >= team->m_numDarwiniansSelected * 0.25f )
                    {
                        // Special case : most of our selected darwinians are controlling a wallbuster
                        // So we go in a straight line
                        directRoute = true;
                    }

                    Vector3 pos = highlightedObjectPos;
                    if( g_app->m_location->m_landscape.IsInLandscape(pos) )
                    {
                        g_app->m_clientToServer->RequestIssueDarwinanOrders( g_app->m_globalWorld->m_myTeamId, pos, directRoute );
                        g_app->m_soundSystem->TriggerOtherEvent( NULL, "SetOrderGoto", SoundSourceBlueprint::TypeInterface );
                    }
                }
            }
        }
		else
        {
            if( g_inputManager->controlEvent( ControlBeginSelectionCircle ) &&
                team->m_selectionCircleCentre == g_zeroVector &&
                !officerUndermouse &&
                !g_app->m_multiwinia->GameInGracePeriod() &&
                !circleBlocked )
            {
                // Begin new selection circle
				increasingSelectionCircle = true;
                team->m_selectionCircleCentre = g_app->m_userInput->GetMousePos3d();
                team->m_selectionCircleRadius = 0.0;                   
                g_app->m_soundSystem->TriggerOtherEvent( NULL, "SelectionCircle", SoundSourceBlueprint::TypeMultiwiniaInterface );
            }

			static bool stoppedLastTime = false;

            if( g_inputManager->controlEvent( ControlEnlargeSelectionCircle ) )
            {
                // Enlarge selection circle
				increasingSelectionCircle = true;
                team->m_selectionCircleRadius += g_advanceTime * 100;
                if( team->m_selectionCircleRadius > 100 ) team->m_selectionCircleRadius = 100;

                double timeNow = GetNetworkTime();
                if( timeNow > team->m_lastOrdersSet )
                {
                    team->m_lastOrdersSet = timeNow;
                    g_app->m_clientToServer->RequestSelectDarwinians( g_app->m_globalWorld->m_myTeamId, team->m_selectionCircleCentre, team->m_selectionCircleRadius );

                    if( team->m_numDarwiniansSelected > 0 &&
                        team->m_teamId == g_app->m_globalWorld->m_myTeamId )
                    {
                        g_app->m_multiwiniaHelp->NotifyGroupSelectedDarwinians();                    
                    }
                }

				stoppedLastTime = false;
            }
            else
            {
                // Stop enlarging selection circle
				if( !stoppedLastTime )
				{
					g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "MultiwiniaInterface SelectionCircle" );
					stoppedLastTime = true;
				}
            }
        }
    }
    
    //
    // Special case for xbox controller:
    // Hitting the right trigger = issue ordes when darwinians are selected
    // (to mirror similar GOTO orders when controlling an officer)

    if( g_inputManager->controlEvent( ControlUnitSecondaryFireDirected ) )
    {
        if( !g_app->m_taskManagerInterface->IsVisible() &&
            !taskStarted &&
            team->m_numDarwiniansSelected > 0 &&
            team->m_selectionCircleCentre == g_zeroVector )
        {
            // Issue orders to selected darwinians
            double timeNow = GetNetworkTime();
            if( timeNow > team->m_lastOrdersSet + 1.0 )
            {
                team->m_lastOrdersSet = timeNow;
                g_app->m_clientToServer->RequestIssueDarwinanOrders( g_app->m_globalWorld->m_myTeamId, highlightedObjectPos, false );
            }    
        }
    }


    if( g_inputManager->controlEvent( ControlEndSelectionCircle ) )
    {
        team->m_selectionCircleCentre.Zero();
        team->m_lastOrdersSet = 0.0;
    }
 


    // Space key should deselect current unit or building
    bool objectSelected = team->m_currentUnitId != -1 ||
                          team->m_currentEntityId != -1 ||
                          team->m_currentBuildingId != -1 ||
                          team->m_numDarwiniansSelected > 0;

    if ( g_inputManager->controlEvent( ControlUnitDeselect ) )
    {
        //if( objectSelected )
        {
            g_app->m_clientToServer->RequestSelectUnit( team->m_teamId, -1, -1, -1 );
            g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
            g_app->m_location->GetMyTaskManager()->m_previousTaskId = -1;

            if( team->m_currentUnitId != -1 )
            {
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
                g_app->m_helpSystem->PlayerDoneAction( HelpSystem::SquadDeselect );
#endif
            }
            else if( team->m_currentEntityId != -1 )
            {
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
                g_app->m_helpSystem->PlayerDoneAction( HelpSystem::EngineerDeselect );
#endif
            }
        }                
    }
    
    if( taskStarted && !g_app->m_taskManagerInterface->IsVisible() )
    {
		if( g_app->IsSinglePlayer() ||
			(!g_app->m_multiwinia->GameInGracePeriod() &&
             !g_app->m_multiwinia->GameOver() &&
             !g_app->GamePaused()) )
        {
            if( g_inputManager->controlEvent( ControlUnitCreate ) )
            {
                Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
                g_app->m_clientToServer->RequestTargetProgram( g_app->m_globalWorld->m_myTeamId,
                                                               g_app->m_location->GetMyTaskManager()->m_currentTaskId,
                                                               mousePos );
            }

            if( g_inputManager->controlEvent( ControlIssueSpecialOrders ) &&
				!increasingSelectionCircle )
            {
				Task* currentTask = g_app->m_location->GetMyTaskManager()->GetCurrentTask();
				bool placingGamepadTask = currentTask && currentTask->m_state == Task::StateStarted && INPUT_MODE_GAMEPAD == g_inputManager->getInputMode();
                // Right clicked while trying to place a unit
                // Probably thought they were playing an ordinary rts, where right click 
                // usually = cancel in this situation

				if( !placingGamepadTask )
				{
					g_app->m_soundSystem->TriggerOtherEvent( NULL, "MouseError", SoundSourceBlueprint::TypeMultiwiniaInterface );
					g_app->m_multiwiniaHelp->ShowSpaceDeselectHelp(true, false);
				}
            }
        }
    }

    if( m_routeId != -1 )
    {
        g_app->m_location->m_routingSystem.DeleteRoute( m_routeId );
        m_routeId = -1;
    }

	if( team->m_currentUnitId == -1 )
    {
        if( team->m_currentEntityId == -1 )
		{
		    if (team->m_currentBuildingId == -1)
            {
                if( team->m_numDarwiniansSelected == 0 )
                {
			        AdvanceNoSelection();
                }
                else
                {
                    bool shiftPressed = g_inputManager->controlEvent( ControlOrderDirectRoute );
                    bool directRoute = shiftPressed;
                    
                    if( CountSelectedDarwiniansCarryingWallBuster() >= team->m_numDarwiniansSelected * 0.25f )
                    {
                        // Special case : most of our selected darwinians are controlling a wallbuster
                        // So we go in a straight line
                        directRoute = true;
                    }

                    m_routeId = g_app->m_location->m_routingSystem.GenerateRoute( team->m_selectedDarwinianCentre, highlightedObjectPos, directRoute );
					AppDebugOut("Generated route - %d\n", m_routeId);
                    g_app->m_location->m_routingSystem.SetRouteIntensity( m_routeId, 1.0 );
                }
            }
		    else
		    {
                if( !team->GetMyEntity() && !team->GetMyUnit() )
                {
			        Building *building = g_app->m_location->GetBuilding(team->m_currentBuildingId);
			        if (!building)
                    {
                        team->m_currentBuildingId = -1;
                    }
                    else if (building->m_type == Building::TypeRadarDish)
			        {
				        AdvanceRadarDishControl(building);
			        }
                    else if( building->m_type == Building::TypeGunTurret)
                    {
					    if( g_app->m_taskManagerInterface->ControlEvent( TaskManagerInterface::TMTerminate ) )
                        {
                            // Player pressed CTRL-C, so terminate this turret
                            g_app->m_clientToServer->RequestSelectUnit( team->m_teamId, -1, -1, -1 );
                            g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
                            //building->Damage( -100 );
                        }

                        if( building->m_id.GetTeamId() != team->m_teamId )
                        {
                            // we're controlling this building but we've lost control of it, so let go
                            g_app->m_clientToServer->RequestSelectUnit( team->m_teamId, -1, -1, -1 );
			                g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
                        }
                    }
                }
		    }
		}
        else
		{
            Entity *ent = team->GetMyEntity();
            if( !ent )
            {
                team->m_currentEntityId = -1;
            }
            else if( ent->m_type == Entity::TypeOfficer )
			{
                if( g_app->m_taskManagerInterface->ControlEvent( TaskManagerInterface::TMTerminate ) )
                {
                    // Player pressed CTRL-C, so demote this officer
                    g_app->m_clientToServer->RequestTerminateEntity( team->m_teamId, team->m_currentEntityId );
                    g_app->m_clientToServer->RequestSelectUnit( team->m_teamId, -1, -1, -1 );
                    g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
                }

                Officer *officer = (Officer *)ent;
				if( g_app->IsSinglePlayer() )
				{
					if( g_inputManager->controlEvent( ControlIconsTaskManagerQuickUnitLeft ) )
					{
						g_app->m_clientToServer->RequestOfficerFollow( team->m_teamId );
					}
					else if( g_inputManager->controlEvent( ControlIconsTaskManagerQuickUnitDown ) )
					{
						g_app->m_clientToServer->RequestOfficerNone( team->m_teamId );
					}
					else if( g_inputManager->controlEvent( ControlIssueSpecialOrders ) )
					{
						bool follow = (g_app->m_userInput->GetMousePos3d()-officer->m_pos).MagSquared() < 2500;

						if( follow )
						{
							g_app->m_clientToServer->RequestOfficerFollow( team->m_teamId );
						}
						else
						{
							Vector3 userMousePos = g_app->m_userInput->GetMousePos3d();
							bool shiftPressed = g_inputManager->controlEvent( ControlOrderDirectRoute );
							m_routeId = g_app->m_location->m_routingSystem.GenerateRoute( ent->m_pos, userMousePos, shiftPressed );
							g_app->m_location->m_routingSystem.SetRouteIntensity( m_routeId, 1.0f );

							WorldObjectId idUnderMouse;
							bool objectUnderMouse = g_app->m_locationInput->GetObjectUnderMouse( idUnderMouse, 255 );                    

							if( idUnderMouse.GetUnitId() == UNIT_BUILDINGS )
							{
								// Focus the mouse on a Radar Dish if one exists under the mouse
								Building *building = g_app->m_location->GetBuilding( idUnderMouse.GetUniqueId() );
								if( building && building->m_type == Building::TypeRadarDish )
								{
									userMousePos = building->m_pos;
								}
							}

							g_app->m_clientToServer->RequestOfficerOrders( team->m_teamId, userMousePos, shiftPressed, false );
						}
					}
				}
				else
                {
                    Vector3 userMousePos = highlightedObjectPos;
					bool directroute = g_inputManager->controlEvent( ControlOrderDirectRoute );
					bool specialMove = g_inputManager->controlEvent( ControlOrderSpecialMove );
                    if( directroute || officer->IsInFormationMode() )
                    {
                        m_routeId = g_app->m_location->m_routingSystem.GenerateRoute( ent->m_pos, userMousePos, true );
                    }
                    else
                    {
                        m_routeId = g_app->m_location->m_routingSystem.GenerateRoute( ent->m_pos, userMousePos, false );
                    }
					g_app->m_location->m_routingSystem.SetRouteIntensity( m_routeId, 1.0f );

                    int mwControlMethod = g_prefsManager->GetInt( "MultiwiniaControlMethod", 1 );

                    if( mwControlMethod == 0 && g_inputManager->controlEvent( ControlUnitSetTarget ) )
                    {
                        g_app->m_clientToServer->RequestOfficerOrders( team->m_teamId, userMousePos, directroute, true );
                    }

					if( g_inputManager->controlEvent( ControlIssueSpecialOrders ) ||
						(g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD && specialMove) )
					{
                        if( mwControlMethod == 0 && officer->IsInFormationMode() )
                        {   
                            // In the old scheme, right click = set goto even in formation
                            g_app->m_clientToServer->RequestOfficerNone( team->m_teamId );
                        }

						if( INPUT_MODE_GAMEPAD == g_inputManager->getInputMode() )
						{
							specialMove = !specialMove;
						}

						g_app->m_clientToServer->RequestOfficerOrders( team->m_teamId, userMousePos, directroute, specialMove );                        
					}
                }
			}
			else if( ent->m_type == Entity::TypeArmour )
            {
				Armour* armour = (Armour*)ent;
                
                if( g_app->Multiplayer() )
                {
                    if( g_app->m_location->m_landscape.IsInLandscape(highlightedObjectPos))
                    {
                        m_routeId = g_app->m_location->m_routingSystem.GenerateRoute( ent->m_pos, highlightedObjectPos, true );
                        g_app->m_location->m_routingSystem.SetRouteIntensity( m_routeId, 1.0f );

                        int mwControlMethod = g_prefsManager->GetInt( "MultiwiniaControlMethod", 1 );
                        
                        // Right click : issue special order eg deploy

					    bool specialMove = g_inputManager->controlEvent( ControlOrderSpecialMove );
                        if( g_inputManager->controlEvent( ControlIssueSpecialOrders ) ||
						    (g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD && specialMove)  )
                        {
                            bool deploy = specialMove;
                            if( mwControlMethod == 0 ) deploy = !specialMove;

                            g_app->m_clientToServer->RequestArmourOrders( team->m_teamId, highlightedObjectPos, deploy );

                            g_app->m_location->m_routingSystem.HighlightRoute( m_routeId, team->m_teamId );

                            if( deploy ) g_app->m_clientToServer->RequestSelectUnit( team->m_teamId, -1, -1, -1 );
                            g_app->m_soundSystem->TriggerEntityEvent( armour, "SetOrders" );
                        }

                        // Left click in darwinian-style control : move here

                        if( g_inputManager->controlEvent( ControlUnitSetTarget ) && mwControlMethod == 0 )
                        {
                            g_app->m_clientToServer->RequestArmourOrders( team->m_teamId, highlightedObjectPos, false );

                            g_app->m_location->m_routingSystem.HighlightRoute( m_routeId, team->m_teamId );
                        }
                    }

                }
                else
                {
			        if( g_inputManager->controlEvent( ControlIssueSpecialOrders ) )
			        {
				        armour->m_awaitingDeployment = true;
			        }
			        else if( armour->m_awaitingDeployment && g_inputManager->controlEvent( ControlUnitCreate ) )
			        {
				        armour->SetOrders(armour->m_pos);
			        }
                    /*if( g_inputManager->controlEvent( ControlWeaponCycleLeft ) ||
                        g_inputManager->controlEvent( ControlWeaponCycleRight ) )
                    {
                        //Armour *armour = (Armour *)ent;
                        //armour->SetDirectOrders();
                        g_app->m_clientToServer->RequestNextWeapon( team->m_teamId );
                    }*/
                }
            }
            else
            {
                if( g_app->Multiplayer() )
                {
                    // Always render a direct route from the entity to the mouse

                    Vector3 userMousePos = highlightedObjectPos;
                    m_routeId = g_app->m_location->m_routingSystem.GenerateRoute( ent->m_pos, userMousePos, true );
                    g_app->m_location->m_routingSystem.SetRouteIntensity( m_routeId, 1.0f );
                }
            }
        }
    }
	else
	{
		// Controlling a unit
        if( team->m_units.ValidIndex(team->m_currentUnitId) )
        {
		    /*Unit *unit = team->m_units.GetData(team->m_currentUnitId);
            if( unit->m_troopType == Entity::TypeInsertionSquadie )
            {
                if( !g_app->m_taskManagerInterface->IsVisible() )
                    {
                    InsertionSquad *squad = (InsertionSquad *)unit;
                    int currentWeapon = -1;
                    LList<int> weaponList;
                    if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeGrenade ) ) 
                    {
                        if( squad->m_weaponType == GlobalResearch::TypeGrenade ) currentWeapon = weaponList.Size();
                        weaponList.PutData( GlobalResearch::TypeGrenade );
                    }
                    if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeRocket ) ) 
                    {
                        if( squad->m_weaponType == GlobalResearch::TypeRocket ) currentWeapon = weaponList.Size();
                        weaponList.PutData( GlobalResearch::TypeRocket );
                    }

                    if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeAirStrike ) ) 
                    {
                        if( squad->m_weaponType == GlobalResearch::TypeAirStrike ) currentWeapon = weaponList.Size();
                        weaponList.PutData( GlobalResearch::TypeAirStrike );
                    }
                    if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeController ) ) 
                    {
                        if( squad->m_weaponType == GlobalResearch::TypeController ) currentWeapon = weaponList.Size();
                        weaponList.PutData( GlobalResearch::TypeController );
                    }

                    if( weaponList.Size() > 1 )
                    {
                        int oldWeapon = currentWeapon;
                        if( g_inputManager->controlEvent( ControlWeaponCycleLeft ) )
                        {
                            currentWeapon--;
                            if( currentWeapon < 0 ) 
                            {
                                currentWeapon = weaponList.Size() - 1;
                            }
                        }

                        if( g_inputManager->controlEvent( ControlWeaponCycleRight ) )
                        {
                            currentWeapon++;
                            if( currentWeapon >= weaponList.Size() )
                            {
                                currentWeapon = 0;
                            }
                        }

                        if( oldWeapon != currentWeapon ) 
                        {
                            g_app->m_clientToServer->RequestRunProgram( squad->m_teamId, weaponList[currentWeapon] );
                            g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureBegin", SoundSourceBlueprint::TypeGesture );
                            g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
                        }
                    }
                }
            }*/
        }
        else
        {
			g_app->m_clientToServer->RequestSelectUnit( team->m_teamId, -1, -1, -1 );
			g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
		}
	}
}


void LocationInput::Advance()
{
#ifdef TESTBED_ENABLED
	if(g_app->GetTestBedMode() == TESTBED_ON)
	{
		if(g_inputManager->controlEvent(ControlToggleRendering))
		{
			g_app->ToggleTestBedRendering();
		}
	}
#endif

	bool chatLog = g_app->m_camera->ChatLogVisible();

    if( g_app->m_globalWorld->m_myTeamId != 255 &&
        EclGetWindows()->Size() == 0 &&
        !chatLog )
    {
        AdvanceTeamControl();
    }


    if( g_app->Multiplayer() )
    {
#ifndef TESTBED_ENABLED
        if( g_app->m_multiwinia->GameInGracePeriod() &&
            g_inputManager->controlEvent( ControlToggleReady ) && 
            EclGetWindows()->Size() == 0 )
        {
            g_app->m_clientToServer->RequestToggleReady( g_app->m_globalWorld->m_myTeamId );
        }
#else
        if( g_app->m_multiwinia->GameInGracePeriod() && EclGetWindows()->Size() == 0 )
        {
            g_app->m_clientToServer->RequestSetTeamReady( g_app->m_globalWorld->m_myTeamId );
        }
#endif

#ifdef TESTBED_ENABLED
		// Have we hit a sync error
		if( g_app->m_clientToServer->m_outOfSyncClients.Size() )
		{
			if(g_app->GetTestBedMode() == TESTBED_ON)
			{
				g_app->m_requestedLocationId = -1;
				g_app->m_atMainMenu = true;
				g_app->m_fTestBedLastTime = 0.0f;
				
				if(g_app->GetTestBedState() != TESTBED_GAME_OVER_DELAY)
				{				
					g_app->m_iNumSyncErrors++;
					g_app->TestBedLogWrite();
				}

				g_app->SetTestBedState(TESTBED_GAME_OVER_DELAY);
			}

		}
#endif

#ifdef TESTBED_ENABLED
		if(g_app->GetTestBedMode() == TESTBED_ON)
		{

			if(g_app->m_iTerminationSequenceID != 0)
			{
				if(g_app->m_iTerminationSequenceID <= g_lastProcessedSequenceId)
				{
					g_app->m_requestedLocationId = -1;
					g_app->m_atMainMenu = true;
					g_app->m_fTestBedLastTime = 0.0f;
					g_app->SetTestBedState(TESTBED_GAME_OVER_DELAY);

					g_app->TestBedLogWrite();
				}
			}
		}
#endif

        if( g_app->m_multiwinia->GameOver() ) 
        {
#ifdef TESTBED_ENABLED
			if(g_app->GetTestBedMode() == TESTBED_ON)
			{
				if(EclGetWindows()->Size() == 0)
				{
					g_app->m_requestedLocationId = -1;
					g_app->m_atMainMenu = true;
					g_app->m_fTestBedLastTime = 0.0f;

					if(g_app->GetTestBedState() != TESTBED_GAME_OVER_DELAY)
					{
						g_app->TestBedLogWrite();
					}

					g_app->SetTestBedState(TESTBED_GAME_OVER_DELAY);
				}
			}
			else
			{
				if( g_inputManager->controlEvent( ControlToggleReady ) &&
					EclGetWindows()->Size() == 0 )
				{
					g_app->m_requestedLocationId = -1;
					g_app->m_atMainMenu = true;
				}
			}

#else
#endif        
		}

		if( ChatInputWindow::IsChatVisible() && g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
		{
			ChatInputWindow::CloseChat();
		}

        if( g_inputManager->controlEvent( ControlLobbyChat ) && !ChatInputWindow::IsChatVisible() &&
			!m_chatToggledThisUpdate && g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD )
        {
            if( EclGetWindows()->Size() == 0 )
            {
                m_chatToggledThisUpdate = true;
                EclRegisterWindow(new ChatInputWindow() );
            }
        }
        else
        {
            m_chatToggledThisUpdate = false;
        }
    }
}


void LocationInput::Render()
{
    g_editorFont.BeginText2D();
	if (g_app->m_location->GetMyTeam())
	{
        glColor4ubv(g_app->m_location->GetMyTeam()->m_colour.GetData());
//		glColor4f( 1.0, 1.0, 1.0, 1.0 );
		g_editorFont.DrawText2D( 12, 19, DEF_FONT_SIZE, 
			"You are TEAM %d", (int)g_app->m_globalWorld->m_myTeamId );
	}

    g_editorFont.EndText2D();
}
