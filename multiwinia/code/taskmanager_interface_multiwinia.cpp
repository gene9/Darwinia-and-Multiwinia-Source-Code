#include "lib/universal_include.h"
#include "lib/text_renderer.h"
#include "lib/math_utils.h"
#include "lib/input/input.h"
#include "lib/targetcursor.h"
#include "lib/vector2.h"
#include "lib/debug_utils.h"
#include "lib/resource.h"
#include "lib/bitmap.h"
#include "lib/profiler.h"
#include "lib/hi_res_time.h"
#include "lib/language_table.h"
#include "lib/debug_render.h"
#include "lib/ogl_extensions.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/preferences.h"

#include <eclipse.h>

#include "network/clienttoserver.h"

#include "app.h"
#include "gamecursor.h"
#include "gesture.h"
#include "global_world.h"
#include "location.h"
#include "renderer.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "taskmanager_interface_multiwinia.h"
#include "team.h"
#include "unit.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "level_file.h"
#include "main.h"
#include "routing_system.h"
#include "entity_grid.h"
#include "particle_system.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "helpsystem.h"
#endif
#include "camera.h"
#include "script.h"
#include "location_input.h"
#include "control_help.h"
#include "shaman_interface.h"
#include "multiwiniahelp.h"
#include "multiwinia.h"

#include "interface/prefs_other_window.h"

#include "sound/soundsystem.h"

#include "worldobject/insertion_squad.h"
#include "worldobject/officer.h"
#include "worldobject/darwinian.h"
#include "worldobject/researchitem.h"
#include "worldobject/trunkport.h"
#include "worldobject/engineer.h"
#include "worldobject/harvester.h"
#include "worldobject/armour.h"
#include "worldobject/gunturret.h"



// ============================================================================


TaskManagerInterfaceMultiwinia::TaskManagerInterfaceMultiwinia()
:   m_chatLogY(0.0f),
    m_screenW(800),
    m_screenH(600),
    m_currentScreenZone(-1),
    m_screenZoneTimer(0.0f),
    m_currentMouseScreenZone(-1),
	m_currentScrollZone(1),
    m_taskManagerDownTime(0.0)
{

    m_viewingDefaultObjective = false;
    m_messageTimer = 0.0f;
    m_currentTaskType = -1;
    m_currentMessageType = -1;
    m_highlightedTaskId = -1;
    SetVisible( false );

    //
    // Pre-load all the graphics we intend to use for these screens
    // If we don't do this the interface screens are slow and sluggish first visit

    for( int i = 0; i < GlobalResearch::NumResearchItems; ++i )
    {
        char iconFilename[256];
        sprintf( iconFilename, "icons/icon_%s.bmp", GlobalResearch::GetTypeName(i) );
        if( g_app->m_resource->DoesTextureExist( iconFilename ) )
        {
            unsigned int texId = g_app->m_resource->GetTexture( iconFilename, true, false );
        }
        
        char gestureFilename[256];
        sprintf( gestureFilename, "icons/gesture_%s.bmp", GlobalResearch::GetTypeName(i) );
        if( g_app->m_resource->DoesTextureExist( gestureFilename ) )
        {
            unsigned int texId = g_app->m_resource->GetTexture( gestureFilename, true, false );
        }
    }

    g_app->m_resource->GetTexture( "textures/interface_grey.bmp", true, false );
    g_app->m_resource->GetTexture( "textures/interface_red.bmp", true, false );
    g_app->m_resource->GetTexture( "icons/gestureguide.bmp", true, false );
    g_app->m_resource->GetTexture( "icons/icon_shadow.bmp", true, false );


    //
    // Create keyboard shortcuts

    //m_keyboardShortcuts.PutData( new KeyboardShortcut("NewTask", GlobalResearch::TypeSquad,     ControlIconsTaskManagerNewSquad) );
    //m_keyboardShortcuts.PutData( new KeyboardShortcut("NewTask", GlobalResearch::TypeEngineer,  ControlIconsTaskManagerNewEngineer) );
    //m_keyboardShortcuts.PutData( new KeyboardShortcut("NewTask", GlobalResearch::TypeOfficer,   ControlIconsTaskManagerNewOfficer) );
    //m_keyboardShortcuts.PutData( new KeyboardShortcut("NewTask", GlobalResearch::TypeArmour,    ControlIconsTaskManagerNewArmour) );

    //m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectWeapon", GlobalResearch::TypeGrenade, ControlIconsTaskManagerSelectGrenade) );
    //m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectWeapon", GlobalResearch::TypeRocket, ControlIconsTaskManagerSelectRocket) );
    //m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectWeapon", GlobalResearch::TypeAirStrike, ControlIconsTaskManagerSelectAirStrike) );
    //m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectWeapon", GlobalResearch::TypeController, ControlIconsTaskManagerSelectController) );

    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 0, ControlTaskManagerSelectTask1) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 1, ControlTaskManagerSelectTask2) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 2, ControlTaskManagerSelectTask3) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 3, ControlTaskManagerSelectTask4) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 4, ControlTaskManagerSelectTask5) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 5, ControlTaskManagerSelectTask6) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 6, ControlTaskManagerSelectTask7) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 7, ControlTaskManagerSelectTask8) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 8, ControlTaskManagerSelectTask9) ); 

    m_keyboardShortcuts.PutData( new KeyboardShortcut("DeleteTask", -1, ControlTaskManagerEndTask) );
    
    m_keyboardShortcuts.PutData( new KeyboardShortcut("ScreenUp", -1, ControlUnitCycleLeft) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("ScreenDown", -1, ControlUnitCycleRight) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("ScreenUp", -1, ControlCameraForwards) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("ScreenDown", -1, ControlCameraBackwards) );
}

TaskManagerInterfaceMultiwinia::~TaskManagerInterfaceMultiwinia()
{
	m_newScreenZones.EmptyAndDelete();
	m_screenZones.EmptyAndDelete();
	m_keyboardShortcuts.EmptyAndDelete();
}

void TaskManagerInterfaceMultiwinia::AdvanceTerminate()
{
	if(!g_app->m_location->GetMyTeam()) return;
    if( EclGetWindows()->Size() > 0 ) return;

	if( g_inputManager->controlEvent( ControlIconsTaskManagerEndTask ) )
    {
        //g_app->m_location->GetMyTeam()->m_taskManager->TerminateTask( g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId );
        //g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId = -1;

        int taskId = g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId;
        g_app->m_clientToServer->RequestTerminateProgram( g_app->m_globalWorld->m_myTeamId, taskId );
    }
}


void TaskManagerInterfaceMultiwinia::HideTaskManager()
{
    //
    // Put the mouse to the middle of the screen
    float midX = g_app->m_renderer->ScreenW() / 2.0f;
    float midY = g_app->m_renderer->ScreenH() / 2.0f;
    g_target->SetMousePos( midX, midY );
    g_app->m_camera->Advance();
    
    Team *team = g_app->m_location->GetMyTeam();
    if( team )
    {
        if( team->m_taskManager->m_tasks.Size() > 0 )
        {
	        g_app->m_location->GetMyTeam()->m_taskManager->SelectTask( g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId );
        }
    }
    g_app->m_gesture->EndGesture();

    SetVisible( false );

    //g_app->m_soundSystem->TriggerOtherEvent( NULL, "Hide", SoundSourceBlueprint::TypeInterface );
}


void TaskManagerInterfaceMultiwinia::Advance()
{
    if( m_lockTaskManager ) return;

    AdvanceTab();
    
    if( !IsVisible() && 
        (g_inputManager->controlEvent(ControlIconsTaskManagerDisplay) ||
         g_inputManager->controlEvent(ControlIconsTaskManagerDisplayDown)))
    {
		// If the player has a Shaman selected, hijack the normal TaskManager and show the Shaman Interface instead

		Entity *e = NULL;
        if( g_app->m_location->GetMyTeam() )
        {
            e = g_app->m_location->GetMyTeam()->GetMyEntity();
        }

		if( e && e->m_type == Entity::TypeShaman )
		{
			g_app->m_shamanInterface->ShowInterface( g_app->m_location->GetMyTeam()->GetMyEntity()->m_id );
		}
		else if (g_app->IsSinglePlayer())
		{
			// Tab key just pressed
			// Pop up the task manager
			m_currentScrollZone = 1;

			SetVisible(); 
	                
			//g_app->m_soundSystem->TriggerOtherEvent( NULL, "Show", SoundSourceBlueprint::TypeInterface );
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
			g_app->m_helpSystem->PlayerDoneAction( HelpSystem::TaskBasics );
#endif

			if (g_inputManager->controlEvent(ControlIconsTaskManagerDisplayDown)) {
				// Controller activated the task manager, record the time 
				m_taskManagerDownTime = GetHighResTime();
			}
		}
    }    
    else if( m_visible && g_inputManager->controlEvent(ControlIconsTaskManagerHide) )
    {
        // Tab key pressed while visible
        // So remove the task manager
        HideTaskManager();
    }
	else if( g_app->m_shamanInterface->IsVisible() && g_inputManager->controlEvent( ControlIconsTaskManagerHide ) )
	{
		g_app->m_shamanInterface->HideInterface();
	}

    AdvanceScreenZones();
    AdvanceKeyboardShortcuts();
    AdvanceTerminate();

    if( m_viewingDefaultObjective
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
        && !g_app->m_sepulveda->IsTalking()
#endif
      )
    {
        // We were running a default objective description (trunk port, research item)
        // So shut it down now
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
        g_app->m_sepulveda->ClearHighlights( "RunDefaultObjective" );
#endif
        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
        m_viewingDefaultObjective = false;
    }


    TaskManager *tm = g_app->m_location->GetMyTaskManager();
    if( tm && 
        tm->m_mostRecentTaskId != -1 )
    {
        if( tm->m_mostRecentTaskIdTimer > 0 &&
            g_prefsManager->GetInt( "HelpEnabled", 1 ))
        {
            double timePassed = GetHighResTime() - tm->m_mostRecentTaskIdTimer;
            if( timePassed > 0.2f &&
                g_inputManager->controlEvent(ControlCloseTaskHelp) )
            {
                //tm->m_mostRecentTaskId = -1;
                //tm->m_mostRecentTaskIdTimer = GetHighResTime() * -1;
            }
        }
        else
        {
            double timePassed = GetHighResTime() - iv_abs(tm->m_mostRecentTaskIdTimer);
            if( timePassed >= 1.0f )
            {
                tm->m_mostRecentTaskId = -1;
            }
        }
    }
}


