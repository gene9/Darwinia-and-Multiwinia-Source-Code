#include "lib/universal_include.h"

#ifdef USE_SEPULVEDA_HELP_TUTORIAL

#include "lib/hi_res_time.h"
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


Demo1Tutorial::Demo1Tutorial()
:   Tutorial()
{
}


void Demo1Tutorial::TriggerChapter( int _chapter )
{
    Tutorial::TriggerChapter( _chapter );       

    switch( m_chapter )
    {
        case 1:         g_app->m_sepulveda->Say( "tutorial_1" ); 
                        RepeatMessage( "tutorial_1" );                                              
                        break;

        case 2:         g_app->m_sepulveda->Say( "tutorial_2" );                                        
                        RepeatMessage( "tutorial_2" );                                              
                        break;
        
        case 3:         
        {
            Building *building = g_app->m_location->GetBuilding(52);
            g_app->m_camera->RequestBuildingFocusMode( building, 200, 200 );
            g_app->m_sepulveda->Say( "tutorial_3" );
            g_app->m_sepulveda->Say( "tutorial_3b" );
            break;
        }

        case 4:	        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        g_app->m_sepulveda->Say( "tutorial_4" );
                        g_app->m_sepulveda->Say( "tutorial_4b" );
                        RepeatMessage( "tutorial_4b" );
                        break;        
                        
        case 5:         g_app->m_sepulveda->Say( "tutorial_5" );
                        g_app->m_sepulveda->DemoGesture( "squad.txt", 2.0 );
                        RepeatMessage( "tutorial_5", 30.0, "squad.txt" );
                        break;

        case 6:         g_app->m_camera->SetTarget("tutorial1");	       
		                g_app->m_camera->SetMoveDuration(3);		                
                        g_app->m_camera->RequestMode(Camera::ModeMoveToTarget);
            	        break;     

        case 7:         g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        g_app->m_sepulveda->Say( "tutorial_7" );
                        RepeatMessage( "tutorial_7" );
                        break;

        case 8:         g_app->m_sepulveda->Say( "tutorial_8" );    
                        RepeatMessage( "tutorial_8" );
                        break;

        case 9:         g_app->m_sepulveda->Say( "tutorial_9" );   
                        RepeatMessage( "tutorial_9" );
                        break;

        case 10:        g_app->m_sepulveda->Say( "tutorial_10" );                       
                        RepeatMessage( "tutorial_10" );
                        break;

        case 11:        g_app->m_sepulveda->Say( "tutorial_11" );
                        RepeatMessage( "tutorial_11", 60 );
                        break;

        case 12:        g_app->m_sepulveda->Say( "tutorial_12" );
                        break;

        case 13:        g_app->m_sepulveda->Say( "tutorial_13" );
                        break;
              
        case 20:        
        {
            Building *building = g_app->m_location->GetBuilding(97);
            g_app->m_camera->RequestBuildingFocusMode( building, 120, 120 );
            g_app->m_sepulveda->Say( "tutorial_20" );
            g_app->m_sepulveda->Say( "tutorial_20b" );
            g_app->m_sepulveda->Say( "tutorial_20c" );
            break;
        }

        case 21:        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        g_app->m_sepulveda->Say( "tutorial_21" );
                        g_app->m_sepulveda->DemoGesture( "engineer.txt", 2.0 );
                        RepeatMessage( "tutorial_21", 30.0, "engineer.txt" );
                        break;

        case 22:        g_app->m_sepulveda->Say( "tutorial_22" );
                        RepeatMessage( "tutorial_22" );
                        break;

        case 23:
        {
            Building *building = g_app->m_location->GetBuilding(52);
            g_app->m_camera->RequestBuildingFocusMode( building, 70, 70 );
            g_app->m_sepulveda->Say( "tutorial_23" );
            break;
        }

        case 24:        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        g_app->m_sepulveda->Say( "tutorial_24" );
                        g_app->m_sepulveda->Say( "tutorial_24b" );
                        g_app->m_sepulveda->Say( "tutorial_24c" );
                        break;

        case 25:        g_app->m_camera->SetTarget("tutorial2");	       
		                g_app->m_camera->SetMoveDuration(3);		                
                        g_app->m_camera->RequestMode(Camera::ModeMoveToTarget);
                        g_app->m_sepulveda->Say( "tutorial_25" );
                        break;

        case 26:        g_app->m_camera->SetTarget("tutorial3");	       
		                g_app->m_camera->SetMoveDuration(15);		                
                        g_app->m_camera->RequestMode(Camera::ModeMoveToTarget);
                        g_app->m_sepulveda->Say( "tutorial_26" );
                        break;

        case 27:        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        break;

        case 28:        g_app->m_camera->SetTarget("tutorial6");	       
		                g_app->m_camera->SetMoveDuration(3);		                
                        g_app->m_camera->RequestMode(Camera::ModeMoveToTarget);        
                        g_app->m_sepulveda->Say( "tutorial_28" );
                        g_app->m_sepulveda->DemoGesture( "squad.txt", 2 );
                        RepeatMessage( "tutorial_28", 30, "squad.txt" );
                        break;

        case 29:        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        g_app->m_sepulveda->Say( "tutorial_29" );
                        RepeatMessage( "tutorial_29" );
                        break;

        case 30:        g_app->m_camera->SetTarget("tutorial4");	       
		                g_app->m_camera->SetMoveDuration(3);		                
                        g_app->m_camera->RequestMode(Camera::ModeMoveToTarget);
                        g_app->m_sepulveda->Say( "tutorial_30" );
                        break;

        case 31:        g_app->m_camera->SetTarget("tutorial5");	       
		                g_app->m_camera->SetMoveDuration(15);		                
                        g_app->m_camera->RequestMode(Camera::ModeMoveToTarget);
                        g_app->m_sepulveda->Say( "tutorial_31" );
                        g_app->m_sepulveda->Say( "tutorial_31b" );
                        break;
 
        case 32:        g_app->m_sepulveda->Say( "tutorial_32" );                        
                        break;

        case 33:        g_app->m_camera->SetTarget("tutorial7");	       
		                g_app->m_camera->SetMoveDuration(4);		                
                        g_app->m_camera->RequestMode(Camera::ModeMoveToTarget);
                        g_app->m_sepulveda->Say( "tutorial_33" );
                        g_app->m_sepulveda->Say( "tutorial_33b" );
                        break;
        
        case 34:        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        g_app->m_sepulveda->Say( "tutorial_34" );
                        RepeatMessage( "tutorial_34", 40 );
                        break;

        case 35:        g_app->m_camera->SetTarget("tutorial2");	       
		                g_app->m_camera->SetMoveDuration(30);		                
                        g_app->m_camera->RequestMode(Camera::ModeMoveToTarget);   
                        g_app->m_sepulveda->Say( "tutorial_35" );
                        g_app->m_sepulveda->Say( "tutorial_35b" );
                        g_app->m_sepulveda->Say( "tutorial_35c" );
                        break;

        case 351:       g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        g_app->m_sepulveda->Say( "tutorial_351" );
                        g_app->m_sepulveda->DemoGesture( "officer.txt", 2 );
                        RepeatMessage( "tutorial_351", 30, "officer.txt" );
                        break;

        case 36:        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        g_app->m_sepulveda->Say( "tutorial_36" );
                        RepeatMessage( "tutorial_36" );
                        break;
            
        case 37:        g_app->m_sepulveda->Say( "tutorial_37" );
                        g_app->m_sepulveda->Say( "tutorial_37b" );
                        RepeatMessage( "tutorial_37b" );
                        break;

        case 38:
        {
            Building *building = g_app->m_location->GetBuilding(90);
            g_app->m_camera->RequestBuildingFocusMode( building, 400, 100 );
            g_app->m_sepulveda->Say( "tutorial_38" );
            g_app->m_sepulveda->Say( "tutorial_38b" );
            g_app->m_sepulveda->Say( "tutorial_38c" );
            g_app->m_sepulveda->Say( "tutorial_38d" );
            g_app->m_sepulveda->Say( "tutorial_38e" );
            break;
        }
        
        case 39:        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        g_app->m_sepulveda->Say( "tutorial_39" );
                        break;
                        
        case 40:        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                        g_app->m_sepulveda->Say( "tutorial_40" );
                        g_app->m_sepulveda->Say( "tutorial_40b" );
                        RepeatMessage( "tutorial_40b" );
                        break;

        case 41:        g_app->m_sepulveda->Say( "tutorial_41" );
                        g_app->m_sepulveda->DemoGesture( "engineer.txt", 2 );
                        RepeatMessage( "tutorial_41", 30, "engineer.txt" );
                        break;
        
        case 42:        g_app->m_sepulveda->Say( "tutorial_42" );
                        RepeatMessage( "tutorial_42" );
                        break;

        case 43:        g_app->m_sepulveda->Say( "tutorial_43" );
                        g_app->m_sepulveda->Say( "tutorial_43b" );
                        RepeatMessage( "tutorial_43b" );
                        break;
    }
}


