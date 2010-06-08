#include "lib/universal_include.h"
#include "lib/debug_render.h"
#include "lib/language_table.h"
#include "lib/math_utils.h"
#include "lib/matrix34.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/hi_res_time.h"
#include "lib/preferences.h"
#include "lib/input/input.h"

#include "main.h"
#include "app.h"
#include "camera.h"
#include "helpsystem.h"
#include "renderer.h"
#include "location.h"
#include "particle_system.h"
#include "sepulveda.h"
#include "global_world.h"
#include "taskmanager.h"
#include "team.h"
#include "unit.h"

#include "worldobject/insertion_squad.h"

#include "sound/soundsystem.h"


// ============================================================================
// Class ActionHelp

ActionHelp::ActionHelp()
:   m_actionDone(false),
	m_data(-1)
{
}


bool ActionHelp::IsActionAvailable()
{
	return false;
}


void ActionHelp::GiveActionHelp()
{
}


void ActionHelp::PlayerDoneAction()
{
	m_actionDone = true;
}


namespace {

	// ============================================================================
	// ActionHelp

	class CameraMovementActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			return true;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_camera_movement" );
		}
	};


	class CameraHeightActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			return true;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_camera_height" );
		}
	};


	class TaskBasicsActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			return true;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_taskmanager_1" );
		}

		void PlayerDoneAction()
		{
			if( !m_actionDone )
			{
				g_app->m_sepulveda->ShutUp();
				g_app->m_sepulveda->Say( "help_taskmanager_2" );            
				m_actionDone = true;
			}
		}
	};


	class SquadDeselectActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			Unit *unit = g_app->m_location->GetMyTeam()->GetMyUnit();
			if( unit && unit->m_troopType == Entity::TypeInsertionSquadie )
			{
				return true;
			}
			return false;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_deselect" );
			m_actionDone = true;
		}
	};


	class TaskShutdownActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			return( g_app->m_taskManager->CapacityUsed() == g_app->m_taskManager->Capacity() );
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_taskmanager_3" );
			m_actionDone = true;
		}
	};


	class TaskReselectActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			bool somethingSelected = g_app->m_location->GetMyTeam()->m_currentUnitId != -1 ||
									 g_app->m_location->GetMyTeam()->m_currentEntityId != -1 ||
									 g_app->m_location->GetMyTeam()->m_currentBuildingId != -1;

			return( g_app->m_taskManager->m_tasks.Size() > 0 && !somethingSelected );
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_taskmanager_4" );
			m_actionDone = true;
		}
	};


	class SquadSummonActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			bool spaceAvailable = g_app->m_taskManager->CapacityUsed() < g_app->m_taskManager->Capacity();
			bool researchAVailable = g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeSquad );
	        
			return ( spaceAvailable && researchAVailable );
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_squad_summon_1" );
			g_app->m_sepulveda->DemoGesture( "squad.txt", 2.0f );
		}

		void PlayerDoneAction()
		{
			if( !m_actionDone )
			{
				g_app->m_sepulveda->ShutUp();
				g_app->m_sepulveda->Say( "help_squad_summon_2" );
				m_actionDone = true;
			}
		}
	};


	class SquadUseActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			if( g_app->m_location && 
				g_app->m_location->GetMyTeam() )
			{
				Unit *unit = g_app->m_location->GetMyTeam()->GetMyUnit();
				if( unit && unit->m_troopType == Entity::TypeInsertionSquadie )
				{
					return true;   
				}
			}
			return false;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_squad_use" );        
		}
	};


	class SquadThrowGrenadeActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{     
			bool researchAvailable = g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeGrenade );
			if( researchAvailable )
			{
				if( g_app->m_location &&
					g_app->m_location->GetMyTeam() )
				{
					Unit *unit = g_app->m_location->GetMyTeam()->GetMyUnit();
					if( unit && unit->m_troopType == Entity::TypeInsertionSquadie )
					{
						return true;   
					}
				}
			}
	        
			return false;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_squad_grenade" );
		}
	};


	class EngineerSummonActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			bool spaceAvailable = g_app->m_taskManager->CapacityUsed() < g_app->m_taskManager->Capacity();
			bool researchAvailable = g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeEngineer );
	        
			if( spaceAvailable && researchAvailable )
			{
				bool spiritsFound = g_app->m_location->m_spirits.NumUsed() > 0;
				if( spiritsFound ) return true;
	            
				for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
				{
					if( g_app->m_location->m_buildings.ValidIndex(i) )
					{
						Building *building = g_app->m_location->m_buildings[i];
						if( building->m_type == Building::TypeControlTower &&
							building->m_id.GetTeamId() != g_app->m_globalWorld->m_myTeamId )
						{
							return true;
						}
					}
				}
			}

			return false;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_engineer_summon" );
			g_app->m_sepulveda->DemoGesture( "engineer.txt", 2.0f );
		}

		void PlayerDoneAction()
		{
			if( !m_actionDone )
			{
				g_app->m_sepulveda->ShutUp();
				g_app->m_sepulveda->Say( "help_engineer_use" );
				m_actionDone = true;
			}
		}
	};


	class EngineerDeselectActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			Entity *entity = g_app->m_location->GetMyTeam()->GetMyEntity();
			if( entity && entity->m_type == Entity::TypeEngineer )
			{
				return true;
			}

			return false;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_deselect" );
			m_actionDone = true;
		}
	};


	class OfficerCreateActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			bool spaceAvailable = g_app->m_taskManager->CapacityUsed() < g_app->m_taskManager->Capacity();
			bool researchAvailable = g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeOfficer );

			if( spaceAvailable && researchAvailable )
			{
				for( int i = 0; i < g_app->m_location->m_teams[0].m_others.Size(); ++i )
				{
					if( g_app->m_location->m_teams[0].m_others.ValidIndex(i) )
					{
						Entity *entity = g_app->m_location->m_teams[0].m_others[i];
						if( entity->m_type == Entity::TypeDarwinian )
						{
							return true;
						}
					}
				}
			}

			return false;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_officer_create" );        
			g_app->m_sepulveda->DemoGesture( "officer.txt", 2.0f );
		}

		void PlayerDoneAction()
		{
			if( !m_actionDone )
			{
				g_app->m_sepulveda->ShutUp();

				g_app->m_sepulveda->Say( "help_officer_use" );            
				m_actionDone = true;
			}
		}       
	};


	class ArmourHelp : public ActionHelp
	{
		void PlayerDoneAction()
		{
			if( !m_actionDone )
			{
				g_app->m_sepulveda->Say( "help_armour_use" );
				m_actionDone = true;
			}
		}       
	};


	class ShowObjectivesActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			return true;
		}

		void GiveActionHelp()
		{
			 g_app->m_sepulveda->Say( "help_showobjectives" );
		}

		void PlayerDoneAction()
		{
			 m_actionDone = true;
		}
	};


	class CameraZoomActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			return true;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_camera_zoom" );
			m_actionDone = true;
		}
	};


	class CameraMoveFastActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			return true;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_camera_movefast" );
			m_actionDone = true;
		}
	};


	class ShowChatLogActionHelp : public ActionHelp
	{
		bool IsActionAvailable()
		{
			return true;
		}

		void GiveActionHelp()
		{
			g_app->m_sepulveda->Say( "help_showchatlog" );
			m_actionDone = true;
		}
	};


	class SeenBuildingHelp : public ActionHelp
	{
		void GiveActionHelp()
		{
			char helpString[64];
			Building *building = g_app->m_location->GetBuilding( m_data );
			if( building )
			{
				sprintf( helpString, "help_%s", Building::GetTypeName( building->m_type ) );
				g_app->m_sepulveda->HighlightBuilding( building->m_id.GetUniqueId(), "HelpSystem" );
				g_app->m_sepulveda->Say( helpString );
			}
		}
	};


	class SeenRadarDishHelp : public ActionHelp
	{
		void GiveActionHelp()
		{
			g_app->m_sepulveda->HighlightBuilding( m_data, "HelpSystem" );
			g_app->m_sepulveda->Say( "help_radardish_1" );
		}

		void PlayerDoneAction()
		{
			if( !m_actionDone )
			{
				g_app->m_sepulveda->ShutUp();
				g_app->m_sepulveda->Say( "help_radardish_2" );
			}
		}
	};


	class SeenGunTurretHelp : public ActionHelp
	{
		void GiveActionHelp()
		{
			g_app->m_sepulveda->HighlightBuilding( m_data, "HelpSystem" );
			g_app->m_sepulveda->Say( "help_gunturret_1" );
		}

		void PlayerDoneAction()
		{
			if( !m_actionDone )
			{
				g_app->m_sepulveda->ShutUp();
				g_app->m_sepulveda->Say( "help_gunturret_2" );
			}
		}
	};


	class ResearchUpgradeHelp : public ActionHelp
	{
		void PlayerDoneAction()
		{
			if( !m_actionDone )
			{
				g_app->m_sepulveda->Say( "research_explanation_1" );
				g_app->m_sepulveda->Say( "research_explanation_2" );
				g_app->m_sepulveda->Say( "research_explanation_3" );
				g_app->m_sepulveda->Say( "research_explanation_4" );

				m_actionDone = true;
			}
		}
	};

} // namespace