void TaskManagerInterfaceMultiwinia::SetupRenderMatrices ()
{
    /*
     *	This is designed to make rendering of the individual screens much easier.
     *  It maps the screen co-ordinates so that each renderer can simply render
     *  to a fixed size window, without having to worry about scrolling, screen resolution etc
     *  The window will be 800x600 on a normal screen
     *  And 960x600 on a widescreen screen
     */
    
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

    float screenRatio = (float) g_app->m_renderer->ScreenW() / (float) g_app->m_renderer->ScreenH();
    m_screenH = 600.0f;
    m_screenW = m_screenH * screenRatio;

	float left = 0;
	float top = 0;
	float right = left + m_screenW;
	float bottom = top + m_screenH;

    float overscan = 0.0f;
    left -= overscan;
    right += overscan;
    bottom += overscan;
    top -= overscan;

    gluOrtho2D(left, right, bottom, top);
	glMatrixMode(GL_MODELVIEW);
}


void TaskManagerInterfaceMultiwinia::ConvertMousePosition( float &_x, float &_y )
{
    float fractionX = _x / (float) g_app->m_renderer->ScreenW();
    float fractionY = _y / (float) g_app->m_renderer->ScreenH();

    _x = m_screenW * fractionX;
    _y = m_screenH * fractionY;
}


void TaskManagerInterfaceMultiwinia::RestoreRenderMatrices()
{
    g_app->m_renderer->SetupMatricesFor2D();
}


void TaskManagerInterfaceMultiwinia::Render()
{    
    if( g_app->m_editing || 
       !g_app->m_location ||
       EclGetWindows()->Size() ||
       g_app->m_hideInterface ) return;

    if( g_app->m_multiwinia->GameInGracePeriod() ||
        g_app->m_multiwinia->GameOver() ) return;

    Team *myTeam = g_app->m_location->GetMyTeam();
    if( myTeam )
    {
        Building *building = g_app->m_location->GetBuilding( myTeam->m_currentBuildingId );
        if( building && building->m_type == Building::TypeGunTurret )
        {
            return;
        }
    }

    START_PROFILE( "Render Taskman");
	    
    glEnable    ( GL_BLEND );
    glDisable   ( GL_CULL_FACE );

    if( g_app->m_locationId != -1 )
    {
        RenderTargetAreas();
    }
    
    g_gameFont.BeginText2D();
    SetupRenderMatrices();

    RenderRecentRunTask();
    RenderMessages();   
    RenderCurrentCrateHelp();

    if( m_visible )
    {        
        RenderRunningTasks();        
        SetupRenderMatrices();    
    }

    RestoreRenderMatrices();

    SetupRenderMatrices();
    RenderScreenZones();
    if( !m_visible
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
        && !g_app->m_sepulveda->IsInCutsceneMode()
#endif
      )
    {
        RenderRunningTasks();
    }
    RestoreRenderMatrices();

    //
    // Render a mouse cursor if we are visible
    // The normal one is obscured by the task manager

    if( m_visible )
    {
        float mouseX = g_target->X();
        float mouseY = g_target->Y();
        g_app->m_gameCursor->RenderStandardCursor( mouseX, mouseY );
    }

    g_gameFont.EndText2D();

    glEnable    ( GL_CULL_FACE );
    glDisable   ( GL_BLEND );

    END_PROFILE( "Render Taskman");
}


void TaskManagerInterfaceMultiwinia::RenderCurrentCrateHelp()
{
    TaskManager *tm = g_app->m_location->GetMyTaskManager();
    if( !tm ) return;
    Task *task = tm->GetCurrentTask();

    if( task && task->m_state == Task::StateStarted )
    {
        char stringId[256];
        sprintf( stringId, "crate_tooltip_%s", Task::GetTaskName(task->m_type) );
        strlwr(stringId);
        UnicodeString fullCaption;
        
        if( ISLANGUAGEPHRASE( stringId ) )
        {
            fullCaption = LANGUAGEPHRASE( stringId );
        }
        else
        {
            char tempCap[1024];
            sprintf(tempCap, "Missing String:\n%s", stringId );
            fullCaption = UnicodeString(tempCap);
        }


        if( task->m_type == GlobalResearch::TypeAirStrike )
        {
            if( task->m_option == 1 )
            {
                fullCaption = LANGUAGEPHRASE("crate_tooltip_napalmstrike");
            }
        }

        if( task->m_type == GlobalResearch::TypeGunTurret )
        {
            switch( task->m_option )
            {
                case 0: fullCaption = LANGUAGEPHRASE("crate_tooltip_gunturret");     break;
                case 1: fullCaption = LANGUAGEPHRASE("crate_tooltip_rocketturret");  break;
                case 3: fullCaption = LANGUAGEPHRASE("crate_tooltip_flameturret");   break;
                default:fullCaption = LANGUAGEPHRASE("crate_tooltip_gunturret");     break;
            }
        }


        float boxW = m_screenH * 0.35f;
        float boxY = 420;
        float fontH = 11;
        float fontG = fontH * 0.2f;

        LList <UnicodeString *> *wrapped = WordWrapText( fullCaption, boxW*1.75f, fontH );

        //wrapped->PutDataAtEnd( new UnicodeString(LANGUAGEPHRASE("multiwinia_deselect")) );

        float boxH = wrapped->Size() * (fontH + fontG) + 10;
        float fontX = m_screenW * 0.5f;
        float fontY = boxY + fontH + fontG - 7 + 5;

        glColor4f( 0.0f, 0.0f, 0.0f, 0.75f );
        glBegin( GL_QUADS );
            glVertex2f( fontX - boxW/2, boxY );
            glVertex2f( fontX + boxW/2, boxY );
            glVertex2f( fontX + boxW/2, boxY+boxH );
            glVertex2f( fontX - boxW/2, boxY+boxH );
        glEnd();

        glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
        glBegin( GL_LINE_LOOP );
            glVertex2f( fontX - boxW/2, boxY );
            glVertex2f( fontX + boxW/2, boxY );
            glVertex2f( fontX + boxW/2, boxY+boxH );
            glVertex2f( fontX - boxW/2, boxY+boxH );
        glEnd();

        for( int i = 0; i < wrapped->Size(); i++ )
        {
            UnicodeString *thisLine = wrapped->GetData(i);

            glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );        
            g_editorFont.SetRenderOutline(true);
            g_editorFont.DrawText2DCentre( fontX, fontY, fontH, *thisLine );

            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );        
            g_editorFont.SetRenderOutline(false);
            g_editorFont.DrawText2DCentre( fontX, fontY, fontH, *thisLine );

            fontY += fontH;
            fontY += fontG;
        }

        wrapped->EmptyAndDelete();
        delete wrapped;
    }
}


void TaskManagerInterfaceMultiwinia::AdvanceScreenZones()
{         
    //
    // Copy the newly generated screenzone list over
    // Remove the old set of screen zones

    m_screenZones.EmptyAndDelete();
    while( m_newScreenZones.Size() > 0 )
    {
        ScreenZone *zone = m_newScreenZones[0];
        m_screenZones.PutData( zone );
        m_newScreenZones.RemoveData(0);
		if( m_currentScreenZone == -1 &&
			zone->m_scrollZone == m_currentScrollZone )
		{
			m_currentScreenZone = m_screenZones.Size() -1;
		}
    }

    //
    // Where is the mouse right now
            
    bool found = false;

    if( m_visible )
    {
        for( int i = 0; i < m_screenZones.Size(); ++i )
        {
            ScreenZone *zone = m_screenZones[i];
            if( ScreenZoneHighlighted(zone) )
            {
                if( m_currentScreenZone != i &&
                    m_currentMouseScreenZone != i)
                {
                    m_currentMouseScreenZone = i;
                    m_currentScreenZone = i;
                    m_screenZoneTimer = GetHighResTime();
                    g_app->m_soundSystem->TriggerOtherEvent( NULL, "MouseOverIcon", SoundSourceBlueprint::TypeInterface );
                }
                found = true;
            }
        }

        if( g_inputManager->controlEvent( ControlMenuDown ) && 
			m_currentScrollZone != 3)
        {
			int numZones = m_screenZones.Size();
			int zonesRemaining = numZones;
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "MouseOverIcon", SoundSourceBlueprint::TypeInterface );
            while( zonesRemaining )
            {
                m_currentScreenZone = (m_currentScreenZone + 1) % numZones;
				zonesRemaining--;

                if ( m_screenZones.ValidIndex( m_currentScreenZone ) &&
                     m_screenZones[m_currentScreenZone]->m_scrollZone == m_currentScrollZone )
                    break;
            }
        }

        if( g_inputManager->controlEvent( ControlMenuUp ) && 
			m_currentScrollZone != 3) 
        {
			int numZones = m_screenZones.Size();
			int zonesRemaining = numZones;
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "MouseOverIcon", SoundSourceBlueprint::TypeInterface );
            while( zonesRemaining )
            {
                m_currentScreenZone = (m_currentScreenZone + numZones - 1) % numZones;
				zonesRemaining--;

                if ( m_screenZones.ValidIndex( m_currentScreenZone ) &&
                     m_screenZones[m_currentScreenZone]->m_scrollZone == m_currentScrollZone )
                    break;
            }
        }

		bool changeScrollZone = false;
		if( g_inputManager->controlEvent( ControlMenuLeft ) )
		{
			if( m_currentScrollZone == 1 )
			{
				m_currentScrollZone = 2;
				changeScrollZone = true;
			}
			else if( m_currentScrollZone == 3 )
			{
				while( true )
				{
					m_currentScreenZone--;
					if( m_currentScreenZone < 0 )
					{
						m_currentScrollZone = 2;
						changeScrollZone = true;
						break;
					}

					if((m_screenZones.ValidIndex( m_currentScreenZone ) &&
						m_screenZones[m_currentScreenZone]->m_scrollZone == m_currentScrollZone ) ||
						m_screenZones.Size() == 0 )
					{
						break;
					}
				}
			}
		}

		if( g_inputManager->controlEvent( ControlMenuRight) )
		{
			if( m_currentScrollZone == 2 )
			{
				if( m_screenZones[m_currentScreenZone]->m_subZones )
				{
					m_currentScrollZone = 3;
				}
				else
				{
					m_currentScrollZone = 1;
				}
				changeScrollZone = true;
			}
			else if( m_currentScrollZone == 3 )
			{
				while( true )
				{
					m_currentScreenZone++;
					if( m_currentScreenZone >= m_screenZones.Size() )
					{
						m_currentScrollZone = 1;
						changeScrollZone = true;
						break;
					}

					if((m_screenZones.ValidIndex( m_currentScreenZone ) &&
						m_screenZones[m_currentScreenZone]->m_scrollZone == m_currentScrollZone ) ||
						m_screenZones.Size() == 0 )
					{
						break;
					}
				}
			}
		}
		
		if( changeScrollZone )
		{
			m_currentScreenZone = -1;
			for( int i = 0; i < m_screenZones.Size(); ++i )
			{ 
				if( m_screenZones[i]->m_scrollZone == m_currentScrollZone )
				{
					m_currentMouseScreenZone = i;
					m_currentScreenZone = i;
					m_screenZoneTimer = GetHighResTime();
					g_app->m_soundSystem->TriggerOtherEvent( NULL, "MouseOverIcon", SoundSourceBlueprint::TypeInterface );
					break;
				}
			}
			if( m_currentScreenZone == -1 )
			{
				m_currentScrollZone = (m_currentScreenZone == 1 ) ? 2 : 1;
			}
		}
    }
    

    //
    // Special case - the player is highlighting a unit/entity in the real world
    // We want to highlight the button that corrisponds to that task ID
    // (if it exists)

    bool highlightOnly = false;

    if( !m_visible )
    {
        m_currentScreenZone = -1;
        WorldObjectId id;
        bool somethingHighlighted = g_app->m_locationInput->GetObjectUnderMouse( id, g_app->m_globalWorld->m_myTeamId );
        if( somethingHighlighted )
        {
            int taskIndex = -1;
			if(g_app->m_location->GetMyTeam())
            for( int i = 0; i < g_app->m_location->GetMyTeam()->m_taskManager->m_tasks.Size(); ++i )
            {
                if( g_app->m_location->GetMyTeam()->m_taskManager->m_tasks[i]->m_objId == id )
                {
                    taskIndex = i;
                    break;
                }
            }

            if( taskIndex != -1 )
            {
                for( int i = 0; i < m_screenZones.Size(); ++i )
                {
                    ScreenZone *zone = m_screenZones[i];
                    if( stricmp( zone->m_name, "SelectTask" ) == 0 && zone->m_data == taskIndex )
                    {
						if( g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD )
						{
							//m_currentScreenZone = i;
						}
                        found = true;
                        highlightOnly = true;
                    }
                }        
            }
        }
    }

    //if( !found ) m_currentScreenZone = -1;

    //
    // Are we highlighting a task?

    m_highlightedTaskId = -1;
    if( m_currentScreenZone != -1 )
    {
        ScreenZone *currentZone = m_screenZones[m_currentScreenZone];
        if( currentZone )
        {
            if( stricmp( currentZone->m_name, "SelectTask" ) == 0 &&
				g_app->m_location->GetMyTeam() &&
                g_app->m_location->GetMyTeam()->m_taskManager->m_tasks.ValidIndex(currentZone->m_data) )
            {
                Task *task = g_app->m_location->GetMyTeam()->m_taskManager->m_tasks[currentZone->m_data];
                m_highlightedTaskId = task->m_id;
            }
        }
    }


    //
    // Handle mouse clickage 

    if( m_currentScreenZone != -1 && 
        (g_inputManager->controlEvent( ControlActivateTMButton ) || ButtonHeldAndReleased()) &&
        !highlightOnly )
    {
        ScreenZone *currentZone = m_screenZones[m_currentScreenZone];
        if( currentZone )
        {
            RunScreenZone( currentZone->m_name, currentZone->m_data );
        }
    }
}