bool Demo1Tutorial::AdvanceCurrentChapter()
{
    bool result = Tutorial::AdvanceCurrentChapter();
    if( !result ) return false;

    //
    // Advance chapter specific code
    
    switch( m_chapter )
    {
        case 0:                                                                                                 break;

        case 1:                     // Camera Movement        
        {
            CameraMount *mount = g_app->m_location->m_levelFile->GetCameraMount( "gamestart" );
            float distance = ( mount->m_pos - g_app->m_camera->GetPos() ).Mag();
            if( distance > 100.0 ) m_nextChapterTimer = GetHighResTime() + 5.0;
            break;        
        }
        
        case 2:                     // Camera Height
        {
            CameraMount *mount = g_app->m_location->m_levelFile->GetCameraMount( "gamestart" );
            float difference = fabs( mount->m_pos.y - g_app->m_camera->GetPos().y );
            if( difference > 100.0 ) m_nextChapterTimer = GetHighResTime() + 10.0;
            break;
        }

        case 3:     if( !g_app->m_sepulveda->IsTalking() )          TriggerChapter(m_chapter+1);                break;       
        case 4:     if( g_app->m_taskManagerInterface->IsVisible() )  TriggerChapter(m_chapter+1);                break;

        case 5:                     // Summon squad
        {
            Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
            if( currentTask && currentTask->m_type == GlobalResearch::TypeSquad )
            {
                TriggerChapter(m_chapter+1);
            }
            else if( currentTask && currentTask->m_type != GlobalResearch::TypeSquad )
            {
                g_app->m_location->GetMyTeam()->m_taskManager->TerminateTask( currentTask->m_id );
                g_app->m_sepulveda->Say( "tutorial_5_error" );
            }
            break;
        }

        case 6:                         // Move cam to spawn point
        {
            if( !g_app->m_camera->IsAnimPlaying() )
            {
                TriggerChapter(m_chapter+1);
            }
            break;   
        }

        case 7:                      // Place squad
        {
            HandleSquadDeath();
            Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
            if( currentTask && 
                currentTask->m_type == GlobalResearch::TypeSquad &&
                currentTask->m_state == Task::StateRunning )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 8:                         // Move squad
        {
            Unit *unit = g_app->m_location->m_teams[2]->GetMyUnit();
            if( unit )
            {
                float distance = ( unit->GetWayPoint() - unit->m_centrePos ).Mag();
                if( distance > 50.0 )
                {
                    m_nextChapterTimer = GetHighResTime() + 5.0;
                }
            }
            break;
        }

        case 9:                         // Fire lasers
        {
            HandleSquadDeath();
            if( g_app->m_location->m_lasers.NumUsed() > 3 )
            {
                m_nextChapterTimer = GetHighResTime() + 5.0;
            }
            break;    
        }

        case 10:                        // Fire grenade
        {
            HandleSquadDeath();
            bool grenadeFound = false;
            for( int i = 0; i < g_app->m_location->m_effects.Size(); ++i )
            {
                if( g_app->m_location->m_effects.ValidIndex(i) )
                {
                    WorldObject *wobj = g_app->m_location->m_effects[i];
                    if( wobj->m_type == WorldObject::EffectThrowableGrenade )
                    {
                        grenadeFound = true;
                        break;
                    }
                }
            }
            if( grenadeFound )
            {
                m_nextChapterTimer = GetHighResTime() + 5.0;
            }
            break;
        }

        case 11:                        // Kill virii
        {
            HandleSquadDeath();
            Building *building = g_app->m_location->GetBuilding(52);            
            AppReleaseAssert( building, "Tutorial building not found" );
            int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( building->m_pos.x, building->m_pos.z, 300.0, 2 );
            if( numEnemies <= 20 ) TriggerChapter( m_chapter+1 );            
            break;
        }

        case 12:                        // Kill virii
        {
            HandleSquadDeath();
            Building *building = g_app->m_location->GetBuilding(52);            
            AppReleaseAssert( building, "Tutorial building not found" );
            int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( building->m_pos.x, building->m_pos.z, 300.0, 2 );
            if( numEnemies <= 5 ) TriggerChapter( m_chapter+1 );            
            break;
        }

        case 13:                        // Kill virii
        {
            HandleSquadDeath();
            Building *building = g_app->m_location->GetBuilding(52);            
            AppReleaseAssert( building, "Tutorial building not found" );
            int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( building->m_pos.x, building->m_pos.z, 300.0, 2 );
            if( numEnemies == 0 ) TriggerChapter( 20 );            
            break;
        }

        case 20:                        // Show spirits
            if( !g_app->m_sepulveda->IsTalking() )
            {
                TriggerChapter(m_chapter+1);
            }
            break;   

        case 21:                        // Summon Engineer
        {
            Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
            if( currentTask && currentTask->m_type == GlobalResearch::TypeEngineer )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }
        
        case 22:                        // Engineer near Control Tower
        {
            HandleEngineerDeath();
            Building *building = g_app->m_location->GetBuilding(52);            
            AppReleaseAssert( building, "Tutorial building not found" );
            int numFound;
            g_app->m_location->m_entityGrid->GetFriends( m_neighbours, building->m_pos.x, building->m_pos.z, 150.0, &numFound, 2 );
            bool engineerFound = false;
            for( int i = 0; i < numFound; ++i )
            {
                WorldObjectId id = m_neighbours[i];
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

        case 23:                        // Engineer reprograms Incubator
        {
            HandleEngineerDeath();
            ControlTower *tower = (ControlTower *) g_app->m_location->GetBuilding(52);
            if( tower && 
                tower->m_id.GetTeamId() == 2 &&
                tower->m_ownership > 98.0 )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 24:                        // Darwinians born
        {
            HandleEngineerDeath();
            Building *building = g_app->m_location->GetBuilding(97);            
            int numFound;
            g_app->m_location->m_entityGrid->GetFriends( m_neighbours, building->m_pos.x, building->m_pos.z, 100.0, &numFound, 2 );
            bool darwinianFound = false;
            for( int i = 0; i < numFound; ++i )
            {
                WorldObjectId id = m_neighbours[i];
                Entity *darwinian = g_app->m_location->GetEntitySafe( id, Entity::TypeDarwinian );
                if( darwinian )
                {
                    darwinianFound = true;
                    break;
                }
            }
            if( darwinianFound )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 25:     if( !g_app->m_sepulveda->IsTalking() )          TriggerChapter(m_chapter+1);                break;       
        case 26:     if( !g_app->m_sepulveda->IsTalking() )          TriggerChapter(30);                         break;       
        
        case 28:            // Summon squad
        {
            Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
            if( currentTask && currentTask->m_type == GlobalResearch::TypeSquad )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }
        
        case 29:            // Place squad
        {
            Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
            if( currentTask && 
                currentTask->m_type == GlobalResearch::TypeSquad &&
                currentTask->m_state == Task::StateRunning )
            {
                TriggerChapter(32);
            }
            break;
        }

        case 30:     
            m_nextChapterTimer = GetHighResTime() + 5.0;
            break;       

        case 31:     if( !g_app->m_sepulveda->IsTalking() )          TriggerChapter(m_chapter+1);                break;       

        case 32:                // Attack second batch
        {
            HandleSquadDeath2();
            Building *building = g_app->m_location->GetBuilding(12);            
            AppReleaseAssert( building, "Tutorial building not found" );
            int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( building->m_pos.x, building->m_pos.z, 400.0, 2 );
            if( numEnemies == 0 )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 33:        
            if( !g_app->m_sepulveda->IsTalking() ) TriggerChapter(m_chapter+1);
            break;
        
        case 34:                // Move engineer to second incubator
        {
            HandleEngineerDeath2();
            ControlTower *tower = (ControlTower *) g_app->m_location->GetBuilding(55);
            if( tower && 
                tower->m_id.GetTeamId() == 2 &&
                tower->m_ownership > 98.0 )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }
        
        case 35:    
            if( !g_app->m_sepulveda->IsTalking() ) TriggerChapter(351);
            break;

        case 351:                // Run Officer task
        {
            Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
            if( currentTask && 
                currentTask->m_type == GlobalResearch::TypeOfficer )
            {
                TriggerChapter(36);
            }
            break;            
        }

        case 36:                // Promote darwinian to officer
        {
            if( g_app->m_location->m_teams[2]->m_specials.Size() > 0 )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 37:                // Give goto orders
        {
            for( int i = 0; i < g_app->m_location->m_teams[2]->m_specials.Size(); ++i )
            {
                WorldObjectId id = *g_app->m_location->m_teams[2]->m_specials.GetPointer(i);
                Officer *officer = (Officer *) g_app->m_location->GetEntitySafe( id, Entity::TypeOfficer );
                if( officer && 
                    officer->m_orders == Officer::OrderGoto )
                {
                    m_nextChapterTimer = GetHighResTime() + 10;
                }
            }
            break;
        }

        case 38:                // Mine information
        {
            if( !g_app->m_sepulveda->IsTalking() )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }

        case 39:
            return true;
            break;

        case 40:                // Create a squad
        {
            Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
            if( currentTask && 
                currentTask->m_type == GlobalResearch::TypeSquad &&
                currentTask->m_state == Task::StateRunning )
            {
                TriggerChapter(32);
            }
            break;
        }

        case 41:                // Create engineer
        {
            Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
            if( currentTask && 
                currentTask->m_type == GlobalResearch::TypeEngineer )
            {
                TriggerChapter(m_chapter+1);
            }
            break;
        }
        
        case 42:                // Engineer selected
        {
            Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
            if( currentTask && 
                currentTask->m_type == GlobalResearch::TypeEngineer &&
                currentTask->m_state == Task::StateRunning )
            {
                TriggerChapter(34);
            }
            break;
        }

        case 43:                // Engineer selected
        {
            Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
            if( currentTask && 
                currentTask->m_type == GlobalResearch::TypeEngineer &&
                currentTask->m_state == Task::StateRunning )
            {
                TriggerChapter(34);
            }
            break;
        }
    }       

    return false;
}


void Demo1Tutorial::HandleSquadDeath()
{
    //
    // Ensure the currently running task is a fully functioning squad

    Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
    
    if( !currentTask || currentTask->m_type != GlobalResearch::TypeSquad )
    {
        // If the player deselected by accident, reselect now
        for( int i = 0; i < g_app->m_location->GetMyTeam()->m_taskManager->m_tasks.Size(); ++i )
        {
            Task *task = g_app->m_location->GetMyTeam()->m_taskManager->m_tasks[i];
            if( task && 
                task->m_type == GlobalResearch::TypeSquad )
            {
                g_app->m_location->GetMyTeam()->m_taskManager->SelectTask( task->m_id );
                return;
            }
        }

        // There are no squad tasks running.
        g_app->m_sepulveda->ShutUp();
        g_app->m_sepulveda->Say( "tutorial_squaddeath" );
        m_chapter = 4;
        m_nextChapterTimer = GetHighResTime() + 10;
    }
}


void Demo1Tutorial::HandleSquadDeath2()
{
    //
    // Ensure the currently running task is a fully functioning squad

    Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
    
    if( !currentTask || currentTask->m_type != GlobalResearch::TypeSquad )
    {
        // If the player deselected by accident, tell the player to reselect now
        for( int i = 0; i < g_app->m_location->GetMyTeam()->m_taskManager->m_tasks.Size(); ++i )
        {
            Task *task = g_app->m_location->GetMyTeam()->m_taskManager->m_tasks[i];
            if( task && 
                task->m_type == GlobalResearch::TypeSquad )
            {
                m_chapter = 39;
                m_nextChapterTimer = GetHighResTime();                
                return;
            }
        }

        // There are no squad tasks running.
        g_app->m_sepulveda->ShutUp();
        g_app->m_sepulveda->Say( "tutorial_squaddeath" );
        m_chapter = 27;
        m_nextChapterTimer = GetHighResTime() + 10;
    }
}


void Demo1Tutorial::HandleEngineerDeath()
{
    //
    // Ensure the currently running task is a fully functioning engineer

    Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
    
    if( !currentTask || currentTask->m_type != GlobalResearch::TypeEngineer )
    {
        // If the player deselected by accident, reselect now
        for( int i = 0; i < g_app->m_location->GetMyTeam()->m_taskManager->m_tasks.Size(); ++i )
        {
            Task *task = g_app->m_location->GetMyTeam()->m_taskManager->m_tasks[i];
            if( task && 
                task->m_type == GlobalResearch::TypeEngineer )
            {
                g_app->m_location->GetMyTeam()->m_taskManager->SelectTask( task->m_id );
                return;
            }
        }

        // There are no engineer tasks running.
        g_app->m_sepulveda->ShutUp();
        g_app->m_sepulveda->Say( "tutorial_engineerdeath" );
        m_chapter = 20;
        m_nextChapterTimer = GetHighResTime() + 10;
    }
}


void Demo1Tutorial::HandleEngineerDeath2()
{
    //
    // Ensure the currently running task is a fully functioning engineer

    Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
    
    if( !currentTask || currentTask->m_type != GlobalResearch::TypeEngineer )
    {
        // If the player deselected by accident, reselect now
        for( int i = 0; i < g_app->m_location->GetMyTeam()->m_taskManager->m_tasks.Size(); ++i )
        {
            Task *task = g_app->m_location->GetMyTeam()->m_taskManager->m_tasks[i];
            if( task && 
                task->m_type == GlobalResearch::TypeEngineer )
            {
                m_chapter = 42;
                m_nextChapterTimer = GetHighResTime();
                return;
            }
        }

        // There are no engineer tasks running.
        g_app->m_sepulveda->ShutUp();
        g_app->m_sepulveda->Say( "tutorial_engineerdeath" );
        m_chapter = 40;
        m_nextChapterTimer = GetHighResTime() + 10;
    }
}

#endif // USE_SEPULVEDA_HELP_TUTORIAL