// ============================================================================
// Class HelpSystem


HelpSystem::HelpSystem()
:   m_nextHelpObjectId(0),
    m_actionHelpTimer(-1.0f)
{
    //
    // Action help

	try {
    m_actionHelp.SetSize( NumActionHelps );
    m_actionHelp.PutData( new CameraMovementActionHelp(),       CameraMovement );
    m_actionHelp.PutData( new CameraHeightActionHelp(),         CameraHeight );
    m_actionHelp.PutData( new TaskBasicsActionHelp(),           TaskBasics );
    m_actionHelp.PutData( new TaskShutdownActionHelp(),         TaskShutdown );
    m_actionHelp.PutData( new SquadSummonActionHelp(),          SquadSummon );
    m_actionHelp.PutData( new SquadUseActionHelp(),             SquadUse );
    m_actionHelp.PutData( new SquadThrowGrenadeActionHelp(),    SquadThrowGrenade );
    m_actionHelp.PutData( new SquadDeselectActionHelp(),        SquadDeselect );
    m_actionHelp.PutData( new EngineerSummonActionHelp(),       EngineerSummon );
    m_actionHelp.PutData( new EngineerDeselectActionHelp(),     EngineerDeselect );
    m_actionHelp.PutData( new TaskReselectActionHelp(),         TaskReselect );
    m_actionHelp.PutData( new OfficerCreateActionHelp(),        OfficerCreate );
    m_actionHelp.PutData( new ShowObjectivesActionHelp(),       ShowObjectives );
    m_actionHelp.PutData( new CameraZoomActionHelp(),           CameraZoom );
    m_actionHelp.PutData( new CameraMoveFastActionHelp(),       CameraMoveFast );
    m_actionHelp.PutData( new ShowChatLogActionHelp(),          ShowChatLog );
    m_actionHelp.PutData( new ArmourHelp(),                     UseArmour );
    m_actionHelp.PutData( new ResearchUpgradeHelp(),            ResearchUpgrade );
	} catch (std::exception ex) {
		const char * problem = ex.what();
		throw ex;
	}


    //
    // Building help

    m_buildingHelp.SetSize( Building::NumBuildingTypes );

    SeenRadarDishHelp *seenRadarDishHelp = new SeenRadarDishHelp();
    m_actionHelp.PutData    ( seenRadarDishHelp,                UseRadarDish );
    m_buildingHelp.PutData  ( seenRadarDishHelp,                Building::TypeRadarDish );
    
    SeenGunTurretHelp *seenGunTurretHelp = new SeenGunTurretHelp();
    m_actionHelp.PutData    ( seenGunTurretHelp,                UseGunTurret );
    m_buildingHelp.PutData  ( seenGunTurretHelp,                Building::TypeGunTurret );
    
    m_buildingHelp.PutData  ( new SeenBuildingHelp(),           Building::TypeIncubator );
    m_buildingHelp.PutData  ( new SeenBuildingHelp(),           Building::TypeTrunkPort );
    m_buildingHelp.PutData  ( new SeenBuildingHelp(),           Building::TypeRefinery );
    m_buildingHelp.PutData  ( new SeenBuildingHelp(),           Building::TypeMine );
    m_buildingHelp.PutData  ( new SeenBuildingHelp(),           Building::TypeAntHill );
    m_buildingHelp.PutData  ( new SeenBuildingHelp(),           Building::TypeControlTower );
    m_buildingHelp.PutData  ( new SeenBuildingHelp(),           Building::TypeResearchItem ); 
    m_buildingHelp.PutData  ( new SeenBuildingHelp(),           Building::TypeGunTurret );
}