bool TaskManagerInterfaceMultiwinia::ButtonHeld()
{
	return GetHighResTime() - m_taskManagerDownTime > 0.5;
}

bool TaskManagerInterfaceMultiwinia::ButtonHeldAndReleased()
{
    return ButtonHeld() && g_inputManager->controlEvent( ControlIconsTaskManagerDisplayUp ) ;
}

void TaskManagerInterfaceMultiwinia::AdvanceKeyboardShortcuts()
{
    if( EclGetWindows()->Size() > 0 ) return;

    for( int i = 0; i < m_keyboardShortcuts.Size(); ++i )
    {
        KeyboardShortcut *shortcut = m_keyboardShortcuts[i];
        if( (*shortcut)() )
        {
            RunScreenZone( shortcut->name(), shortcut->data() );
        }
    }
}


void TaskManagerInterfaceMultiwinia::RunScreenZone( const char *_name, int _data )
{
	if(!g_app->m_location->GetMyTeam()) return;

    //
    // New task buttons 

    if( stricmp( _name, "NewTask" ) == 0 )
    {
        //
        // Prevent creation of new tasks if we're placing one
        
        Task *task = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
        if( task && 
            task->m_state == Task::StateStarted && 
            task->m_type != GlobalResearch::TypeOfficer )
        {
            SetCurrentMessage( MessageFailure, -1, 2.5f );
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
            g_app->m_sepulveda->Say( "task_manager_targetfirst" );
#endif
            return;
        }


        if( g_app->m_globalWorld->m_research->HasResearch( _data ) )
        {
            g_app->m_clientToServer->RequestRunProgram( g_app->m_globalWorld->m_myTeamId, _data );
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureBegin", SoundSourceBlueprint::TypeGesture );
			if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
			{
				HideTaskManager();
			}
        }
        else
        {
            SetCurrentMessage( MessageFailure, -1, 2.5f );
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
            g_app->m_sepulveda->Say( "research_notyetavailable" );
#endif
        }
    }


    //
    // Select task buttons

    if( stricmp( _name, "SelectTask" ) == 0 )
    {
        Team *myTeam = g_app->m_location->GetMyTeam();
        if( myTeam->m_taskManager->m_tasks.ValidIndex(_data) )
        {
            Task *nextTask = myTeam->m_taskManager->m_tasks[_data];
            if( nextTask->m_startTimer <= 0.0f )
            {
                g_app->m_clientToServer->RequestSelectProgram( g_app->m_globalWorld->m_myTeamId, nextTask->m_id );
                //g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId = nextTask->m_id;  
                //g_app->m_location->GetMyTeam()->m_taskManager->SelectTask( g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId );    

                g_app->m_soundSystem->TriggerOtherEvent( NULL, "SelectTask", SoundSourceBlueprint::TypeInterface );       
			    if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
			    {
				    HideTaskManager();
			    }
            }
        }
    }


    //
    // Delete task buttons

    if( stricmp( _name, "DeleteTask" ) == 0 )
    {
        if( _data == -1 )
        {
            g_app->m_location->GetMyTeam()->m_taskManager->TerminateTask( g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId );
        }
        else
        {
            g_app->m_location->GetMyTeam()->m_taskManager->TerminateTask( _data );        
        }
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "DeleteTask", SoundSourceBlueprint::TypeInterface );                  
    }


    //
    // Select weapon buttons

    if( stricmp( _name, "SelectWeapon" ) == 0 )
    {
        if( g_app->m_globalWorld->m_research->HasResearch( _data ) )
        {
            g_app->m_clientToServer->RequestRunProgram( g_app->m_globalWorld->m_myTeamId, _data );
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureBegin", SoundSourceBlueprint::TypeGesture );
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
			if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
			{
				HideTaskManager();
			}
        }
        else
        {
            SetCurrentMessage( MessageFailure, -1, 2.5f );
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
            g_app->m_sepulveda->Say( "research_notyetavailable" );
#endif
        }                
    }

    
    //
    // Research

    if( stricmp( _name, "Research" ) == 0 )
    {
        g_app->m_globalWorld->m_research->SetCurrentResearch( _data );
    }


    //
    // Objectives

    if( stricmp( _name, "Objective" ) == 0 )
    {
        bool primary = _data > 0;
        int objectiveId = _data;
        if( !primary ) objectiveId *= -1;
        objectiveId -= 10;

        GlobalEventCondition *gec = NULL;
        if( primary ) gec = g_app->m_location->m_levelFile->m_primaryObjectives[objectiveId];
        else          gec = g_app->m_location->m_levelFile->m_secondaryObjectives[objectiveId];
        AppDebugAssert( gec );

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
        g_app->m_sepulveda->ShutUp();
#endif
        if( gec->m_cutScene )
        {
            g_app->m_script->RunScript( gec->m_cutScene );
        }
        else
        {
            RunDefaultObjective( gec );
        }
		HideTaskManager();
    }
}


bool TaskManagerInterfaceMultiwinia::ScreenZoneHighlighted( ScreenZone *_zone )
{
    float mouseX = g_target->X();
    float mouseY = g_target->Y();
    ConvertMousePosition( mouseX, mouseY );

    return( mouseX >= _zone->m_x && 
            mouseY >= _zone->m_y &&
            mouseX <= _zone->m_x + _zone->m_w &&
            mouseY <= _zone->m_y + _zone->m_h );
}


void TaskManagerInterfaceMultiwinia::RenderScreenZones()
{
/*
    for( int i = 0; i < m_screenZones.Size(); ++i )
    {
        ScreenZone *zone = m_screenZones[i];
        
        float hX = zone->m_x;
        float hY = zone->m_y;
        float hW = zone->m_w;
        float hH = zone->m_h;

        hX -= m_screenY * m_screenW;

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        glBegin( GL_LINE_LOOP );
            glVertex2f( hX, hY );
            glVertex2f( hX+hW, hY );
            glVertex2f( hX+hW, hY+hH );
            glVertex2f( hX, hY+hH );
        glEnd();           
    }

*/

    if( m_screenZones.ValidIndex(m_currentScreenZone) )
    {
        ScreenZone *zone = m_screenZones[m_currentScreenZone];

        float overSpill = zone->m_h * 0.1f;
        float hX = zone->m_x - overSpill;
        float hY = zone->m_y - overSpill;
        float hW = zone->m_w + overSpill * 2;
        float hH = zone->m_h + overSpill * 2;

        RenderScreenZoneHighlight( hX, hY, hW, hH );        
    }
}


void TaskManagerInterfaceMultiwinia::RenderScreenZoneHighlight( float x, float y, float w, float h )
{
    if( g_app->Multiplayer() )
    {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/mouse_highlight.bmp" ) );
        glEnable( GL_TEXTURE_2D );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        glBegin( GL_QUADS );
            glTexCoord2f(0.0f,0.0f);      glVertex2f( x, y );
            glTexCoord2f(1.0f,0.0f);      glVertex2f( x + h, y );
            glTexCoord2f(1.0f,1.0f);      glVertex2f( x + h, y + h );
            glTexCoord2f(0.0f,1.0f);      glVertex2f( x, y + h );
        glEnd();

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable( GL_TEXTURE_2D );
    }
    else
    {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );

        glColor4f( 0.05f, 0.05f, 0.5f, 0.3f );

        glBegin( GL_QUADS );
            glVertex2f( x, y );
            glVertex2f( x+w, y );
            glVertex2f( x+w, y+h );
            glVertex2f( x, y+h );
        glEnd();

        glColor4f( 1.0f, 1.0f, 0.3f, 1.0f );
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/mouse_selection.bmp" ) );
        glEnable( GL_TEXTURE_2D );

        glBegin( GL_QUADS );
            glTexCoord2f(0.0f,0.0f);      glVertex2f( x, y );
            glTexCoord2f(0.5f,0.0f);      glVertex2f( x + h/2, y );
            glTexCoord2f(0.5f,1.0f);      glVertex2f( x + h/2, y + h );
            glTexCoord2f(0.0f,1.0f);      glVertex2f( x, y + h );

            glTexCoord2f(0.5f,0.0f);      glVertex2f( x + w - h/2, y );
            glTexCoord2f(1.0f,0.0f);      glVertex2f( x + w, y );
            glTexCoord2f(1.0f,1.0f);      glVertex2f( x + w, y + h );
            glTexCoord2f(0.5f,1.0f);      glVertex2f( x + w - h/2, y + h );            
        glEnd();

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable( GL_TEXTURE_2D );
    }
}


