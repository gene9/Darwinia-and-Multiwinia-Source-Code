#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/input/input.h"
#include "lib/text_renderer.h"

#include "main.h"
#include "tutorial.h"
#include "sepulveda.h"
#include "app.h"
#include "script.h"
#include "renderer.h"
#include "level_file.h"
#include "location.h"
#include "camera.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "global_world.h"
#include "team.h"
#include "unit.h"
#include "entity_grid.h"

#include "worldobject/controltower.h"
#include "worldobject/officer.h"
#include "worldobject/radardish.h"



Demo2Tutorial::Demo2Tutorial()
:   Tutorial()
{
}


void Demo2Tutorial::TriggerChapter( int _chapter )
{
    Tutorial::TriggerChapter( _chapter );

    switch( m_chapter )
    {
        case 1:         g_app->m_sepulveda->Say( "launchpad_tutorial_1" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_2" );
                        RepeatMessage( "launchpad_tutorial_2" );                                              
                        break;

        case 2:         g_app->m_sepulveda->Say( "launchpad_tutorial_3" );                                        
                        RepeatMessage( "launchpad_tutorial_3" );                                              
                        break;

        case 3:         g_app->m_sepulveda->Say( "launchpad_tutorial_4" );
                        RepeatMessage( "launchpad_tutorial_4" );
                        break;

        case 4:         g_app->m_sepulveda->Say( "launchpad_tutorial_5" );
                        RepeatMessage( "launchpad_tutorial_5" );
                        break;

        case 5:
        {
                        Building *building = g_app->m_location->GetBuilding(85);
                        DarwiniaReleaseAssert(building, "Tutorial building not found" );
                        g_app->m_camera->SetTarget( building->m_pos, 400, 300 );  
		                g_app->m_camera->SetMoveDuration(2);		                
                        g_app->m_camera->RequestMode(Camera::ModeMoveToTarget);
                        break;
        }

        case 6:         g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
                        g_app->m_sepulveda->Say( "launchpad_tutorial_6" );
                        RepeatMessage( "launchpad_tutorial_6" );
                        break;

        case 7:         g_app->m_sepulveda->Say( "launchpad_tutorial_7" );
                        RepeatMessage( "launchpad_tutorial_7" );
                        break;

        case 8:         g_app->m_sepulveda->Say( "launchpad_tutorial_8" );
                        RepeatMessage( "launchpad_tutorial_8" );
                        break;

        case 9:         break;

        case 10:        g_app->m_sepulveda->Say( "launchpad_tutorial_9" );
                        break;

        case 11:        g_app->m_sepulveda->HighlightBuilding( 129, "research" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_10" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_11" );
                        RepeatMessage( "launchpad_tutorial_11" );
                        break;

        case 12:        break;

        case 13:        g_app->m_sepulveda->ClearHighlights();
                        g_app->m_sepulveda->Say( "launchpad_tutorial_12" );
                        RepeatMessage( "launchpad_tutorial_12" );
                        break;

        case 14:    
        {
                        g_app->m_camera->RecordCameraPosition();
                        Building *research = g_app->m_location->GetBuilding(129);
                        g_app->m_camera->RequestBuildingFocusMode(research, 150, 20);
                        g_app->m_camera->SetTargetFOV( 30 );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_12b" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_12c" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_12d" );
                        break;
        }

        case 141:
        {
                        g_app->m_camera->RequestMode(Camera::ModeFreeMovement); 
                        g_app->m_sepulveda->Say( "launchpad_tutorial_12e" );
                        break;
        }

        case 15:        g_app->m_camera->RequestMode(Camera::ModeFreeMovement);                                                
                        g_app->m_sepulveda->Say( "launchpad_tutorial_13" );       
                        g_app->m_sepulveda->Say( "launchpad_tutorial_13b" );       
                        RepeatMessage( "launchpad_tutorial_13b" );
                        break;

        case 16:        g_app->m_sepulveda->Say( "launchpad_tutorial_14" );
                        RepeatMessage( "launchpad_tutorial_14" );
                        break;

        case 17:        break;

        case 18:        g_app->m_sepulveda->Say( "launchpad_tutorial_15" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_16" );
                        break;

        case 19:        
        {
                        g_app->m_camera->RecordCameraPosition();
                        Building *building = g_app->m_location->GetBuilding(66);
                        g_app->m_camera->RequestBuildingFocusMode( building, 150, 50 );
                        DarwiniaReleaseAssert( building, "Tutorial building not found" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_17" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_18" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_19" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_20" );
                        break;
        }

        case 191:       //g_app->m_camera->RestoreCameraPosition();
                        break;

        case 20:        g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
                        g_app->m_sepulveda->Say( "launchpad_tutorial_21" );
                        RepeatMessage( "launchpad_tutorial_21", 60 );
                        break;

        case 21:        g_app->m_sepulveda->Say( "launchpad_tutorial_22" );
                        break;

        case 22:        g_app->m_sepulveda->Say( "launchpad_tutorial_23" );
                        RepeatMessage( "launchpad_tutorial_23" );
                        break;

        case 23:        g_app->m_sepulveda->HighlightBuilding( 67, "Tutorial" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_24" );
                        RepeatMessage( "launchpad_tutorial_24" );
                        break;

        case 24:        g_app->m_sepulveda->ClearHighlights( "Tutorial" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_25" );
                        RepeatMessage( "launchpad_tutorial_25" );
                        break;

        case 25:        g_app->m_sepulveda->Say( "launchpad_tutorial_26" );
                        RepeatMessage( "launchpad_tutorial_26" );
                        break;

        case 26:        break;

        case 40:        
        {
                        Building *building = g_app->m_location->GetBuilding(86);
                        DarwiniaReleaseAssert( building, "Tutorial building not found" );
                        g_app->m_camera->RequestBuildingFocusMode( building, 200, 150 );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_40" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_41" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_42" );
                        break;
        }

        case 41:        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_43" );
                        RepeatMessage( "launchpad_tutorial_43", 60 );
                        break;
        
        case 42:        g_app->m_sepulveda->Say( "launchpad_tutorial_44" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_45" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_46" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_47" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_48" );
                        break;
       
        case 43:        BeginPlayerBusyCheck();
                        break;

        case 44:        g_app->m_camera->SetTarget( "incubator1" );
                        g_app->m_camera->CutToTarget();
                        g_app->m_camera->SetTarget( "incubator2" );
                        g_app->m_camera->SetMoveDuration( 30 );
                        g_app->m_camera->RequestMode( Camera::ModeMoveToTarget );
                        g_app->m_camera->SetFOV( 40 );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_49" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_50" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_51" );
                        break;
            
        case 45:        g_app->m_camera->SetTarget( "incubator3" );
                        g_app->m_camera->SetMoveDuration( 15 );
                        g_app->m_camera->RequestMode( Camera::ModeMoveToTarget );
                        g_app->m_camera->SetFOV( 40 );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_52" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_53" );
                        break;

        case 46:        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        break;

        case 461:       BeginPlayerBusyCheck();
                        break;
                        
        case 47:        
        {
                        Building *building = g_app->m_location->GetBuilding( 0 );
                        g_app->m_camera->RequestBuildingFocusMode( building, 600, 100 );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_54" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_54b" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_55" );
                        break;
        }

        case 48:    
        {
                        Building *building = g_app->m_location->GetBuilding( 1 );
                        g_app->m_camera->RequestBuildingFocusMode( building, 400, 300 );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_56" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_57" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_58" );
                        break;
        }

        case 49:
        {
                        Building *building = g_app->m_location->GetBuilding( 25 );
                        g_app->m_camera->RequestBuildingFocusMode( building, 400, 200 );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_59" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_60" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_61" );
                        break;
        }

        case 50:
        {
                        g_app->m_camera->SetTarget( "incubator2" );
                        g_app->m_camera->SetMoveDuration( 5 );
                        g_app->m_camera->RequestMode( Camera::ModeMoveToTarget );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_62" );
                        break;
        }

        case 51:        RepeatMessage( "launchpad_tutorial_62" );
                        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        break;

        case 52:        g_app->m_sepulveda->Say( "launchpad_tutorial_63" );
                        RepeatMessage( "launchpad_tutorial_63" );
                        break;

        case 53:        g_app->m_sepulveda->Say( "launchpad_tutorial_64" );
                        RepeatMessage( "launchpad_tutorial_64" );
                        break;

        case 54:        g_app->m_sepulveda->Say( "launchpad_tutorial_65" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_66" );
                        RepeatMessage( "launchpad_tutorial_66" );
                        break;

        case 55:        g_app->m_sepulveda->Say( "launchpad_tutorial_67" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_68" );
                        RepeatMessage( "launchpad_tutorial_68", 60.0f );
                        break;

        case 56:
        {
                        Building *building = g_app->m_location->GetBuilding( 25 );
                        g_app->m_camera->RequestBuildingFocusMode( building, 400, 250 );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_69" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_70" );
                        break;
        }

        case 57:
        {
                        Building *building = g_app->m_location->GetBuilding( 1 );
                        g_app->m_camera->RequestBuildingFocusMode( building, 400, 300 );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_71" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_72" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_73" );
                        break;
        }

        case 58:        g_app->m_camera->SetTarget( "attack6" );
                        g_app->m_camera->SetMoveDuration( 10 );
                        g_app->m_camera->RequestMode( Camera::ModeMoveToTarget );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_74" );
                        break;
              
        case 59:        g_app->m_camera->SetTarget( "wherenext1" );
                        g_app->m_camera->SetMoveDuration( 8 );
                        g_app->m_camera->RequestMode( Camera::ModeMoveToTarget );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_75" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_76" );
                        g_app->m_sepulveda->Say( "launchpad_tutorial_77" );
                        break;
                        
        case 60:        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        break;

        default:
                        g_app->m_sepulveda->Say( "launchpad_tutorial_done" );
                        break;
    }
}


bool Demo2Tutorial::AdvanceCurrentChapter()
{
    bool result = Tutorial::AdvanceCurrentChapter();
    if( !result ) return false;

    switch( m_chapter )
    {
        case 0:                                                                                                 
            break;

        case 1:                     // Camera Movement        
        {
            CameraMount *mount = g_app->m_location->m_levelFile->GetCameraMount( "start" );
            float distance = ( mount->m_pos - g_app->m_camera->GetPos() ).Mag();
            if( distance > 200.0f ) m_nextChapterTimer = GetHighResTime() + 5.0f;
            break;        
        }
        
        case 2:                     // Camera Height
        {
            CameraMount *mount = g_app->m_location->m_levelFile->GetCameraMount( "start" );
            float difference = fabs( mount->m_pos.y - g_app->m_camera->GetPos().y );
            if( difference > 100.0f ) m_nextChapterTimer = GetHighResTime() + 10.0f;
            break;
        }

        case 3:                     // Task Manager Visible
        {
            if( g_app->m_taskManagerInterface->m_visible )
            {
                TriggerChapter( m_chapter+1 );
            }
            break;
        }

        case 4:                     // Create a SQUAD
        {
            Task *currentTask = g_app->m_taskManager->GetCurrentTask();
            if( currentTask && currentTask->m_type == GlobalResearch::TypeSquad )
            {
                TriggerChapter(m_chapter+1);
            }
            else if( currentTask && currentTask->m_type != GlobalResearch::TypeSquad )
            {
                g_app->m_taskManager->TerminateTask( currentTask->m_id );
                g_app->m_sepulveda->Say( "launchpad_tutorial_5_error" );
            }
            break;
        }

        case 5:                     // Move the camera
            if( !g_app->m_camera->IsAnimPlaying() )
            {
                TriggerChapter(m_chapter+1);
            }
            break;

        case 6:                     // Place the squad in the world
        {
            HandleSquadDeath();
            Task *currentTask = g_app->m_taskManager->GetCurrentTask();
            if( currentTask && 
                currentTask->m_type == GlobalResearch::TypeSquad &&
                currentTask->m_state == Task::StateRunning )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 7:                     // Move the squad around
        {
            HandleSquadDeath();
            Unit *unit = g_app->m_location->m_teams[2].GetMyUnit();
            if( unit )
            {
                float distance = ( unit->GetWayPoint() - unit->m_centrePos ).Mag();
				if( unit->m_vel.Mag() > 0)
                {
                    m_nextChapterTimer = GetHighResTime() + 5.0f;
                }
            }
            break;
        }

        case 8:                         // Fire lasers
            HandleSquadDeath();
            for( int l = 0; l < g_app->m_location->m_lasers.Size(); ++l )
            {
                if( g_app->m_location->m_lasers.ValidIndex(l) )
                {
                    if( g_app->m_location->m_lasers[l].m_fromTeamId == 2 )
                    {
                        m_nextChapterTimer = GetHighResTime() + 5.0f;
                    }
                }
            }
            break;    

        case 9:                         // UNUSED   Move the camera
            TriggerChapter(m_chapter+1);
            break;

        case 10:                        // Kill all virii 
        {
            HandleSquadDeath();
            Building *building = g_app->m_location->GetBuilding(129);            
            int numEnemies = 0;
            if( building )
            {
                numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( building->m_pos.x, building->m_pos.z, 200.0f, 2 );
            }
            if( numEnemies == 0 ) m_nextChapterTimer = GetHighResTime() + 5.0f;          
            break;
        }         

        case 11:                        // Summon Engineer
        {
            Task *currentTask = g_app->m_taskManager->GetCurrentTask();
            if( currentTask && currentTask->m_type == GlobalResearch::TypeEngineer )
            {
                TriggerChapter(13);
            }
            break;            
        }

        case 13:                        // Place Engineer
        {
            Building *building = g_app->m_location->GetBuilding(129);            
            DarwiniaReleaseAssert( building, "Tutorial building not found" );
            bool engineerFound = false;
            if( building )
            {
                int numFound;
                WorldObjectId *ids = g_app->m_location->m_entityGrid->GetFriends( building->m_pos.x, building->m_pos.z, 150.0f, &numFound, 2 );
                for( int i = 0; i < numFound; ++i )
                {
                    WorldObjectId id = ids[i];
                    Entity *engineer = g_app->m_location->GetEntitySafe( id, Entity::TypeEngineer );
                    if( engineer )
                    {
                        engineerFound = true;
                        break;
                    }
                }
            }
            if( engineerFound ||
                !building )
            {
                TriggerChapter(m_chapter+1);
            }
            break;            
        }

        case 14:                        // Finish talking
            if( !g_app->m_sepulveda->IsTalking() )
            {
                TriggerChapter(141);
            }
            break;
            
        case 141:                        // Has grenade research
            if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeGrenade ) )
            {
                TriggerChapter( 15 );
            }
            break;

        case 15:                        // Reselected the SQUAD
        {
            Task *currentTask = g_app->m_taskManager->GetCurrentTask();
            if( currentTask && 
                currentTask->m_type == GlobalResearch::TypeSquad &&
                currentTask->m_state == Task::StateRunning )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 16:                        // Thrown a grenade
        {
            for( int i = 0; i < g_app->m_location->m_effects.Size(); ++i )
            {
                if( g_app->m_location->m_effects.ValidIndex(i) )
                {
                    WorldObject *wobj = g_app->m_location->m_effects[i];
                    if( wobj->m_id.GetUnitId() == UNIT_EFFECTS &&
                        wobj->m_type == WorldObject::EffectThrowableGrenade &&
                        wobj->m_id.GetTeamId() == 2 )
                    {
                        m_nextChapterTimer = GetHighResTime() + 5.0f;
                    }
                }
            }
            break;
        }

        case 17:   TriggerChapter(m_chapter+1);            
            
        case 18:                        // Wipe out virii
        {
            Building *building = g_app->m_location->GetBuilding(75);            
            DarwiniaReleaseAssert( building, "Tutorial building not found" );
            int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( building->m_pos.x, building->m_pos.z, 200.0f, 2 );
            if( numEnemies == 0 ) m_nextChapterTimer = GetHighResTime() + 5.0f; 
            break;
        }

        case 19:                        // Finish talking
            if( !g_app->m_sepulveda->IsTalking() )
            {
                TriggerChapter(191);
            }
            break;

        case 191:                       // Cam movement
            if( !g_app->m_camera->IsAnimPlaying() )
            {
                TriggerChapter(20);
            }
            break;

        case 20:                        // Engineer near radar dish
        {
            Building *building = g_app->m_location->GetBuilding(66);            
            DarwiniaReleaseAssert( building, "Tutorial building not found" );
            int numFound;
            WorldObjectId *ids = g_app->m_location->m_entityGrid->GetFriends( building->m_pos.x, building->m_pos.z, 150.0f, &numFound, 2 );
            bool engineerFound = false;
            for( int i = 0; i < numFound; ++i )
            {
                WorldObjectId id = ids[i];
                Entity *engineer = g_app->m_location->GetEntitySafe( id, Entity::TypeEngineer );
                if( engineer )
                {
                    engineerFound = true;
                    break;
                }
            }
            if( engineerFound )
            {
                TriggerChapter(m_chapter+1);
            }
            break;            
        }

        case 21:                        // Radar dish reprogrammed
        {
            Building *building = g_app->m_location->GetBuilding(66);            
            DarwiniaReleaseAssert( building, "Tutorial building not found" );
            if( building->m_id.GetTeamId() == 2 )
            {
                m_nextChapterTimer = GetHighResTime() + 2.0f;
            }
            break;
        }

        case 22:                        // Radar dish selected
        {
            Building *building = g_app->m_location->GetBuilding( g_app->m_location->GetMyTeam()->m_currentBuildingId );
            if( building && building->m_id.GetUniqueId() == 66 )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 23:                        // Radar dish aligned
        {
            Building *building = g_app->m_location->GetBuilding( 66 );
            if( building && building->m_type == Building::TypeRadarDish )
            {
                RadarDish *dish = (RadarDish *) building;
                if( dish->GetConnectedDishId() == 67 )
                {
                    TriggerChapter(m_chapter+1);
                }
            }
            break;
        }

        case 24:                        // Reselected squad
        {
            HandleDishMisalignment();
            Task *currentTask = g_app->m_taskManager->GetCurrentTask();
            if( currentTask && 
                currentTask->m_type == GlobalResearch::TypeSquad &&
                currentTask->m_state == Task::StateRunning )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 25:                        // Squad in transit
        {
            Unit *currentUnit = g_app->m_location->GetMyTeam()->GetMyUnit();
            if( currentUnit && currentUnit->m_troopType == Entity::TypeInsertionSquadie )
            {
                bool activeFound = false;
                for( int i = 0; i < currentUnit->m_entities.Size(); ++i )
                {
                    if( currentUnit->m_entities.ValidIndex(i) )
                    {
                        Entity *ent = currentUnit->m_entities[i];
                        if( ent->m_enabled && !ent->m_dead )
                        {
                            activeFound = true;
                            break;
                        }
                    }
                }
                if( !activeFound )
                {
                    m_nextChapterTimer = GetHighResTime() + 3;
                }
            }
            break;
        }

        case 26:                        // Wipe out virii + centipede on island 2
        {
            Building *building = g_app->m_location->GetBuilding(86);            
            DarwiniaReleaseAssert( building, "Tutorial building not found" );
            int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( building->m_pos.x, building->m_pos.z, 250.0f, 2 );
            if( numEnemies == 0 ) 
            {
                m_chapter = 39;
                m_nextChapterTimer = GetHighResTime() + 2.0f; 
            }
            break;
        }

        case 40:                            // Finish talking
            if( !g_app->m_sepulveda->IsTalking() )
            {
                m_nextChapterTimer = GetHighResTime() + 2.0f;
            }
            break;

        case 41:                            // Reprogrammed Incubator
        {
            Building *building = g_app->m_location->GetBuilding(86);            
            DarwiniaReleaseAssert( building, "Tutorial building not found" );
            if( building->m_id.GetTeamId() == 2 )
            {
                m_nextChapterTimer = GetHighResTime() + 5.0f;
            }
            break;
        }

        case 42:                        // First Darwinians born
        {
            Building *building = g_app->m_location->GetBuilding(86);            
            DarwiniaReleaseAssert( building, "Tutorial building not found" );
            bool includeTeam[NUM_TEAMS];
            memset( includeTeam, 0, sizeof(bool) * NUM_TEAMS );
            includeTeam[0] = true;
            int numDarwinians = g_app->m_location->m_entityGrid->GetNumNeighbours( building->m_pos.x, building->m_pos.z, 300.0f, includeTeam  );
            if( numDarwinians > 0 ) 
            {
                m_nextChapterTimer = GetHighResTime() + 1.0f; 
            }
            break;
        }

        case 43:                            // Wait for player to not be busy
            if( !IsPlayerBusy() )
            {
                m_nextChapterTimer = GetHighResTime() + 1.0f;
            }
            break;

        case 44:                        // Finish talking
            if( !g_app->m_sepulveda->IsTalking() )
            {
                m_nextChapterTimer = GetHighResTime() + 1.0f;
            }
            break;

        case 45:                        // Finish talking
            if( !g_app->m_sepulveda->IsTalking() )
            {
                m_nextChapterTimer = GetHighResTime() + 1.0f;
            }
            break;

        case 46:                        // Picked up Rocket Research
            if( g_app->m_globalWorld->m_research->HasResearch( GlobalResearch::TypeOfficer ) )
            {
                TriggerChapter(461);
            }
            break;

        case 461:                       // player not busy
            if( !IsPlayerBusy() )
            {
                TriggerChapter(47);
            }
            break;

        case 47:                        // Cutscene occuring
        case 48:
        case 49:
        case 50:
            if( !g_app->m_sepulveda->IsTalking() )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
            
        case 51:                        // Running officer
        {
            Task *currentTask = g_app->m_taskManager->GetCurrentTask();
            if( currentTask && currentTask->m_type == GlobalResearch::TypeOfficer )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 52:                        // Officer selected
        {
            Entity *entity = g_app->m_location->GetMyTeam()->GetMyEntity();
            if( entity && entity->m_type == Entity::TypeOfficer )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 53:                        // Officer in follow mode
        {
            Entity *entity = g_app->m_location->GetMyTeam()->GetMyEntity();
            if( entity && entity->m_type == Entity::TypeOfficer )
            {
                Officer *officer = (Officer *) entity;
                if( officer->m_orders == Officer::OrderFollow )
                {
                    TriggerChapter(m_chapter+1);
                }
            }
            break;
        }

        case 54:                        // Officer in GOTO mode
        {
            Entity *entity = g_app->m_location->GetMyTeam()->GetMyEntity();
            if( entity && entity->m_type == Entity::TypeOfficer )
            {
                Officer *officer = (Officer *) entity;
                if( officer->m_orders == Officer::OrderGoto )
                {
                    TriggerChapter(m_chapter+1);
                }
            }
            break;
        }

        case 55:                        // First fuel generator working
        {
            GlobalEventCondition *cond = g_app->m_location->m_levelFile->m_primaryObjectives[0];
            if( cond->Evaluate() )
            {
                m_nextChapterTimer = GetHighResTime() + 5.0f;
            }
            break;
        }

        case 56:                    // cutscene
        case 57:
        case 58:
        case 59:
            if( !g_app->m_sepulveda->IsTalking() )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
            
        case 60:
            return true;
            break;
    }

    return false;
}


void Demo2Tutorial::HandleDishMisalignment()
{
    Building *building = g_app->m_location->GetBuilding( g_app->m_location->GetMyTeam()->m_currentBuildingId );
    if( building && building->m_type == Building::TypeRadarDish )
    {
        RadarDish *dish = (RadarDish *) building;
        if( dish->GetConnectedDishId() != 67 )
        {
            if( g_app->m_location->GetMyTeam()->m_currentBuildingId != 67 )
            {
                TriggerChapter( 22 );
            }
            else
            {
                TriggerChapter( 23 );
            }
        }
    }
}


void Demo2Tutorial::HandleSquadDeath()
{
    //
    // Ensure the currently running task is a fully functioning squad

    Task *currentTask = g_app->m_taskManager->GetCurrentTask();
    
    if( !currentTask || currentTask->m_type != GlobalResearch::TypeSquad )
    {
        // If the player deselected by accident, reselect now
        for( int i = 0; i < g_app->m_taskManager->m_tasks.Size(); ++i )
        {
            Task *task = g_app->m_taskManager->m_tasks[i];
            if( task && 
                task->m_type == GlobalResearch::TypeSquad )
            {
                g_app->m_taskManager->SelectTask( task->m_id );
                return;
            }
        }

        // There are no squad tasks running.
        g_app->m_sepulveda->ShutUp();
        g_app->m_sepulveda->Say( "launchpad_tutorial_9_error" );
        m_chapter = 2;
        m_nextChapterTimer = GetHighResTime() + 5;
    }
}


float Demo2Tutorial::s_playerBusyTimer = 0.0f;


void Demo2Tutorial::BeginPlayerBusyCheck()
{
    s_playerBusyTimer = GetHighResTime();
}


bool Demo2Tutorial::IsPlayerBusy()
{
    float timeNow = GetHighResTime();
   
    //
    // See if we are doing squaddie things

    if( g_app->m_location->GetMyTeam() )
    {
        Unit *currentUnit = g_app->m_location->GetMyTeam()->GetMyUnit();
        if( currentUnit && currentUnit->m_troopType == Entity::TypeInsertionSquadie )
        {         
            if( g_inputManager->controlEvent( ControlUnitPrimaryFireTarget ) )
            {
                // We are firing lasers
                s_playerBusyTimer = timeNow;
                return true;
            }

            Vector3 wayPoint = currentUnit->GetWayPoint();            
            if( g_app->m_location->m_entityGrid->AreEnemiesPresent( wayPoint.x, wayPoint.z, 100, 2 ) )
            {
                // Enemies near our squaddie waypoint
                s_playerBusyTimer = timeNow;
                return true;
            }
        }
    }


    //
    // Under timer?

    if( timeNow < s_playerBusyTimer + 3.0f )
    {
        return true;
    }


    //
    // The player isn't busy

    return false;
}