void HelpSystem::PlayerDoneAction( int _actionId, int _data )
{
    if( !m_helpEnabled ) return;

    if( m_actionHelp.ValidIndex(_actionId) )
    {
        ActionHelp *help = m_actionHelp[ _actionId ];
        help->m_data = _data;
        help->PlayerDoneAction();
    }
}


void HelpSystem::Advance()
{
    if( !g_app->m_sepulveda->IsTalking() )
    {
        g_app->m_sepulveda->ClearHighlights( "HelpSystem" );
    }

    
    m_helpEnabled = (bool) g_prefsManager->GetInt( "HelpEnabled", 1 );
    
    bool cameraInteractive = g_app->m_camera->IsInteractive();

    bool tutorialRunning = g_app->m_tutorial;
    if( tutorialRunning ) m_helpEnabled = false;

    //
    // Even if help isn't enabled, pressing H should bring something up

    if( !m_helpEnabled ) 
    {
        if( g_inputManager->controlEvent( ControlSepulvedaHelp ) && cameraInteractive )
        {
            RunDefaultHelp();
        }
        return;
    }

    //
    // Advance our context sensitive help
    // Don't do anything if we're busy shooting, or if Sepulveda is already talking

    bool helpGiven = false;
    bool helpRequired = false;
    
    if( g_app->m_location )
    {        
        bool currentlyFiring = false;
        if( g_app->m_location->GetMyTeam() )
        {
            Unit *currentUnit = g_app->m_location->GetMyTeam()->GetMyUnit();
            if( currentUnit &&
                currentUnit->m_troopType == Entity::TypeInsertionSquadie &&
                g_inputManager->controlEvent( ControlUnitPrimaryFireTarget ) ) // TODO: Add directional mode
            {
                currentlyFiring = true;
            }
        }

        if( g_app->m_sepulveda->IsTalking() ||
            currentlyFiring )                           
        {
            m_actionHelpTimer = -1.0f;
        }
        else if( m_actionHelpTimer == -1.0f )
        {
            m_actionHelpTimer = GetHighResTime();        
        }
        
        if( g_inputManager->controlEvent( ControlSepulvedaHelp ) && cameraInteractive )
        {
            g_app->m_sepulveda->ShutUp();
            m_actionHelpTimer = GetHighResTime() - HELPSYSTEM_ACTIONHELP_INTERVAL - 10;
            helpRequired = true;
        }

        if( m_actionHelpTimer != -1.0f &&          
            GetHighResTime() > m_actionHelpTimer + HELPSYSTEM_ACTIONHELP_INTERVAL &&
		    g_inputManager->getInputMode() == INPUT_MODE_KEYBOARD )
        {
            // Sepulveda has been quiet for a while, so look for something helpful to say
            for( int i = 0; i < NumActionHelps; ++i )
            {
                if( m_actionHelp.ValidIndex(i) )
                {
                    ActionHelp *help = m_actionHelp[i];
                    if( !help->m_actionDone && help->IsActionAvailable() )
                    {
                        help->GiveActionHelp();
                        helpGiven = true;
                        break;
                    }
                }
            }
            m_actionHelpTimer = GetHighResTime();
        }

        // Are we looking at an interesting building?

        bool cameraInteractive = g_app->m_camera->IsInteractive();
        if( !currentlyFiring && !g_app->m_sepulveda->IsTalking() && cameraInteractive )
        {
            if( RunHighlightedBuildingHelp() ) helpGiven = true;
        }        

        if( helpRequired && !helpGiven )
        {
            RunDefaultHelp();
        }
    }
}