void TaskManagerInterfaceMultiwinia::RenderTooltip()
{
    g_gameFont.SetRenderShadow(true);
    glColor4ub( 255, 255, 150, 30 );

    if( m_screenZones.ValidIndex( m_currentScreenZone ) )
    {
        ScreenZone *zone = m_screenZones[m_currentScreenZone];

        //
        // Render tooltip caption

        float timeRequired = strlen(zone->m_toolTip) / 50.0f;
        float timeSoFar = GetHighResTime() - m_screenZoneTimer;

		UnicodeString clipped = LANGUAGEPHRASE(zone->m_toolTip);
        if( timeSoFar < timeRequired )
        {
            float fraction = timeSoFar / timeRequired;
            clipped.m_unicodestring[ int(clipped.Length() * fraction) ] = '\x0';
        }

        g_gameFont.DrawText2D( 20, m_screenH - 12, 12, clipped );
        g_gameFont.DrawText2D( 20, m_screenH - 12, 12, clipped );


		if (g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD) {
			//
			// Render keyboard shortcut

			KeyboardShortcut *selectedShortcut = NULL;

			for( int i = 0; i < m_keyboardShortcuts.Size(); ++i )
			{
				KeyboardShortcut *shortcut = m_keyboardShortcuts[i];
				if( stricmp( shortcut->name(), zone->m_name ) == 0 &&
					shortcut->data() == zone->m_data )
				{
					selectedShortcut = shortcut;
					break;
				}

				if( stricmp( shortcut->name(), zone->m_name ) == 0 &&
					stricmp( shortcut->name(), "DeleteTask" ) == 0 )
				{
					// Special case : DeleteTask shortcut key
					selectedShortcut = shortcut;
					break;
				}
			}

			if( selectedShortcut )
			{
				char caption[256];

				sprintf( caption, " : %s", 
					selectedShortcut->noun().c_str() );

                UnicodeString cap = LANGUAGEPHRASE("help_keyboardshortcut") + UnicodeString(caption);
	            
				g_gameFont.DrawText2D( m_screenW - 250, m_screenH - 12, 12, cap );
				g_gameFont.DrawText2D( m_screenW - 250, m_screenH - 12, 12, cap );
			}
		}
    }

//    else
//    {
//        //
//        // How do I escape the task manager
//
//        char *toolTip = LANGUAGEPHRASE("newcontrols_hidetaskmanager");
//        glColor4ub( 255, 255, 150, 50 );
//        g_gameFont.DrawText2D( 20, m_screenH - 12, 12, toolTip );
//    }
    
    g_gameFont.SetRenderShadow(false);

}


void TaskManagerInterfaceMultiwinia::RenderMessages()
{
    if( m_currentMessageType != -1 )
    {
        //
        // Are we done yet?

        if( GetHighResTime() >= m_messageTimer )
        {
            m_currentMessageType = -1;
            return;
        }


        //
        // Lookup message portion

        char currentMessageStringId[256];        
        sprintf( currentMessageStringId, "taskmanager_msg%d", m_currentMessageType );
        if( !ISLANGUAGEPHRASE( currentMessageStringId ) )
        {
            m_currentMessageType = -1;
            return;
        }

		const char * message =  currentMessageStringId;

        
        //
        // Lookup task name

        //char *taskName = NULL;

        UnicodeString taskName;
        
        if( m_currentTaskType == 999 )
        {
            taskName = LANGUAGEPHRASE("taskmanager_mapeditor");
        }
        else if( m_currentTaskType == 998 )
        {
            taskName = LANGUAGEPHRASE("taskmanager_accessallareas");
        }
        else if( m_currentTaskType != -1 )
        {
            if( m_currentTaskType == GlobalResearch::TypeGunTurret )
            {
                if( g_app->m_location->GetMyTaskManager()->GetCurrentTask() )
                {
                    int turretType = g_app->m_location->GetMyTaskManager()->GetCurrentTask()->m_option;
                    taskName = LANGUAGEPHRASE(GunTurret::GetTurretTypeName( turretType ));
                }
            }
            else if( m_currentTaskType == GlobalResearch::TypeAirStrike )
            {
                if( g_app->m_location->GetMyTaskManager()->GetCurrentTask() )
                {
                    bool napalm = (g_app->m_location->GetMyTaskManager()->GetCurrentTask()->m_option == 1);
                    if( napalm )
                    {
                        taskName = LANGUAGEPHRASE("crate_napalmstrike");//LANGUAGEPHRASE("crate_napalmstrike");
                    }
                    else
                    {
                        Task::GetTaskNameTranslated(m_currentTaskType, taskName);//Translated( m_currentTaskType );            
                    }
                }
            }
            else
            {
                Task::GetTaskNameTranslated(m_currentTaskType, taskName);//Translated( m_currentTaskType );            
            }
        }


        //
        // Build string

        UnicodeString fullMessage( 1024 );
        if( taskName )
        {
            if( m_currentMessageType == MessageResearchUpgrade )
            {
                int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( m_currentTaskType );
				fullMessage.swprintf( L"%ls: %ls v%d.0", LANGUAGEPHRASE(message).m_unicodestring, taskName.m_unicodestring, researchLevel );
            }
			else if( m_currentMessageType == MessageSuccess )
			{
				fullMessage.swprintf( L"%ls", taskName.m_unicodestring );
			}
			else if( m_currentMessageType == MessageShutdown )
			{
                fullMessage = LANGUAGEPHRASE("dialog_terminated");
                fullMessage.ReplaceStringFlag(L'P', taskName );
			}
            else
            {
				fullMessage.swprintf( L"%ls: %ls", LANGUAGEPHRASE(message).m_unicodestring, taskName.m_unicodestring ); 
            }
        }
        else
        {
			fullMessage.swprintf( L"%ls", LANGUAGEPHRASE(message).m_unicodestring );
        }

        //
        // Render string

        float timeRemaining = m_messageTimer - GetHighResTime();
        float alpha = timeRemaining * 0.5f;
        alpha = min( alpha, 1.0f );
        alpha = max( alpha, 0.0f );        
        float size = 40.0f;
        if( timeRemaining < 2.0f ) 
        {
            //size = iv_sqrt( 2.7f - timeRemaining ) * 30.0f;        
            size += iv_sqrt( 2.0f - timeRemaining ) * 15.0f;
        }

        float outlineAlpha = alpha * alpha * alpha;

        g_titleFont.SetRenderOutline(true);
        glColor4f( outlineAlpha, outlineAlpha, outlineAlpha, 0.0f );
        g_titleFont.DrawText2DCentre( m_screenW/2.0f, 370.0f, size, fullMessage );
        
        g_titleFont.SetRenderOutline(false);
        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        g_titleFont.DrawText2DCentre( m_screenW/2.0f, 370.0f, size, fullMessage );
    }
}


void TaskManagerInterfaceMultiwinia::RenderTargetAreas()
{
    if( !g_app->m_location->GetMyTeam() ) return;

    Task *task = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();

    if( task && 
        task->m_state == Task::StateStarted )    
    {
        LList<TaskTargetArea> *targetAreas = g_app->m_location->GetMyTeam()->m_taskManager->GetTargetArea( task->m_id );
        RGBAColour *colour = &g_app->m_location->GetMyTeam()->m_colour;
        
        if( targetAreas->Size() == 1 &&
            targetAreas->GetPointer(0)->m_centre == g_zeroVector &&
            targetAreas->GetPointer(0)->m_radius > 9999 )
        {
			delete targetAreas;
			return;
        }

        for( int i = 0; i < targetAreas->Size(); ++i )
        {
            TaskTargetArea *tta = targetAreas->GetPointer(i);

            if( tta->m_stationary )
            {
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
                //g_app->m_sepulveda->HighlightPosition( tta->m_centre, tta->m_radius, "TaskTargetAreas" );
#endif
            }
            
            float angle = g_gameTime * 3.0f;
            Vector3 dif( tta->m_radius * iv_sin(angle), 0.0f, tta->m_radius * iv_cos(angle) );
            
            Vector3 pos = tta->m_centre + dif;
            pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) + 5.0f;
            g_app->m_particleSystem->CreateParticle( pos, g_zeroVector, Particle::TypeMuzzleFlash, 60.0f );

            pos = tta->m_centre - dif;
            pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) + 5.0f;
            g_app->m_particleSystem->CreateParticle( pos, g_zeroVector, Particle::TypeMuzzleFlash, 60.0f );
        }

        delete targetAreas;
    }
    else
    {
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
        g_app->m_sepulveda->ClearHighlights( "TaskTargetAreas" );
#endif
    }

}


static void	RenderIcon( const char *_foreground, const char *_background, int _x, int _y, float _iconSize, unsigned _alpha)
{
	// Render the shadow
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( _background ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glDepthMask     ( false );
    glColor4ub      ( _alpha, _alpha, _alpha, 0.0f );         
	
    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );           glVertex2f( _x, _y );
        glTexCoord2i( 1, 1 );           glVertex2f( _x + _iconSize, _y );
        glTexCoord2i( 1, 0 );           glVertex2f( _x + _iconSize, _y + _iconSize );
        glTexCoord2i( 0, 0 );           glVertex2f( _x, _y + _iconSize );
    glEnd();	

	// Render the icon
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( _foreground ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    
    glColor4ub		( 255, 255, 255, _alpha );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );           glVertex2f( _x, _y );
        glTexCoord2i( 1, 1 );           glVertex2f( _x + _iconSize, _y );
        glTexCoord2i( 1, 0 );           glVertex2f( _x + _iconSize, _y + _iconSize );
        glTexCoord2i( 0, 0 );           glVertex2f( _x, _y + _iconSize );
    glEnd();
}


void TaskManagerInterfaceMultiwinia::RenderRecentRunTask ()
{
    m_showingRecentTask = false;
    if( !g_app->m_location->GetMyTeam() ) return;
    if( g_app->m_multiwinia->GameOver() ) return;

    TaskManager *manager = g_app->m_location->GetMyTaskManager();
    Team *team = g_app->m_location->GetMyTeam();

    float overallScale = 0.4f;

    if( manager && team )
    {
        int previousTaskType = -1;

        for( int i = 0; i < manager->m_recentTasks.Size(); ++i )
        {
            if( manager->m_recentTasks.ValidIndex(i) )
            {
                TaskSource *ts = manager->m_recentTasks[i];
                Task *task = manager->GetTask( ts->m_taskId );
                if( task )
                {
                    m_showingRecentTask = true;
                    char bmpFilename[256];
                    sprintf( bmpFilename, "icons/icon_%s.bmp", Task::GetTaskName(task->m_type) );
                    unsigned int texId = g_app->m_resource->GetTexture( bmpFilename );

                    bool minimising = ts->m_timer < 0.0f;                
                    float timePassed = GetHighResTime() - iv_abs(ts->m_timer);
                    if( timePassed < 3 ) ts->m_rotation += g_advanceTime * 20.0f * pow( 3 - timePassed, 4 );
                    float scaleFactor = min( timePassed * 3.0f, 1.0f );
                    float thisAlpha = min( timePassed * 2.0f, 1.0f );

                    Vector2 iconCentre( m_screenW/2.0f, m_screenH * 1/4.1f );        
                    float iconSize = scaleFactor * m_screenH/5.0f * overallScale;

                    //float boxW = scaleFactor * m_screenH/1.3f * overallScale;                
                    float boxW = 10;
                    float boxH = scaleFactor * m_screenH/4.2f * overallScale;
                    float shadowOffset = 0;
                    float shadowSize = iconSize * 1.25f;   
                    float shadowAlpha = thisAlpha * 0.9f;
                    float colourSize = iconSize;
                    float crateSize = iconSize * 1.1f;
                    float fontH = boxH * 0.3f;

                    iconCentre.y += (boxH * 1.1f * i);
                    previousTaskType = task->m_type;

                    float fontY = iconCentre.y - fontH * 0.4f;

                    Vector2 boxPos = iconCentre;
                    iconCentre.x = boxPos.x - boxW/2.0f + iconSize/1.5f;
                    
                    UnicodeString specialCaption;
                    if( !minimising )
                    {
					    UnicodeString name;
                        bool nameFound = false;
                        if( task->m_type == GlobalResearch::TypeAirStrike )
                        {
                            if( task->m_option == 1 )
                            {
                                name = LANGUAGEPHRASE("cratename_napalmstrike");
                                specialCaption = LANGUAGEPHRASE("crate_tooltip_napalmstrike");
                                nameFound = true;
                            }
                        }

                        if( task->m_type == GlobalResearch::TypeGunTurret )
                        {
                            name = LANGUAGEPHRASE(GunTurret::GetTurretTypeName(task->m_option));
                            switch( task->m_option )
                            {
                                case 0: specialCaption = LANGUAGEPHRASE("crate_tooltip_gunturret");     break;
                                case 1: specialCaption = LANGUAGEPHRASE("crate_tooltip_rocketturret");  break;
                                case 3: specialCaption = LANGUAGEPHRASE("crate_tooltip_flameturret");   break;
                                default:specialCaption = LANGUAGEPHRASE("crate_tooltip_gunturret");     break;
                            }
                            nameFound = true;
                        }

                        if( !nameFound )
                        {
					        Task::GetTaskNameTranslated( task->m_type, name );
                        }

                        name.StrUpr();

                        UnicodeString cratePowerupFound;
                        switch( ts->m_source )
                        {
                            case 0:     cratePowerupFound = LANGUAGEPHRASE("multiwinia_cratepowerupfound");         break;
                            case 1:     cratePowerupFound = LANGUAGEPHRASE("multiwinia_reinforcementsgiven");       break;
                            case 2:     cratePowerupFound = LANGUAGEPHRASE("multiwinia_retributiongiven");          break;
                        }

					    float texW = g_titleFont.GetTextWidth( name.Length(), fontH );
                        float crateOpenedW = g_titleFont.GetTextWidth( cratePowerupFound.Length(), fontH*0.7f );
                        float biggerW = max( texW, crateOpenedW );
                        if( biggerW + iconSize > boxW )
                        {
                            boxW = biggerW + iconSize * 1.5f;
                            iconCentre.x = boxPos.x - boxW/2.0f + iconSize/1.5f;
                        }
                       

                        //
                        // Render box

                        glColor4f ( 0.0f, 0.0f, 0.0f, 0.9f );
                        glBegin( GL_QUADS );
                            glVertex2f( boxPos.x - boxW/2, boxPos.y - boxH/2 );
                            glVertex2f( boxPos.x + boxW/2, boxPos.y - boxH/2 );
                            glVertex2f( boxPos.x + boxW/2, boxPos.y + boxH/2 );
                            glVertex2f( boxPos.x - boxW/2, boxPos.y + boxH/2 );
                        glEnd();

                        glShadeModel(GL_SMOOTH);
                        glBegin( GL_QUADS );
                            glColor4f ( 0.0f, 0.0f, 0.0f, 0.3f );
                            glVertex2f( boxPos.x - boxW/2, boxPos.y - boxH/2 );
                            glVertex2f( boxPos.x + boxW/2, boxPos.y - boxH/2 );
                            glColor4f ( 1.0f, 1.0f, 1.0f, 0.3f );
                            glVertex2f( boxPos.x + boxW/2, boxPos.y + boxH/2 );
                            glVertex2f( boxPos.x - boxW/2, boxPos.y + boxH/2 );
                        glEnd();
                        glShadeModel(GL_FLAT);

                        glColor4f ( 1.0f, 1.0f, 1.0f, 0.4f );
                        glBegin( GL_LINE_LOOP );
                            glVertex2f( boxPos.x - boxW/2, boxPos.y - boxH/2 );
                            glVertex2f( boxPos.x + boxW/2, boxPos.y - boxH/2 );
                            glVertex2f( boxPos.x + boxW/2, boxPos.y + boxH/2 );
                            glVertex2f( boxPos.x - boxW/2, boxPos.y + boxH/2 );
                        glEnd();


                        //
                        // Title

                        /*glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                        g_editorFont.SetRenderOutline(true);
                        g_editorFont.DrawText2DCentre( iconCentre.x, fontY+fontH, fontH*0.4f, LANGUAGEPHRASE("multiwinia_cratepowerupfound") );

                        glColor4f( 1.0f, 1.0f, 1.0f, 0.75f );
                        g_editorFont.SetRenderOutline(false);
                        g_editorFont.DrawText2DCentre( iconCentre.x, fontY+fontH, fontH*0.4f, LANGUAGEPHRASE("multiwinia_cratepowerupfound") );*/


                        float fontX = iconCentre.x + iconSize/1.5f;

                        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                        g_titleFont.SetRenderOutline(true);
                        g_titleFont.DrawText2D( fontX, fontY, fontH*0.7f, cratePowerupFound );

                        glColor4f( 0.7f, 0.7f, 0.7f, 1.0f );
                        g_titleFont.SetRenderOutline(false);
                        g_titleFont.DrawText2D( fontX, fontY, fontH*0.7f, cratePowerupFound );

					    // Adjust for non-English
					    float languageAdjust = 1.0f;
					    if( stricmp( g_prefsManager->GetString( "TextLanguage", "english" ), "english" ) != 0 )
						    languageAdjust = 0.8f; 
    					
                        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                        g_titleFont.SetRenderOutline(true);
                        g_titleFont.DrawText2D( fontX, fontY+fontH*0.75f, fontH * languageAdjust, name );

                        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                        g_titleFont.SetRenderOutline(false);
                        g_titleFont.DrawText2D( fontX, fontY+fontH*0.75f, fontH * languageAdjust, name );
                    }



                    if( minimising )
                    {                    
                        Vector2 posDiff = Vector2(task->m_screenX, task->m_screenY) - iconCentre;
                        posDiff *= min(timePassed * 2.0,1.0);
                        iconCentre += posDiff;

                        iconSize = m_screenH/6.0f;
                        float sizeDif = task->m_screenH - iconSize;
                        iconSize += sizeDif * min(timePassed * 2.0,1.0);

                        thisAlpha *= (1.0f - min(timePassed,1.0f));
                    }

                    //
                    // Shadow icon

                    glEnable        ( GL_TEXTURE_2D );
                    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/icon_shadow.bmp" ) );
                    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
                    glDepthMask     ( false );
                    glColor4f       ( shadowAlpha, shadowAlpha, shadowAlpha, 0.0f );           

                    glBegin( GL_QUADS );
                        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowSize/2 + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
                        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowSize/2 + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
                        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowSize/2 + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
                        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowSize/2 + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
                    glEnd();

                    //
                    // Colour fill

                    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/marker_colour.bmp" ) );
                    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

                    RGBAColour colour = team->m_colour;
                    colour.r *= 0.6f;
                    colour.g *= 0.6f;
                    colour.b *= 0.6f;
                    glColor4ubv     ( colour.GetData() );           

                    glBegin( GL_QUADS );
                        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - colourSize/2 + shadowOffset, iconCentre.y - colourSize/2 + shadowOffset );
                        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + colourSize/2 + shadowOffset, iconCentre.y - colourSize/2 + shadowOffset );
                        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + colourSize/2 + shadowOffset, iconCentre.y + colourSize/2 + shadowOffset );
                        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - colourSize/2 + shadowOffset, iconCentre.y + colourSize/2 + shadowOffset );
                    glEnd();


                    //
                    // Generic Crate icon

                    if( !minimising && ts->m_source == 0 )
                    {
                        glEnable        ( GL_TEXTURE_2D );
                        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/icon_crate.bmp" ) );
                        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
                        glDepthMask     ( false );
                        glColor4f       ( 1.0f, 1.0f, 1.0f, 0.15f );           

                        Vector3 offset( crateSize*1.15f, 0, crateSize*1.15f );
                        offset.RotateAroundY( ts->m_rotation * -0.02f );

                        g_app->m_renderer->Clip( boxPos.x - boxW/2, boxPos.y - boxH/2, boxW, boxH );

                        glBegin( GL_QUADS );    
                            glTexCoord2i( 0, 1 );           glVertex2dv( (iconCentre + offset).GetData() );     offset.RotateAroundY(0.5f*M_PI);
                            glTexCoord2i( 1, 1 );           glVertex2dv( (iconCentre + offset).GetData() );     offset.RotateAroundY(0.5f*M_PI);
                            glTexCoord2i( 1, 0 );           glVertex2dv( (iconCentre + offset).GetData() );     offset.RotateAroundY(0.5f*M_PI);
                            glTexCoord2i( 0, 0 );           glVertex2dv( (iconCentre + offset).GetData() );
                        glEnd();

                        g_app->m_renderer->EndClip();
                    }

                    /*glColor4f       ( 1.0f, 1.0f, 1.0f, 0.1f );           
                    offset.Set( crateSize/2, 0, crateSize/2 );
                    offset.RotateAroundY( g_gameTimer.GetAccurateTime() * -0.3f );

                    glBegin( GL_QUADS );
                        glTexCoord2i( 0, 1 );           glVertex2dv( (iconCentre + offset).GetData() );     offset.RotateAroundY(0.5f*M_PI);
                        glTexCoord2i( 1, 1 );           glVertex2dv( (iconCentre + offset).GetData() );     offset.RotateAroundY(0.5f*M_PI);
                        glTexCoord2i( 1, 0 );           glVertex2dv( (iconCentre + offset).GetData() );     offset.RotateAroundY(0.5f*M_PI);
                        glTexCoord2i( 0, 0 );           glVertex2dv( (iconCentre + offset).GetData() );
                    glEnd();*/

                    
                    //
                    // Icon itself 

                    glDepthMask     ( true );
                    glBindTexture   ( GL_TEXTURE_2D, texId );
                    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

                    glColor4f   ( 1.0f, 1.0f, 1.0f, thisAlpha );        

                    glBegin( GL_QUADS );
                        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y - iconSize/2 );
                        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y - iconSize/2 );
                        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y + iconSize/2 );
                        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y + iconSize/2 );
                    glEnd();

                    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                    glDisable       ( GL_TEXTURE_2D );                
                }
            }
        }
    }
}