bool HelpSystem::RunHighlightedBuildingHelp()
{
    Building *building = g_app->m_camera->GetBestBuildingInView();
    bool reprogramRequired = false;
    bool helpGiven = false;

    if( building && building->m_id.GetTeamId() != g_app->m_globalWorld->m_myTeamId )
    {
        //
        // This building must be reprogrammed before it can be of use
        // Look for a nearby control tower to ensure this is possible
        
        int controlTowerFound = -1;
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *thisBuilding = g_app->m_location->m_buildings[i];
                if( thisBuilding->m_type == Building::TypeControlTower &&
                    thisBuilding->GetBuildingLink() == building->m_id.GetUniqueId() )
                {
                    controlTowerFound = thisBuilding->m_id.GetUniqueId();
                    break;
                }
            }
        }

        if( controlTowerFound != -1 )
        {
            g_app->m_sepulveda->HighlightBuilding( building->m_id.GetUniqueId(), "HelpSystem" );
            g_app->m_sepulveda->HighlightBuilding( controlTowerFound, "HelpSystem" );
            g_app->m_sepulveda->Say( "help_building_reprogram" );
            reprogramRequired = true;
            helpGiven = true;
        }        
    }

    if( building && m_buildingHelp.ValidIndex( building->m_type ) && !reprogramRequired )
    {
        ActionHelp *help = m_buildingHelp.GetData( building->m_type );
        bool controlTowerAlreadyReprogrammed = ( building->m_type == Building::TypeControlTower &&
                                                 building->m_id.GetTeamId() == 2 );

        if( !help->m_actionDone && !controlTowerAlreadyReprogrammed )
        {
            help->m_data = building->m_id.GetUniqueId();
            help->GiveActionHelp();
            helpGiven = true;
        }
    }

    return helpGiven;
}