void TaskManagerInterfaceMultiwinia::RenderRunningTasks()
{
	Team *myTeam = g_app->m_location->GetMyTeam();
    if( !myTeam ) return;

    int numSlots = myTeam->m_taskManager->Capacity();
    int numTasks = myTeam->m_taskManager->m_tasks.Size();

	int specialTaskId = -1;
	int specialTaskType = -1;
	/*if( myTeam->m_numDarwiniansSelected > 0 )
	{
		bool specialDarwinianSelection = false;
		Task *task = myTeam->m_taskManager->GetCurrentTask();
		if( task )
		{
			if( task->m_type == GlobalResearch::TypeSubversion ||
				task->m_type == GlobalResearch::TypeHotFeet ||
				task->m_type == GlobalResearch::TypeBooster )
			{
				specialDarwinianSelection = true;
			}
		}

		if( !specialDarwinianSelection )
		{
			specialTaskType = Entity::TypeDarwinian;
			numTasks ++;
			specialTaskId = myTeam->m_taskManager->m_tasks.Size();
		}
	}
		 
	Entity *specialEntity = myTeam->GetMyEntity();
	if( specialEntity && specialEntity->m_type == Entity::TypeOfficer )
	{
		specialTaskType = Entity::TypeOfficer;
		numTasks++;
		specialTaskId = myTeam->m_taskManager->m_tasks.Size();
	}*/

    Entity *specialEntity = NULL;

	static float s_iconSize = 60;
    static float s_iconY = 550;
    float s_iconGap = 10;
    static float s_iconX = m_screenW/2 - (s_iconSize + s_iconGap) * numTasks * 0.5f + s_iconSize/2.0f + s_iconGap/2.0f;

    float desiredIconSize = 55;
    float desiredIconX = m_screenW/2 - (s_iconSize + s_iconGap) * numTasks * 0.5f + s_iconSize/2.0f + s_iconGap/2.0f;
    float desiredIconY = 550;

    //if( m_visible )
    //{
    //    desiredIconSize = 100;
    //    desiredIconY = 400;
    //    desiredIconX = m_screenW/2 - (s_iconSize + s_iconGap) * numTasks * 0.5f + s_iconSize/2.0f + s_iconGap/2.0f;
    //}

    float factor = min(g_advanceTime, 0.1) * 10;
    s_iconSize = ( s_iconSize * (1-factor) ) + ( desiredIconSize * factor );
    s_iconX = ( s_iconX * (1-factor) ) + ( desiredIconX * factor );
    s_iconY = ( s_iconY * (1-factor) ) + ( desiredIconY * factor );

    float iconX = s_iconX;
    float iconY = s_iconY;
    float iconSize = s_iconSize;
    float shadowOffset = 0;
    float shadowSize = iconSize + 10;   
    float iconAlpha = 0.9f;
	float compassSize = iconSize * 0.77f;

    float sizeChangeIfSelected = 1.1f;

    int currentTaskId = myTeam->m_taskManager->GetTaskIndex( myTeam->m_taskManager->m_currentTaskId );


	//
	// Render the multiwinia help 

	if( specialTaskType != -1 )
	{
		float helpX = s_iconX + ( iconSize + s_iconGap ) * myTeam->m_taskManager->m_tasks.Size();
		float helpY = s_iconY;
		//g_app->m_multiwiniaHelp->RenderCurrentUnitHelp( helpX, helpY, iconSize, specialTaskType );
	}
    else if( myTeam->m_taskManager->m_currentTaskId != -1 )
    {
        int taskIndex = -1;
        for( int i = 0; i < myTeam->m_taskManager->m_tasks.Size(); ++i )
        {
            if( myTeam->m_taskManager->m_tasks[i]->m_id == myTeam->m_taskManager->m_currentTaskId )
            {
                taskIndex = i;
                break;
            }
        }
        float helpX = s_iconX + ( iconSize + s_iconGap ) * taskIndex;
		float helpY = s_iconY;
        //g_app->m_multiwiniaHelp->RenderCurrentTaskHelp( helpX, helpY, iconSize, myTeam->m_taskManager->m_currentTaskId );
    }


    if( g_app->m_location->GetMyTeam() &&
        g_app->m_location->GetMyTeam()->m_taskManager->m_tasks.Size() )
    {
        glEnable        ( GL_TEXTURE_2D );
        glEnable        ( GL_BLEND );
        glDepthMask     ( false );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glColor4f       ( 0.0f, 0.0f, 0.0f, 0.75f );           

        float tabSize = iconSize * 0.4f;
        float tabShadowSize = tabSize + 1;
        Vector2 iconCentre( iconX - iconSize * 1.2f, iconY + iconSize/2.0f - tabSize/2.0f );

        if( g_inputManager->getInputMode() == INPUT_MODE_KEYBOARD )
        {
            glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_tab.bmp" ) );
        }
        else
        {
            glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_lb.bmp" ) );
        }

        glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - tabShadowSize, iconCentre.y - tabShadowSize );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + tabShadowSize, iconCentre.y - tabShadowSize );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + tabShadowSize, iconCentre.y + tabShadowSize );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - tabShadowSize, iconCentre.y + tabShadowSize );
        glEnd();

        glColor4f       ( 1.0f, 1.0f, 1.0f, 0.75f );           
        
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - tabSize, iconCentre.y - tabSize );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + tabSize, iconCentre.y - tabSize );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + tabSize, iconCentre.y + tabSize );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - tabSize, iconCentre.y + tabSize );
        glEnd();
    }


    //
    // Render shadows for available task slots

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTextureWithAlpha( "icons/marker_shadow.bmp" ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDepthMask     ( false );
    glColor4f       ( 0.0f, 0.0f, 0.0f, iconAlpha * 0.9f );           
    
    for( int i = 0; i < numTasks; ++i )
    {
        Vector2 iconCentre( iconX, iconY );        
        
        if( i == currentTaskId )        shadowSize *= sizeChangeIfSelected;
        else                            shadowSize /= sizeChangeIfSelected;

        glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowSize/2.2f + shadowOffset, iconCentre.y - shadowSize/2.2f + shadowOffset );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowSize/2.2f + shadowOffset, iconCentre.y - shadowSize/2.2f + shadowOffset );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowSize/2.2f + shadowOffset, iconCentre.y + shadowSize/2.2f + shadowOffset );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowSize/2.2f + shadowOffset, iconCentre.y + shadowSize/2.2f + shadowOffset );
        glEnd();

        if( i == currentTaskId )        shadowSize /= sizeChangeIfSelected;
        else                            shadowSize *= sizeChangeIfSelected;

        iconX += iconSize;
        iconX += s_iconGap;
    }

    iconX = s_iconX;


    //
    // Render colour circles

    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTextureWithAlpha( "icons/marker_colour.bmp" ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

    for( int i = 0; i < numTasks; ++i )
    {
        RGBAColour colour = myTeam->m_colour; 
        
        Task *task = NULL;
		if( i != specialTaskId ) 
		{
			task = myTeam->m_taskManager->m_tasks[i];
		}

        if( task && task->m_startTimer > 0.0f )
        {
            iconX += iconSize;
            iconX += s_iconGap;
            continue;
        }

        if( task && task->m_type == GlobalResearch::TypeGunTurret )
        {
            Building *b = g_app->m_location->GetBuilding( task->m_objId.GetUniqueId() );
            if( b )
            {
                if( b->m_id.GetTeamId() == 255 ) 
                {
                    iconX += iconSize;
                    iconX += s_iconGap;
                    continue;
                }
                else
                {
                    colour = g_app->m_location->m_teams[b->m_id.GetTeamId()]->m_colour;
                }
            }
        }


        if( task->m_state != Task::StateRunning )
        {
            colour.r *= 0.1f;
            colour.g *= 0.1f;
            colour.b *= 0.1f;
        }


        float thisAlpha = 1.0f;
        if( task && task->m_state == Task::StateStarted ) thisAlpha = iv_abs( iv_sin(g_gameTime * 4.0f) );
        if( thisAlpha < 0.5f ) thisAlpha = 0.5f;

        colour.a *= thisAlpha;

        glColor4ubv( colour.GetData() );

        Vector2 iconCentre( iconX, iconY );        

        if( i == currentTaskId )        shadowSize *= sizeChangeIfSelected;
        else                            shadowSize /= sizeChangeIfSelected;

        glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowSize/2.4f + shadowOffset, iconCentre.y - shadowSize/2.4f + shadowOffset );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowSize/2.4f + shadowOffset, iconCentre.y - shadowSize/2.4f + shadowOffset );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowSize/2.4f + shadowOffset, iconCentre.y + shadowSize/2.4f + shadowOffset );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowSize/2.4f + shadowOffset, iconCentre.y + shadowSize/2.4f + shadowOffset );
        glEnd();

        if( i == currentTaskId )        shadowSize /= sizeChangeIfSelected;
        else                            shadowSize *= sizeChangeIfSelected;

        iconX += iconSize;
        iconX += s_iconGap;
    }

    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );

    iconX = s_iconX;
    

    //
    // Render Generic Crate icon

    //iconX = s_iconX;

    //for( int i = 0; i < numTasks; ++i )
    //{
    //    if( i != specialTaskId ) 
    //    {
    //        glEnable        ( GL_TEXTURE_2D );
    //        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/icon_crate.bmp" ) );
    //        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    //        glDepthMask     ( false );
    //        glColor4f       ( 1.0f, 1.0f, 1.0f, 0.1f );           

    //        Vector2 iconCentre( iconX, iconY );        
    //        float crateSize = iconSize * 1.0f;

    //        if( i == currentTaskId )        crateSize *= sizeChangeIfSelected;
    //        else                            crateSize /= sizeChangeIfSelected;

    //        Vector3 offset( crateSize/2, 0, crateSize/2 );
    //        offset.RotateAroundY( g_gameTimer.GetAccurateTime() * -0.3f );

    //        Task *task = g_app->m_location->GetMyTeam()->m_taskManager->m_tasks[i];

    //        if( task && task->m_state == Task::StateStarted )
    //        {
    //            glBegin( GL_QUADS );
    //                glTexCoord2i( 0, 1 );           glVertex2dv( (iconCentre + offset).GetData() );     offset.RotateAroundY(0.5f*M_PI);
    //                glTexCoord2i( 1, 1 );           glVertex2dv( (iconCentre + offset).GetData() );     offset.RotateAroundY(0.5f*M_PI);
    //                glTexCoord2i( 1, 0 );           glVertex2dv( (iconCentre + offset).GetData() );     offset.RotateAroundY(0.5f*M_PI);
    //                glTexCoord2i( 0, 0 );           glVertex2dv( (iconCentre + offset).GetData() );
    //            glEnd();
    //        }

    //        if( i == currentTaskId )        crateSize /= sizeChangeIfSelected;
    //        else                            crateSize *= sizeChangeIfSelected;
    //    }

    //    iconX += iconSize;
    //    iconX += s_iconGap;
    //}    

    iconX = s_iconX;
    glDisable       ( GL_TEXTURE_2D );


    //
    // Render our running tasks
        
    for( int i = 0; i < numTasks; ++i )    
    {                
        Task *task = NULL;
		char bmpFilename[256];
		strcpy( bmpFilename, "icons/icon_notask.bmp" );

		if( i != specialTaskId ) 
		{
			task = g_app->m_location->GetMyTeam()->m_taskManager->m_tasks[i];
			if( task )
			{
				sprintf( bmpFilename, "icons/icon_%s.bmp", Task::GetTaskName(task->m_type) );
			}
		}
		else
		{
			switch( specialTaskType )
			{
				case Entity::TypeDarwinian:
					sprintf( bmpFilename, "icons/icon_darwinian.bmp" );
					break;

				case Entity::TypeOfficer:
					sprintf( bmpFilename, "icons/icon_officer.bmp" );
					break;
			}
		}

        unsigned int texId = g_app->m_resource->GetTexture( bmpFilename );

        if( i == currentTaskId )        iconSize *= sizeChangeIfSelected;
        else                            iconSize /= sizeChangeIfSelected;

        if( i == currentTaskId )        compassSize *= sizeChangeIfSelected;
        else                            compassSize /= sizeChangeIfSelected;

        //
        // Create clickable zone over the task

		if( task )
		{
			char captionId[256];
			sprintf( captionId, "newcontrols_select_%s", Task::GetTaskName(task->m_type) );
			if( task->m_state == Task::StateStarted ) sprintf( captionId, "newcontrols_place_%s", Task::GetTaskName(task->m_type) );

			ScreenZone *zone = new ScreenZone( "SelectTask", captionId, 
												iconX - iconSize/2, iconY - iconSize/2,
												iconSize, iconSize, i );
			m_newScreenZones.PutData( zone );
			zone->m_scrollZone = 2;

            task->m_screenX = iconX;
            task->m_screenY = iconY;
            task->m_screenH = iconSize;
		}

        Vector2 iconCentre( iconX, iconY );        

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, texId );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

        if( task && task->m_startTimer > 0.0f )
        {
            glColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
        }
        else if( task->m_state != Task::StateRunning )
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.6f );
        }

        else
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );        
        }
        
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y + iconSize/2 );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y + iconSize/2 );
        glEnd();

        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_TEXTURE_2D );

        if( task && 
            task->m_id == myTeam->m_taskManager->m_currentTaskId &&
            task->m_state != Task::StateRunning )     
        {
            float x = iconCentre.x - iconSize/2.0f - 5;
            float y = iconCentre.y - iconSize/2.0f - 5;
            float w = iconSize + 10;
            float h = iconSize + 10;

            RenderScreenZoneHighlight( x, y, w, h );
        }


		//
		// Render special extra stuff for each task

		if( !task )
		{
			if( specialTaskType == Entity::TypeDarwinian )
			{
				int numDarwinians = myTeam->m_numDarwiniansSelected;

				g_gameFont.SetRenderOutline(true);
				glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
				g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+19, 12, "%d", numDarwinians );

				g_gameFont.SetRenderOutline(false);
				glColor4f( 1.0f, 1.0f, 1.0f, 0.9f );
				g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+19, 12, "%d", numDarwinians );                    
			}
		}


        RenderExtraTaskInfo( task, iconCentre, iconSize, 1.0f, false, true );


        //
        // Render compass if we are an object

		if( !task )
		{
			Vector3 taskWorldPos;

			if( specialEntity ) taskWorldPos = specialEntity->m_pos;
			else if( specialTaskType == Entity::TypeDarwinian ) taskWorldPos = myTeam->m_selectedDarwinianCentre;
			
			if( taskWorldPos != g_zeroVector ) RenderCompass( iconCentre.x, iconCentre.y, 
															taskWorldPos, 
															true,
															compassSize );
		}

		if( task )
		{
			Vector3 taskWorldPos = task->GetPosition();

			if( taskWorldPos != g_zeroVector ) RenderCompass( iconCentre.x, iconCentre.y, 
															  taskWorldPos, 
															  task->m_id == g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId,
															  compassSize );


			//
			// Render delete icon

			if( m_visible )
			{
				float deleteX = iconCentre.x;
				float deleteY = iconCentre.y + iconSize * 0.6f;
				float deleteSize = iconSize * 0.4f;

				glEnable        ( GL_TEXTURE_2D );
				glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/icon_shadow.bmp" ) );
				glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
				glDepthMask     ( false );
				glColor4f       ( 0.9f, 0.9f, 0.9f, 0.0f );        

				glBegin( GL_QUADS );
					glTexCoord2i(0,0);      glVertex2f( deleteX - deleteSize/2.0f, deleteY - deleteSize/2.0f );
					glTexCoord2i(1,0);      glVertex2f( deleteX + deleteSize/2.0f, deleteY - deleteSize/2.0f );
					glTexCoord2i(1,1);      glVertex2f( deleteX + deleteSize/2.0f, deleteY + deleteSize/2.0f );
					glTexCoord2i(0,1);      glVertex2f( deleteX - deleteSize/2.0f, deleteY + deleteSize/2.0f );
				glEnd();

				glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/icon_delete.bmp" ) );
				glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
				glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	             
				glBegin( GL_QUADS );
					glTexCoord2i(0,0);      glVertex2f( deleteX - deleteSize/2.0f, deleteY - deleteSize/2.0f );
					glTexCoord2i(1,0);      glVertex2f( deleteX + deleteSize/2.0f, deleteY - deleteSize/2.0f );
					glTexCoord2i(1,1);      glVertex2f( deleteX + deleteSize/2.0f, deleteY + deleteSize/2.0f );
					glTexCoord2i(0,1);      glVertex2f( deleteX - deleteSize/2.0f, deleteY + deleteSize/2.0f );
				glEnd();

				glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				glDisable       ( GL_TEXTURE_2D );

				char captionId[256];
				sprintf( captionId, "newcontrols_delete_%s", Task::GetTaskName(task->m_type) );
	            
				ScreenZone *zone = new ScreenZone( "DeleteTask", captionId, 
													deleteX - deleteSize/2.0f, deleteY - deleteSize/2.0f,
													deleteSize, deleteSize, task->m_id );
				m_newScreenZones.PutData( zone );
			}
        }
    
        if( i == currentTaskId )        iconSize /= sizeChangeIfSelected;
        else                            iconSize *= sizeChangeIfSelected;

        if( i == currentTaskId )        compassSize /= sizeChangeIfSelected;
        else                            compassSize *= sizeChangeIfSelected;

        iconX += iconSize;
        iconX += s_iconGap;
    }
}


void TaskManagerInterfaceMultiwinia::RenderExtraTaskInfo( Task *task, Vector2 iconCentre, float iconSize, float alpha, bool targetOrders, bool taskManager )
{
    //glColor4f( 1.0f, 1.0f, 1.0f, alpha );
    //glBegin( GL_LINE_LOOP );
    //    glVertex2f( iconCentre.x - iconSize/2.0f, iconCentre.y - iconSize/2.0f );
    //    glVertex2f( iconCentre.x + iconSize/2.0f, iconCentre.y - iconSize/2.0f );
    //    glVertex2f( iconCentre.x + iconSize/2.0f, iconCentre.y + iconSize/2.0f );
    //    glVertex2f( iconCentre.x - iconSize/2.0f, iconCentre.y + iconSize/2.0f );
    //glEnd();

    glDisable( GL_CULL_FACE );

    if( task )
    {
        float fontSize = iconSize * 0.2f;

        if( task->m_startTimer > 0.0f )
        {
            g_gameFont.SetRenderShadow(true);
            glColor4f( alpha,alpha,alpha, 0.0f );
            g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+2, 12, "%2.0", task->m_startTimer );

            g_gameFont.SetRenderShadow(false);
            glColor4f( 1.0f, 1.0f, 1.0f, alpha*0.9f );
            g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+2, 12, "%2.0f", task->m_startTimer );    
        }
        else if( task->m_type == GlobalResearch::TypeEngineer )
        {
            Engineer *engineer = (Engineer *) g_app->m_location->GetEntitySafe( task->m_objId, Entity::TypeEngineer );
            if( engineer )
            {
                char *state = engineer->GetCurrentAction();
                int numSpirits = engineer->GetNumSpirits();

                g_gameFont.SetRenderOutline(true);
                glColor4f( alpha,alpha,alpha, 0.0f );
                g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y + 25, 7, LANGUAGEPHRASE(state) );
                g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+2, 12, "%d", numSpirits );

                g_gameFont.SetRenderOutline(false);
                glColor4f( 1.0f, 1.0f, 1.0f, alpha*0.9f );
                g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y + 25, 7, LANGUAGEPHRASE(state) );
                g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+2, 12, "%d", numSpirits );                    
            }
        }
        else if( task->m_type == GlobalResearch::TypeHarvester )
        {
            Harvester *harvester = (Harvester *) g_app->m_location->GetEntitySafe( task->m_objId, Entity::TypeHarvester );
            if( harvester )
            {
                int numSpirits = harvester->GetNumSpirits();

                g_gameFont.SetRenderOutline(true);
                glColor4f( alpha,alpha,alpha, 0.0f );
                g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+19, 12, "%d", numSpirits );

                g_gameFont.SetRenderOutline(false);
                glColor4f( 1.0f, 1.0f, 1.0f, alpha*0.9f );
                g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+19, 12, "%d", numSpirits );                    
            }
        }
        else if( task->m_type == GlobalResearch::TypeSquad )
        {
            InsertionSquad *squad = (InsertionSquad *) g_app->m_location->GetUnit( task->m_objId );
            if( squad )
            {
                int numSquaddies = squad->NumAliveEntities();

                glColor4f( alpha,alpha,alpha, 0.0f );
                g_gameFont.SetRenderOutline(true);
                g_gameFont.DrawText2D( iconCentre.x+11, iconCentre.y+5, 10, "x%d", numSquaddies );
                if( task->m_lifeTimer > 0.0f )
                {
                    g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+(iconSize *0.4), 12, "%1.0f", task->m_lifeTimer );

                    if( task->m_lifeTimer <= 10 )
                    {
                        glColor4f( 1.0f, 0.0f, 0.0f, alpha*0.9f );
                    }
                    else
                    {
                        glColor4f( 1.0f, 1.0f, 1.0f, alpha*0.9f );
                    }
                }
                else
                {
                    glColor4f( 1.0f, 1.0f, 1.0f, alpha*0.9f );
                }
                g_gameFont.SetRenderOutline(false);
                g_gameFont.DrawText2D( iconCentre.x+11, iconCentre.y+5, 10, "x%d", numSquaddies );
                if( task->m_lifeTimer > 0.0f )
                {
                    g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+(iconSize *0.4), 12, "%1.0f", task->m_lifeTimer );
                }
            }
        }
        else if( task->m_type == GlobalResearch::TypeBooster ||
            task->m_type == GlobalResearch::TypeHotFeet ||
            task->m_type == GlobalResearch::TypeSubversion ||
            task->m_type == GlobalResearch::TypeRage ||
            (task->m_type == GlobalResearch::TypeTank && g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeTankBattle ))
        {
            if( task->m_state == Task::StateRunning )
            {
                if( task->m_lifeTimer > 0.0f )
                {
                    g_gameFont.SetRenderOutline(true);
                    glColor4f( alpha,alpha,alpha, 0.0f );
                    g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+(iconSize *0.4), 12, "%1.0f", task->m_lifeTimer );

                    g_gameFont.SetRenderOutline(false);
                    if( task->m_lifeTimer <= 10 )
                    {
                        glColor4f( 1.0f, 0.0f, 0.0f, alpha*0.9f );
                    }
                    else
                    {
                        glColor4f( 1.0f, 1.0f, 1.0f, alpha*0.9f );
                    }
                    g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+(iconSize *0.4), 12, "%1.0f", task->m_lifeTimer );
                }
            }
        }
        else if( task->m_type == GlobalResearch::TypeArmour )
        {
            Armour *armour = (Armour *) g_app->m_location->GetEntitySafe( task->m_objId, Entity::TypeArmour );
            if( armour )
            {
                if( !targetOrders )
                {
                    int numDarwinians = armour->GetNumPassengers();

                    g_gameFont.SetRenderOutline(true);
                    glColor4f( alpha,alpha,alpha, 0.0f );
                    g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+iconSize*0.33f, fontSize, "%d", numDarwinians );

                    g_gameFont.SetRenderOutline(false);
                    glColor4f( 1.0f, 1.0f, 1.0f, alpha*0.9f );
                    g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+iconSize*0.33f, fontSize, "%d", numDarwinians );  
                }

                bool draw = false;
                int currentState = armour->m_state;
                if( targetOrders ) currentState = armour->m_conversionOrder;

               
                if( currentState == Armour::StateLoading )
                {
                    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/arrows_load.bmp" ) );
                    draw = true;
                }
                else if( currentState == Armour::StateUnloading )
                {
                    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/arrows_unload.bmp" ) );
                    draw = true;
                }
                
                if( draw )
                {
                    glEnable        ( GL_TEXTURE_2D );
                    glDepthMask     ( false );
                    
                    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
                    glColor4f       ( alpha,alpha,alpha, 0.0f );

                    glBegin( GL_QUADS );
                    glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - iconSize/2.2f, iconCentre.y - iconSize/2.2f );
                    glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + iconSize/2.2f, iconCentre.y - iconSize/2.2f );
                    glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + iconSize/2.2f, iconCentre.y + iconSize/2.2f );
                    glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - iconSize/2.2f, iconCentre.y + iconSize/2.2f );
                    glEnd();

                    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
                    glColor4f       ( 1.0f, 1.0f, 1.0f, alpha*0.3f );        

                    glBegin( GL_QUADS );
                    glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - iconSize/2.2f, iconCentre.y - iconSize/2.2f );
                    glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + iconSize/2.2f, iconCentre.y - iconSize/2.2f );
                    glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + iconSize/2.2f, iconCentre.y + iconSize/2.2f );
                    glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - iconSize/2.2f, iconCentre.y + iconSize/2.2f );
                    glEnd();

                    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                    glDisable       ( GL_TEXTURE_2D );
                }

                //
                // Render a moving icon if we are moving

                if( !targetOrders && taskManager )
                {
                    float distToWaypoint = ( armour->m_pos - armour->m_wayPoint ).MagSquared();
                    bool moving = ( distToWaypoint > 30.0f * 30.0f );

                    if( moving )
                    {
                        glEnable        ( GL_TEXTURE_2D );
                        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/arrows_moving.bmp" ) );
                        glDepthMask     ( false );
                        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
                        glColor4f       ( alpha,alpha,alpha, 0.0f );

                        glBegin( GL_QUADS );
                        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - iconSize/2.2f, iconCentre.y - iconSize/2.2f );
                        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + iconSize/2.2f, iconCentre.y - iconSize/2.2f );
                        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + iconSize/2.2f, iconCentre.y + iconSize/2.2f );
                        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - iconSize/2.2f, iconCentre.y + iconSize/2.2f );
                        glEnd();

                        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
                        glColor4f       ( 1.0f, 1.0f, 1.0f, alpha*0.3f );        

                        glBegin( GL_QUADS );
                        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - iconSize/2.2f, iconCentre.y - iconSize/2.2f );
                        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + iconSize/2.2f, iconCentre.y - iconSize/2.2f );
                        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + iconSize/2.2f, iconCentre.y + iconSize/2.2f );
                        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - iconSize/2.2f, iconCentre.y + iconSize/2.2f );
                        glEnd();

                        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                        glDisable       ( GL_TEXTURE_2D );
                    }
                }
            }
        }
    }
}