void HelpSystem::RunDefaultHelp()
{
    Team *team = g_app->m_location->GetMyTeam();

    if( g_app->m_location && team )
    {
        bool helpGiven = false;

        //
        // Currently selected unit

        Unit *unit = team->GetMyUnit();
        if( unit )
        {
            if( unit->m_troopType == Entity::TypeInsertionSquadie )            
            {
                InsertionSquad *squad = (InsertionSquad *) unit;
                helpGiven = true;
                g_app->m_sepulveda->Say( "help_squad_use" );
                g_app->m_sepulveda->Say( "help_deselect" );
                
                if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeGrenade ) &&
                    squad->m_weaponType == GlobalResearch::TypeGrenade )
                {
                    g_app->m_sepulveda->Say( "help_squad_grenade" );
                }             
                
                if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeGrenade ) &&
                    squad->m_weaponType != GlobalResearch::TypeGrenade )
                {
                    g_app->m_sepulveda->Say( "help_squad_setgrenade" );
                }
                
                if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeRocket ) &&
                    squad->m_weaponType != GlobalResearch::TypeRocket )
                {
                    g_app->m_sepulveda->Say( "help_squad_setrocket" );
                }

                if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeAirStrike ) &&
                    squad->m_weaponType != GlobalResearch::TypeAirStrike )
                {
                    g_app->m_sepulveda->Say( "help_squad_setairstrike" );
                }
            }
        }

        //
        // Currently selected entity

        Entity *entity = team->GetMyEntity();
        if( entity )
        {
            if( entity->m_type == Entity::TypeEngineer )
            {
                helpGiven = true;
                g_app->m_sepulveda->Say( "help_engineer_use" );
                g_app->m_sepulveda->Say( "help_deselect" );                
            }
            else if( entity->m_type == Entity::TypeOfficer )
            {
                helpGiven = true;
                g_app->m_sepulveda->Say( "help_officer_use" );
                if( g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeOfficer ) > 2 )
                {
                    g_app->m_sepulveda->Say( "help_officer_toggle" );
                }
                g_app->m_sepulveda->Say( "help_deselect" );
            }
            else if( entity->m_type == Entity::TypeArmour )
            {
                helpGiven = true;
                g_app->m_sepulveda->Say( "help_armour_use" );
                g_app->m_sepulveda->Say( "help_deselect" );
            }
        }

        //
        // Currently selected building

        Building *building = g_app->m_location->GetBuilding( team->m_currentBuildingId );
        if( building )
        {
            if( building->m_type == Building::TypeRadarDish )
            {
                helpGiven = true;
                g_app->m_sepulveda->Say( "help_radardish_2" );
            }
            else if( building->m_type == Building::TypeGunTurret )
            {
                helpGiven = true;
                g_app->m_sepulveda->Say( "help_gunturret_2" );                
            }
        }

        //
        // Currently highlighted building

        if( RunHighlightedBuildingHelp() ) helpGiven = true;

        //
        // Current Task that needs targetting

        Task *task = g_app->m_taskManager->GetCurrentTask();
        if( task && task->m_state == Task::StateStarted )
        {
            if( task->m_type == GlobalResearch::TypeOfficer )
            {
                helpGiven = true;
                g_app->m_sepulveda->Say( "help_officer_create_2" );
            }
            else if( task->m_type == GlobalResearch::TypeSquad )
            {
                helpGiven = true;
                g_app->m_sepulveda->Say( "help_squad_summon_2" );
            }            
            else if( task->m_type == GlobalResearch::TypeEngineer )
            {
                helpGiven = true;
                g_app->m_sepulveda->Say( "help_engineer_summon_2" );
            }
            else if( task->m_type == GlobalResearch::TypeArmour )
            {
                helpGiven = true;
                g_app->m_sepulveda->Say( "help_armour_summon" );
            }
        }


        // 
        // Basic taskmanager type stuff if we haven't been able to find any
        // help to give them

        if( !helpGiven )
        {
            int numRunningTasks = g_app->m_taskManager->CapacityUsed();
            int maxTasks = g_app->m_taskManager->Capacity();

            if( numRunningTasks == 0 )
            {
                g_app->m_sepulveda->Say( "help_taskmanager_1" );
                g_app->m_sepulveda->Say( "help_taskmanager_2" );
                g_app->m_sepulveda->Say( "help_taskmanager_2" );
                g_app->m_sepulveda->Say( "help_showobjectives" );
            }
            else if( numRunningTasks == maxTasks )
            {
                g_app->m_sepulveda->Say( "task_manager_full" );
                g_app->m_sepulveda->Say( "help_taskmanager_4" );
            }
            else
            {
                g_app->m_sepulveda->Say( "help_taskmanager_4" );
                g_app->m_sepulveda->Say( "help_taskmanager_2" );
                g_app->m_sepulveda->Say( "help_showobjectives" );
            }
        }
    }
}


void HelpSystem::Render()
{
    if( !m_helpEnabled ) return;
	if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD ) return;

    START_PROFILE(g_app->m_profiler, "Render Helpsys");

    //
    // Press h for help

    if( !g_app->m_editing && 
        g_app->m_location && 
        !g_app->m_renderer->m_renderingPoster &&
        ( (int) g_gameTime % 2 == 0 ) &&
		g_inputManager->getInputMode() == INPUT_MODE_KEYBOARD )
    {
        g_gameFont.BeginText2D();
        g_gameFont.SetRenderOutline(true);        
        glColor4f(1.0f,1.0f,1.0f,0.0f);
        g_gameFont.DrawText2DCentre( g_app->m_renderer->ScreenW()/2.0f, 15, 15, LANGUAGEPHRASE("help_pressh") );
        g_gameFont.SetRenderOutline(false);
        glColor4f(1.0f,1.0f,1.0f,1.0f);
        g_gameFont.DrawText2DCentre( g_app->m_renderer->ScreenW()/2.0f, 15, 15, LANGUAGEPHRASE("help_pressh") );
        g_gameFont.EndText2D();
    }
    
    
    END_PROFILE(g_app->m_profiler, "Render Helpsys");
}