void TaskManagerInterfaceMultiwinia::RenderCompass( float _screenX, float _screenY, Vector3 const &_worldPos, bool _selected, float _size )
{
    g_app->m_renderer->SetupMatricesFor3D();  
    float screenH = g_app->m_renderer->ScreenH();
    float screenW = g_app->m_renderer->ScreenW();

    //
    // Project worldPos into screen co-ordinates
    // Is it on screen or not?

    float screenX, screenY;
    g_app->m_camera->Get2DScreenPos( _worldPos, &screenX, &screenY );
    screenY = screenH - screenY;

	Vector3 toCam = g_app->m_camera->GetPos() - _worldPos;
	float angle = toCam * g_app->m_camera->GetFront();
	Vector3 rotationVector = toCam ^ g_app->m_camera->GetFront();

    Vector2 compassVector;

    if( angle <= 0.0f &&
        screenX >= 0 && screenX < screenW &&
        screenY >= 0 && screenY < screenH )
    {
        // _pos is onscreen
        screenX *= ( m_screenW / screenW );
        screenY *= ( m_screenH / screenH );

        compassVector.Set( screenX - _screenX, screenY - _screenY );
        compassVector.Normalise();       
    }
    else
    {
        // _pos is offscreen
        Vector3 rayStart, rayDir;
        g_app->m_camera->GetClickRay( _screenX * screenW / m_screenW,   
                                      _screenY * screenH / m_screenH,
                                      &rayStart, &rayDir );
        Vector3 camPos = rayStart + rayDir * 1000;
        Vector3 camToTarget = ( _worldPos - camPos ).SetLength( 100 );

        float posX, posY;
        g_app->m_camera->Get2DScreenPos( camPos + camToTarget, &posX, &posY );
        posY = screenH - posY;
        posX *= ( m_screenW / screenW );
        posY *= ( m_screenH / screenH );
        
        compassVector.Set( posX - _screenX, posY - _screenY );
        compassVector.Normalise();
    }


    //
    // Render the compass

    Vector2 screenPos( _screenX - 0.5f, _screenY - 0.5f );    
    Vector2 compassRight = compassVector;
    float temp = compassRight.x;
    compassRight.x = compassRight.y;
    compassRight.y = temp * -1.0f;

    glEnable( GL_TEXTURE_2D );
    
    g_app->m_renderer->SetupMatricesFor2D();  
    SetupRenderMatrices();
    
    glColor4f       ( 0.0f, 0.0f, 0.0f, 0.8f );        

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTextureWithAlpha( "icons/compass_shadow.bmp" ) );

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex2dv( (screenPos - compassRight * _size + compassVector * _size).GetData() );
        glTexCoord2i(1,1);      glVertex2dv( (screenPos + compassRight * _size + compassVector * _size).GetData() );
        glTexCoord2i(1,0);      glVertex2dv( (screenPos + compassRight * _size - compassVector * _size).GetData() );
        glTexCoord2i(0,0);      glVertex2dv( (screenPos - compassRight * _size - compassVector * _size).GetData() );
    glEnd();

    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/compass.bmp", true, false ) );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    if( _selected ) glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    else            glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );          


    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex2dv( (screenPos - compassRight * _size + compassVector * _size).GetData() );
        glTexCoord2i(1,1);      glVertex2dv( (screenPos + compassRight * _size + compassVector * _size).GetData() );
        glTexCoord2i(1,0);      glVertex2dv( (screenPos + compassRight * _size - compassVector * _size).GetData() );
        glTexCoord2i(0,0);      glVertex2dv( (screenPos - compassRight * _size - compassVector * _size).GetData() );
    glEnd();

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable( GL_TEXTURE_2D );
}


bool TaskManagerInterfaceMultiwinia::ControlEvent( TMControl _type ) {
	switch ( _type ) {
		case TMTerminate: return g_inputManager->controlEvent( ControlIconsTaskManagerEndTask );
		case TMDisplay:   return g_inputManager->controlEvent( ControlIconsTaskManagerDisplay );
		default:          return false;
	}
}

bool TaskManagerInterfaceMultiwinia::AdviseCreateControlHelpBlue()
{
	return m_currentScreenZone != -1 && 
		ButtonHeld() && g_inputManager->controlEvent( ControlIconsTaskManagerDisplayPressed ) && 
		g_app->m_location->GetMyTeam() && 
		g_app->m_location->GetMyTeam()->m_taskManager->CapacityUsed() < g_app->m_location->GetMyTeam()->m_taskManager->Capacity();
}

bool TaskManagerInterfaceMultiwinia::AdviseCreateControlHelpGreen()
{
	ScreenZone *screenZone = NULL;

	return 
		m_currentScreenZone != -1 && 
		g_app->m_location->GetMyTeam() &&
		g_app->m_location->GetMyTeam()->m_taskManager->CapacityUsed() < g_app->m_location->GetMyTeam()->m_taskManager->Capacity() &&
		(screenZone = m_screenZones[m_currentScreenZone]) && 
        strcmp( screenZone->m_name, "NewTask" ) == 0;
}

bool TaskManagerInterfaceMultiwinia::AdviseOverSelectableZone()
{
	ScreenZone *screenZone = NULL;

	return 
		m_currentScreenZone != -1 && 
		(screenZone = m_screenZones[m_currentScreenZone]) &&
		strcmp( screenZone->m_name, "NewTask" ) != 0;
}


bool TaskManagerInterfaceMultiwinia::AdviseCloseControlHelp()
{
	if (!m_visible)
		return false;

	if (m_currentScreenZone != -1 && 
		ButtonHeld() && g_inputManager->controlEvent( ControlIconsTaskManagerDisplayPressed ))
		return false;

	return true;
}
