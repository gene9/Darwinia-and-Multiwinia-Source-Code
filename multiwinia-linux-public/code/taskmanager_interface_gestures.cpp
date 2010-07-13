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

#include "network/clienttoserver.h"

#include "app.h"
#include "gamecursor.h"
#include "gesture.h"
#include "global_world.h"
#include "location.h"
#include "renderer.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "taskmanager_interface_gestures.h"
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

#include "sound/soundsystem.h"

#include "worldobject/insertion_squad.h"
#include "worldobject/officer.h"
#include "worldobject/darwinian.h"
#include "worldobject/researchitem.h"
#include "worldobject/trunkport.h"
#include "worldobject/engineer.h"



// ============================================================================


TaskManagerInterfaceGestures::TaskManagerInterfaceGestures()
:   m_screenId(ScreenTaskManager),
    m_chatLogY(0.0f),
    m_screenX(0.0f),
    m_desiredScreenX(0.0f),
    m_screenW(800),
    m_screenH(600),
    m_currentScreenZone(-1),
    m_screenZoneTimer(0.0f)
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

    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 0, ControlTaskManagerSelectTask1) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 1, ControlTaskManagerSelectTask2) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 2, ControlTaskManagerSelectTask3) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 3, ControlTaskManagerSelectTask4) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 4, ControlTaskManagerSelectTask5) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 5, ControlTaskManagerSelectTask6) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 6, ControlTaskManagerSelectTask7) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 7, ControlTaskManagerSelectTask8) );
    m_keyboardShortcuts.PutData( new KeyboardShortcut("SelectTask", 8, ControlTaskManagerSelectTask9) );

}


void TaskManagerInterfaceGestures::AdvanceGestures()
{
    //
    // We won't permit new gestures to be drawn if we already have
    // an object waiting to be placed into the world
    
    if( g_inputManager->controlEvent( ControlGestureActive ) ||
        ( g_app->m_gesture->IsRecordingGesture() &&
	      !g_inputManager->controlEvent( ControlGestureActive ) ) ) // TODO: Really?
    {
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
    }


    //
    // Only permit gestures to be drawn in the central screen

    if( g_inputManager->controlEvent( ControlGestureActive ) ||
        ( g_app->m_gesture->IsRecordingGesture() &&
	      !g_inputManager->controlEvent( ControlGestureActive ) ) ) // TODO: Really?
    {
        if( m_screenId != ScreenTaskManager )
        {            
            return;
        }
    }


    //
    // Prevent gestures if we're over a screen zone

    if( m_currentScreenZone != -1 )
    {
        return;
    }
    
    
    //
    // Starting a new gesture

    if ( !g_app->m_gesture->IsRecordingGesture() &&
	     g_inputManager->controlEvent( ControlGestureActive ) )
    {
        g_app->m_gesture->BeginGesture();
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureBegin", SoundSourceBlueprint::TypeGesture );
    }


    //
    // Adding to a gesture

    if( g_app->m_gesture->IsRecordingGesture() &&
		g_inputManager->controlEvent( ControlGestureActive ) )
    {
        int mouseX = g_target->X();
        int mouseY = g_target->Y();

        g_app->m_gesture->AddSample( mouseX, mouseY );
    }


    //
    // Finishing a gesture
    
    if ( g_app->m_gesture->IsRecordingGesture() &&
	     !g_inputManager->controlEvent( ControlGestureActive ) )
    {
        g_app->m_gesture->EndGesture();
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureEnd", SoundSourceBlueprint::TypeGesture );

        if( g_app->m_gesture->GestureSuccess() )
        {
            int symbolId = g_app->m_gesture->GetSymbolID();
            int taskType = g_app->m_location->GetMyTeam()->m_taskManager->MapGestureToTask( symbolId );

            if( g_app->m_globalWorld->m_research->HasResearch( taskType ) )
            {
                g_app->m_clientToServer->RequestRunProgram( g_app->m_globalWorld->m_myTeamId, taskType );
                g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
            }
            else
            {
                SetCurrentMessage( MessageFailure, -1, 2.5f );
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
                g_app->m_sepulveda->Say( "research_notyetavailable" );
#endif
            }
        }
        else
        {
            SetCurrentMessage( MessageFailure, -1, 2.5f );
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureFail", SoundSourceBlueprint::TypeGesture );
        }
    }

}

void TaskManagerInterfaceGestures::AdvanceTerminate()
{
    if( ControlEvent( TMTerminate ) )
    {
        g_app->m_location->GetMyTeam()->m_taskManager->TerminateTask( g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId );
        g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId = -1;

//        if( m_tasks.Size() > 0 )
//        {
//            Task *task = m_tasks[0];
//            SelectTask( task->m_id );
//        }
    }
}


void TaskManagerInterfaceGestures::HideTaskManager()
{
    if( m_visible )
    {
        //
        // Put the mouse to the middle of the screen
//        float midX = g_app->m_renderer->ScreenW() / 2.0f;
//        float midY = g_app->m_renderer->ScreenH() / 2.0f;    
//        g_target->SetMousePos( midX, midY );
//        g_app->m_camera->Advance();
        
	    if( g_app->m_location->GetMyTeam()->m_taskManager->m_tasks.Size() > 0 )
	    {
		    g_app->m_location->GetMyTeam()->m_taskManager->SelectTask( g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId );
	    }
	    g_app->m_gesture->EndGesture();

        m_screenX = 0.0f;
        m_desiredScreenX = 0.0f;
        m_screenId = ScreenTaskManager;
	    SetVisible( false );
    }
}



void TaskManagerInterfaceGestures::Advance()
{
    if( m_lockTaskManager ) return;

    AdvanceScrolling();
    AdvanceTab();
    if( g_inputManager->controlEvent( ControlGesturesTaskManagerDisplay ) )
    {
        if( !m_visible )
        {
            // Alt key just pressed
            m_screenX = 0.0f;
            m_desiredScreenX = 0.0f;
            m_screenId = ScreenTaskManager;
        }

        SetVisible();
        AdvanceScreenEdges();        
        AdvanceGestures();
        
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
        g_app->m_helpSystem->PlayerDoneAction( HelpSystem::TaskBasics );
#endif
    }
    else
	{
		// Alt key just released
        HideTaskManager();
    }

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

    AdvanceTerminate();
    AdvanceScreenZones();
    AdvanceKeyboardShortcuts();
}


void TaskManagerInterfaceGestures::AdvanceScrolling()
{   
    if( m_screenX != m_desiredScreenX )
    {
        float diff = g_advanceTime * 5;
        m_screenX = m_screenX * (1.0f-diff) + (m_desiredScreenX * diff);
        if( iv_abs( m_desiredScreenX - m_screenX ) < 0.001f )
        {
            m_screenX = m_desiredScreenX;
        }
    }
}


void TaskManagerInterfaceGestures::AdvanceScreenEdges()
{
    if( g_app->m_gesture->IsRecordingGesture() ) return;

    bool scrollPermitted = iv_abs(m_desiredScreenX - m_screenX) < 0.1f;
    
    int screenBorder = 10;
    bool keyLeft        = g_inputManager->controlEvent( ControlGestureLeft );
    bool keyRight       = g_inputManager->controlEvent( ControlGestureRight );
    
    switch( m_screenId )
    {
        case ScreenTaskManager:
            if( scrollPermitted )
            {
                if( g_target->X() < screenBorder || keyLeft )
                {
                    m_screenId = ScreenResearch;
                    m_desiredScreenX = -1.0f;                    
                }
                if( g_target->X() > g_app->m_renderer->ScreenW()-screenBorder || keyRight )
                {
                    m_screenId = ScreenObjectives;
                    m_desiredScreenX = 1.0f;
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
                    g_app->m_helpSystem->PlayerDoneAction( HelpSystem::ShowObjectives );
#endif
                }
            }
            break;

        case ScreenObjectives:
            if( scrollPermitted )
            {
                if( g_target->X() < screenBorder || keyLeft )
                {
                    m_screenId = ScreenTaskManager;
                    m_desiredScreenX = 0.0f;
                }
            }
            break;

        case ScreenResearch:
            if( scrollPermitted )
            {
                if( g_target->X() > g_app->m_renderer->ScreenW()-screenBorder || keyRight )
                {
                    m_screenId = ScreenTaskManager;
                    m_desiredScreenX = 0.0f;
                }
            }
            break;
    }
}


void TaskManagerInterfaceGestures::SetupRenderMatrices ( int _screenId )
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

    switch( _screenId )
    {
        case ScreenOverlay:
            break;

        case ScreenTaskManager:
            left    += m_screenX * m_screenW;
            break;

        case ScreenObjectives:
            left    += (m_screenX-1.0f) * m_screenW;
            break;

        case ScreenResearch:
            left    += (m_screenX+1.0f) * m_screenW;
            break;
    }

	float right = left + m_screenW;
	float bottom = top + m_screenH;

    gluOrtho2D(left, right, bottom, top);
	glMatrixMode(GL_MODELVIEW);
}


void TaskManagerInterfaceGestures::ConvertMousePosition( float &_x, float &_y )
{
    float fractionX = _x / (float) g_app->m_renderer->ScreenW();
    float fractionY = _y / (float) g_app->m_renderer->ScreenH();

    _x = m_screenW * fractionX;
    _y = m_screenH * fractionY;
}


void TaskManagerInterfaceGestures::RestoreRenderMatrices()
{
    g_app->m_renderer->SetupMatricesFor2D();
}


void TaskManagerInterfaceGestures::Render()
{
    if( g_app->m_editing || !g_app->m_location ) return;
    
    START_PROFILE( "Render Taskman");
	    
    glEnable    ( GL_BLEND );
    glDisable   ( GL_CULL_FACE );

    if( g_app->m_locationId != -1 )
    {
        RenderTargetAreas();
    }
    
    g_gameFont.BeginText2D();
    SetupRenderMatrices( ScreenOverlay );

    RenderMessages();    

    if( m_visible )
    {        
        int panelHeight = 100;

        RenderTitleBar();
        SetupRenderMatrices( ScreenOverlay );
    
        //
        // Render silver bar

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        
        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_divider.bmp" ) );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );            

        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );       glVertex2i(0, panelHeight-10);
            glTexCoord2i( 100, 0 );       glVertex2i(m_screenW,panelHeight-10);
            glTexCoord2i( 100, 1 );       glVertex2i(m_screenW,panelHeight+16);
            glTexCoord2i( 0, 1 );       glVertex2i(0,panelHeight+16);
        glEnd();

        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );       glVertex2i(0, m_screenH-26);
            glTexCoord2i( 100, 0 );       glVertex2i(m_screenW,m_screenH-26);
            glTexCoord2i( 100, 1 );       glVertex2i(m_screenW,m_screenH);
            glTexCoord2i( 0, 1 );       glVertex2i(0,m_screenH);
        glEnd();
        
        glDisable( GL_TEXTURE_2D );

        RenderOverview();
        RenderTooltip();
    
        if( m_screenX > -1.0f && m_screenX < 1.0f )     RenderTaskManager();
        if( m_screenX < 0.0f )                          RenderResearch();
        if( m_screenX > 0.0f )                          RenderObjectives();
    }

    if( m_visible )
    {
        SetupRenderMatrices( ScreenOverlay );
        RenderScreenZones();
    }
    
    RestoreRenderMatrices();

    if( m_visible )
    {        
        RenderGestures();
    }

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


void TaskManagerInterfaceGestures::AdvanceScreenZones()
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
    }

    //
    // Where is the mouse right now
            
    bool found = false;

    for( int i = 0; i < m_screenZones.Size(); ++i )
    {
        ScreenZone *zone = m_screenZones[i];
        if( ScreenZoneHighlighted(zone) )
        {
            if( m_currentScreenZone != i )
            {
                m_currentScreenZone = i;
                m_screenZoneTimer = GetHighResTime();
            }
            found = true;
        }
    }


    //
    // Special case - the player is highlighting a unit/entity in the real world
    // We want to highlight the button that corrisponds to that task ID
    // (if it exists)

    bool highlightOnly = false;

    if( !m_visible )
    {
        WorldObjectId id;
        bool somethingHighlighted = g_app->m_locationInput->GetObjectUnderMouse( id, g_app->m_globalWorld->m_myTeamId );
        if( somethingHighlighted )
        {
            int taskIndex = -1;
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
                        m_currentScreenZone = i;
                        found = true;
                        highlightOnly = true;
                    }
                }        
            }
        }
    }

    if( !found ) m_currentScreenZone = -1;

    //
    // Are we highlighting a task?

    m_highlightedTaskId = -1;
    if( m_currentScreenZone != -1 )
    {
        ScreenZone *currentZone = m_screenZones[m_currentScreenZone];
        if( stricmp( currentZone->m_name, "SelectTask" ) == 0 &&
            g_app->m_location->GetMyTeam()->m_taskManager->m_tasks.ValidIndex(currentZone->m_data) )
        {
            Task *task = g_app->m_location->GetMyTeam()->m_taskManager->m_tasks[currentZone->m_data];
            m_highlightedTaskId = task->m_id;
        }
    }


    //
    // Handle mouse clickage 

    if( m_currentScreenZone != -1 && 
        g_inputManager->controlEvent( ControlActivateTMButton ) && // TODO: This
        !highlightOnly )
    {
        ScreenZone *currentZone = m_screenZones[m_currentScreenZone];
        RunScreenZone( currentZone->m_name, currentZone->m_data );
    }

}


void TaskManagerInterfaceGestures::AdvanceKeyboardShortcuts()
{
    for( int i = 0; i < m_keyboardShortcuts.Size(); ++i )
    {
        KeyboardShortcut *shortcut = m_keyboardShortcuts[i];
        if( (*shortcut)() )
        {
            RunScreenZone( shortcut->name(), shortcut->data() );
        }
    }
}


void TaskManagerInterfaceGestures::RunScreenZone( const char *_name, int _data )
{
    //
    // New task buttons 

    if( stricmp( _name, "NewTask" ) == 0 )
    {
        if( g_app->m_globalWorld->m_research->HasResearch( _data ) )
        {
            g_app->m_clientToServer->RequestRunProgram( g_app->m_globalWorld->m_myTeamId, _data );
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureBegin", SoundSourceBlueprint::TypeGesture );
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
        if( g_app->m_location->GetMyTeam()->m_taskManager->m_tasks.ValidIndex(_data) )
        {
            Task *nextTask = g_app->m_location->GetMyTeam()->m_taskManager->m_tasks[_data];
            g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId = nextTask->m_id;  
            g_app->m_location->GetMyTeam()->m_taskManager->SelectTask( g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId );    
        }
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
    // Show screen

    if( stricmp( _name, "ShowScreen" ) == 0 )
    {
        m_screenId = _data;

        switch( m_screenId )
        {
            case ScreenOverlay:         
            case ScreenTaskManager:     m_desiredScreenX = 0.0f;            break;
            case ScreenObjectives:      m_desiredScreenX = 1.0f;            break;
            case ScreenResearch:        m_desiredScreenX = -1.0f;           break;
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
    }
}


bool TaskManagerInterfaceGestures::ScreenZoneHighlighted( ScreenZone *_zone )
{
    float mouseX = g_target->X();
    float mouseY = g_target->Y();
    ConvertMousePosition( mouseX, mouseY );

    mouseX += m_screenX * m_screenW;

    return( mouseX >= _zone->m_x && 
            mouseY >= _zone->m_y &&
            mouseX <= _zone->m_x + _zone->m_w &&
            mouseY <= _zone->m_y + _zone->m_h );
}


void TaskManagerInterfaceGestures::RenderScreenZones()
{
/*
    for( int i = 0; i < m_screenZones.Size(); ++i )
    {
        ScreenZone *zone = m_screenZones[i];
        
        float hX = zone->m_x;
        float hY = zone->m_y;
        float hW = zone->m_w;
        float hH = zone->m_h;

        hX -= m_screenX * m_screenW;

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

        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        
        float overSpill = zone->m_h * 0.1f;
        float hX = zone->m_x - overSpill;
        float hY = zone->m_y - overSpill;
        float hW = zone->m_w + overSpill * 2;
        float hH = zone->m_h + overSpill * 2;

        hX -= m_screenX * m_screenW;

        glColor4f( 0.05f, 0.05f, 0.5f, 0.3f );

        glBegin( GL_QUADS );
            glVertex2f( hX, hY );
            glVertex2f( hX+hW, hY );
            glVertex2f( hX+hW, hY+hH );
            glVertex2f( hX, hY+hH );
        glEnd();

        glColor4f( 1.0f, 1.0f, 0.3f, 1.0f );
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/mouse_selection.bmp" ) );
        glEnable( GL_TEXTURE_2D );
        
        glBegin( GL_QUADS );
            glTexCoord2f(0.0f,0.0f);      glVertex2f( hX, hY );
            glTexCoord2f(0.5f,0.0f);      glVertex2f( hX + hH/2, hY );
            glTexCoord2f(0.5f,1.0f);      glVertex2f( hX + hH/2, hY + hH );
            glTexCoord2f(0.0f,1.0f);      glVertex2f( hX, hY + hH );

            glTexCoord2f(0.5f,0.0f);      glVertex2f( hX + hW - hH/2, hY );
            glTexCoord2f(1.0f,0.0f);      glVertex2f( hX + hW, hY );
            glTexCoord2f(1.0f,1.0f);      glVertex2f( hX + hW, hY + hH );
            glTexCoord2f(0.5f,1.0f);      glVertex2f( hX + hW - hH/2, hY + hH );            
        glEnd();
        
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable( GL_TEXTURE_2D );
    }
}


void TaskManagerInterfaceGestures::RenderTooltip()
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


        //
        // Render keyboard shortcut

        KeyboardShortcut *keyPress = NULL;
        for( int i = 0; i < m_keyboardShortcuts.Size(); ++i )
        {
            KeyboardShortcut *shortcut = m_keyboardShortcuts[i];
            if( stricmp( shortcut->name(), zone->m_name ) == 0 &&
                shortcut->data() == zone->m_data )
            {
                keyPress = shortcut;
                break;
            }
        }

        if( keyPress != NULL )
        {
            char caption[256];

		    sprintf( caption, " : %s", 
			    keyPress->noun().c_str() );

            UnicodeString cap = LANGUAGEPHRASE("help_keyboardshortcut") + UnicodeString(caption);
            
		    g_gameFont.DrawText2D( m_screenW - 250, m_screenH - 12, 12, cap );
		    g_gameFont.DrawText2D( m_screenW - 250, m_screenH - 12, 12, cap );
        }
    }
    
    g_gameFont.SetRenderShadow(false);

}


void TaskManagerInterfaceGestures::RenderMessages()
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
        if( !&(LANGUAGEPHRASE( currentMessageStringId )) )
        {
            m_currentMessageType = -1;
            return;
        }

		const UnicodeString message = LANGUAGEPHRASE( currentMessageStringId );

        
        //
        // Lookup task name

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
            Task::GetTaskNameTranslated( m_currentTaskType, taskName );
        }


        //
        // Build string

        wchar_t fullMessage[256];
		if( taskName.Length() > 0 )
        {
            if( m_currentMessageType == MessageResearchUpgrade )
            {
                int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( m_currentTaskType );
				swprintf( fullMessage, sizeof(fullMessage)/sizeof(wchar_t),
						  L"%ls: %ls v%d.0", message.m_unicodestring, taskName.m_unicodestring, researchLevel );
            }
            else
            {
                swprintf( fullMessage, sizeof(fullMessage)/sizeof(wchar_t),
						  L"%ls: %ls", message.m_unicodestring, taskName.m_unicodestring );
            }
        }
        else
        {
            swprintf( fullMessage, sizeof(fullMessage)/sizeof(wchar_t), L"%ls", message.m_unicodestring );
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

        g_gameFont.SetRenderOutline(true);
        glColor4f( outlineAlpha, outlineAlpha, outlineAlpha, 0.0f );
        g_gameFont.DrawText2DCentre( m_screenW/2.0f, 370.0f, size, UnicodeString(fullMessage) );
        
        g_gameFont.SetRenderOutline(false);
        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        g_gameFont.DrawText2DCentre( m_screenW/2.0f, 370.0f, size, UnicodeString(fullMessage) );
    }
}


void TaskManagerInterfaceGestures::RenderTargetAreas()
{
    Task *task = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();

    if( task && task->m_type != GlobalResearch::TypeOfficer && task->m_state == Task::StateStarted )    
    {
        LList<TaskTargetArea> *targetAreas = g_app->m_location->GetMyTeam()->m_taskManager->GetTargetArea( task->m_id );
        RGBAColour *colour = &g_app->m_location->GetMyTeam()->m_colour;
        
        for( int i = 0; i < targetAreas->Size(); ++i )
        {
            TaskTargetArea *tta = targetAreas->GetPointer(i);

            if( tta->m_stationary )
            {
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
                g_app->m_sepulveda->HighlightPosition( tta->m_centre, tta->m_radius, "TaskTargetAreas" );
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


static BitmapRGBA *s_alphaMask = NULL;
static unsigned int s_alphaMaskId = -1;
static BitmapRGBA *s_gestures=NULL;
static unsigned int s_textureId=-1;

#ifdef USE_DIRECT3D
void TaskManagerInterfaceGestures::ReleaseD3dResources()
{
	SAFE_DELETE(s_alphaMask);
	SAFE_DELETE(s_gestures);
	if(s_alphaMaskId!=-1)
	{
		glDeleteTextures(1,&s_alphaMaskId);
		s_alphaMaskId = -1;
	}
	if(s_textureId!=-1)
	{
		glDeleteTextures(1,&s_textureId);
		s_textureId = -1;
	}
}
#endif

void TaskManagerInterfaceGestures::RenderTaskManager()
{
    SetupRenderMatrices( ScreenTaskManager );
    
    //
    // Render title
    
    g_gameFont.SetRenderOutline(true);
    glColor4f( 0.8f, 0.8f, 0.8f, 0.0f );
    g_gameFont.DrawText2DCentre( m_screenW/2.0f, 30, 50, LANGUAGEPHRASE("taskmanager_taskmanager") );
    g_gameFont.SetRenderOutline(false);
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_gameFont.DrawText2DCentre( m_screenW/2.0f, 30, 50, LANGUAGEPHRASE("taskmanager_taskmanager") );

    if( !s_alphaMask )
    {
	    BinaryReader *reader = g_app->m_resource->GetBinaryReader("icons/gesture_alpha.bmp");
        s_alphaMask = new BitmapRGBA( reader, "bmp" );
        delete reader;

        for( int x = 0; x < s_alphaMask->m_width; ++x )
        {
            for( int y = 0; y < s_alphaMask->m_height; ++y )
            {
                RGBAColour col = s_alphaMask->GetPixel( x, y );
                RGBAColour newCol( 255, 255, 255, col.r );
                s_alphaMask->PutPixel( x, y, newCol );
            }
        }
    
        s_alphaMaskId = s_alphaMask->ConvertToTexture();
    }

    //
    // Render background gradient thing

    float y = 116;
    float height = m_screenH - y - 26;
    float x = m_screenW/2.0f - height/2.0f;
  
    glEnable            (GL_TEXTURE_2D );

    gglActiveTextureARB (GL_TEXTURE0_ARB);
    glBindTexture       (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_red.bmp" ) );
    glTexParameteri     (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri     (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvf           (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);        
    glTexParameteri     (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri     (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );            
    glEnable            (GL_TEXTURE_2D );

    gglActiveTextureARB (GL_TEXTURE1_ARB);
    glBindTexture       (GL_TEXTURE_2D, s_alphaMaskId );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_RGB_EXT);
    glTexEnvf           (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
    glTexParameteri     (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri     (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glEnable            (GL_TEXTURE_2D );

    glColor4f( 1.0f, 1.0f, 1.0f, 0.85f );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin( GL_QUADS );
        gglMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,1);  
        gglMultiTexCoord2fARB(GL_TEXTURE0_ARB,0,0);     
        glVertex2f( x, y );
        gglMultiTexCoord2fARB(GL_TEXTURE1_ARB,1,1);     
        gglMultiTexCoord2fARB(GL_TEXTURE0_ARB,10,0);     
        glVertex2f( x+height, y );
        gglMultiTexCoord2fARB(GL_TEXTURE1_ARB,1,0);     
        gglMultiTexCoord2fARB(GL_TEXTURE0_ARB,10,1);     
        glVertex2f( x+height, y+height );
        gglMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,0);     
        gglMultiTexCoord2fARB(GL_TEXTURE0_ARB,0,1);     
        glVertex2f( x, y+height );
    glEnd();

    gglActiveTextureARB (GL_TEXTURE1_ARB);
    glDisable		    (GL_TEXTURE_2D);

    gglActiveTextureARB (GL_TEXTURE0_ARB);
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable		    (GL_TEXTURE_2D);




    //
    // Render edge block things

    glEnable            ( GL_TEXTURE_2D );
    glBindTexture       ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_red.bmp" ) );    
    glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );            

    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex2f( 0, y );
        glTexCoord2i(5,0);      glVertex2f( x, y );
        glTexCoord2i(5,1);      glVertex2f( x, y+height );
        glTexCoord2i(0,1);      glVertex2f( 0, y+height );
    glEnd();

    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex2f( x+height, y );
        glTexCoord2i(5,0);      glVertex2f( m_screenW, y );
        glTexCoord2i(5,1);      glVertex2f( m_screenW, y+height );
        glTexCoord2i(0,1);      glVertex2f( x+height, y+height );
    glEnd();

    glDisable           ( GL_TEXTURE_2D );


    //
    // Render gesture guide

    float radius = 160;
    float screenWCentre = m_screenW/2.0f;
    float screenHCentre = m_screenH/2.0f-10;

    glEnable        ( GL_TEXTURE_2D );
    unsigned int texId = g_app->m_resource->GetTexture( "icons/gestureguide.bmp" );
    glBindTexture   ( GL_TEXTURE_2D, texId );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glColor4f       ( 1.0f, 1.0f, 1.0f, 0.7f );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );           glVertex2f( screenWCentre - radius, screenHCentre - radius );
        glTexCoord2i( 1, 1 );           glVertex2f( screenWCentre + radius, screenHCentre - radius );
        glTexCoord2i( 1, 0 );           glVertex2f( screenWCentre + radius, screenHCentre + radius );
        glTexCoord2i( 0, 0 );           glVertex2f( screenWCentre - radius, screenHCentre + radius );
    glEnd();

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_TEXTURE_2D );


    //
    // Render running tasks

    RenderRunningTasks();


    //
    // Render navigation arrows

    if( iv_abs(m_screenX) < 0.1f )
    {
        glColor4f( 0.0f, 0.0f, 0.5f, 0.7f );
        if( fmodf(g_gameTime, 2.0f) < 1.0f )
        {
            glColor4f( 0.0f, 0.0f, 0.5f, 0.4f );
        }
        glBegin( GL_TRIANGLES );
            glVertex2f( 10, 103 );
            glVertex2f( 19, 94 );
            glVertex2f( 19, 112 );

            glVertex2f( m_screenW-10, 103 );
            glVertex2f( m_screenW-19, 94 );
            glVertex2f( m_screenW-19, 112 );
        glEnd();


        float screenCentre = m_screenW/2.0f;
        float boxW = 100;
        float boxY = 95;
        float boxH = 16;

        ScreenZone *zoneResearch    = new ScreenZone( "ShowScreen", "newcontrols_showresearch",    screenCentre-boxW*1.5f, boxY, boxW, boxH, 3 );
        ScreenZone *zoneOverview    = new ScreenZone( "ShowScreen", "newcontrols_showtaskmanager", screenCentre-boxW*0.5f, boxY, boxW, boxH, 1 );
        ScreenZone *zoneObjectives  = new ScreenZone( "ShowScreen", "newcontrols_showobjectives",  screenCentre+boxW*0.5f, boxY, boxW, boxH, 2 );
        ScreenZone *zoneLeft        = new ScreenZone( "ShowScreen", "newcontrols_showresearch",    0, boxY, 30, boxH, 3 );
        ScreenZone *zoneRight       = new ScreenZone( "ShowScreen", "newcontrols_showobjectives",  m_screenW-30, boxY, 30, boxH, 2 );
        
        m_newScreenZones.PutData( zoneResearch );
        m_newScreenZones.PutData( zoneOverview );
        m_newScreenZones.PutData( zoneObjectives );
        m_newScreenZones.PutData( zoneLeft );
        m_newScreenZones.PutData( zoneRight );
    }
}


void TaskManagerInterfaceGestures::RenderTitleBar()
{
    SetupRenderMatrices( ScreenTaskManager );

    float x = -m_screenW;
    float w = m_screenW * 3;
    float panelHeight = 100;
    
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glColor4f( 0.7f, 0.7f, 0.9f, 1.0f );
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_grey.bmp" ) );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );            
    
    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex2i(x,0);
        glTexCoord2i(10,1);      glVertex2i(x+w,0);
        glTexCoord2i(10,0);      glVertex2i(x+w,panelHeight);
        glTexCoord2i(0,0);      glVertex2i(x,panelHeight);
    glEnd();

    glDisable( GL_TEXTURE_2D );
}


void TaskManagerInterfaceGestures::RenderRunningTasks()
{  
    int numSlots = g_app->m_location->GetMyTeam()->m_taskManager->Capacity();

    float iconSize = 70;
    float shadowSize = 78;
    float iconGap = 18;
    float shadowOffset = 0;
    float totalWidth = (numSlots-1) * ( iconSize + iconGap );    
    float iconX = (m_screenW/2.0f) - totalWidth/2.0f;
    float iconY = 490;


    //
    // Render shadows for available task slots

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/icon_shadow.bmp" ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glDepthMask     ( false );
    glColor4f       ( 0.7f, 0.7f, 0.7f, 0.0f );        
    
    if( m_visible ) glColor4f( 0.9f, 0.9f, 0.9f, 0.0f );
    
	glBegin( GL_QUADS );

	for( int i = 0; i < numSlots; ++i )
    {
        Vector2 iconCentre( iconX, iconY );        
        
        glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - shadowSize/2 + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
        glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + shadowSize/2 + shadowOffset, iconCentre.y - shadowSize/2 + shadowOffset );
        glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + shadowSize/2 + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
        glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - shadowSize/2 + shadowOffset, iconCentre.y + shadowSize/2 + shadowOffset );
        
        iconX += iconSize;
        iconX += iconGap;        
    }

	glEnd();

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );

    iconX = (m_screenW/2.0f) - totalWidth/2.0f;

    
    //
    // Render our running tasks
    
    int numTasks = g_app->m_location->GetMyTeam()->m_taskManager->m_tasks.Size();
    
    for( int i = 0; i < numTasks; ++i )    
    {                
        Task *task = g_app->m_location->GetMyTeam()->m_taskManager->m_tasks[i];
        char bmpFilename[256];
        sprintf( bmpFilename, "icons/icon_%s.bmp", Task::GetTaskName(task->m_type) );
        unsigned int texId = g_app->m_resource->GetTexture( bmpFilename );

        //
        // Create clickable zone over the task

        char captionId[256];
        sprintf( captionId, "newcontrols_select_%s", Task::GetTaskName(task->m_type) );
        if( task->m_state == Task::StateStarted ) sprintf( captionId, "newcontrols_place_%s", Task::GetTaskName(task->m_type) );

        ScreenZone *zone = new ScreenZone( "SelectTask", captionId, 
                                            iconX - iconSize/2, iconY - iconSize/2,
                                            iconSize, iconSize, i );
        m_newScreenZones.PutData( zone );
        
        bool invisible = ( task->m_state == Task::StateStarted &&
                           fmod( g_gameTime, 1.0 ) < 0.4 );

        if( !invisible )
        {
            glEnable        ( GL_TEXTURE_2D );
            glBindTexture   ( GL_TEXTURE_2D, texId );
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

            if( task->m_id == g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId )     
                glColor4f   ( 1.0f, 1.0f, 1.0f, 1.0f );        
            else if( m_visible )                    glColor4f   ( 1.0f, 1.0f, 1.0f, 0.5f );
            else                                    glColor4f   ( 1.0f, 1.0f, 1.0f, 0.3f );            
        
            Vector2 iconCentre( iconX, iconY );        
    
            glBegin( GL_QUADS );
                glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y - iconSize/2 );
                glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y - iconSize/2 );
                glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + iconSize/2, iconCentre.y + iconSize/2 );
                glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - iconSize/2, iconCentre.y + iconSize/2 );
            glEnd();

            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glDisable       ( GL_TEXTURE_2D );

            if( task->m_type == GlobalResearch::TypeEngineer )
            {
                Engineer *engineer = (Engineer *) g_app->m_location->GetEntitySafe( task->m_objId, Entity::TypeEngineer );
                if( engineer )
                {
                    char *state = engineer->GetCurrentAction();
                    int numSpirits = engineer->GetNumSpirits();

                    g_gameFont.SetRenderShadow(true);
                    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                    g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y + 31, 8, LANGUAGEPHRASE(state) );
                    g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+2, 13, "%d", numSpirits );
            
                    g_gameFont.SetRenderShadow(false);
                    glColor4f( 1.0f, 1.0f, 1.0f, 0.9f );
                    g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y + 31, 8, LANGUAGEPHRASE(state) );
                    g_gameFont.DrawText2DCentre( iconCentre.x+3, iconCentre.y+2, 13, "%d", numSpirits );                    
                }
            }
            else if( task->m_type == GlobalResearch::TypeSquad )
            {
                InsertionSquad *squad = (InsertionSquad *) g_app->m_location->GetUnit( task->m_objId );
                if( squad )
                {
                    int numSquaddies = squad->NumAliveEntities();

                    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                    g_gameFont.SetRenderShadow(true);
                    g_gameFont.DrawText2D( iconCentre.x+13, iconCentre.y+5, 13, "x%d", numSquaddies );

                    glColor4f( 1.0f, 1.0f, 1.0f, 0.9f );
                    g_gameFont.SetRenderShadow(false);
                    g_gameFont.DrawText2D( iconCentre.x+13, iconCentre.y+5, 13, "x%d", numSquaddies );
                }
            }


            //
            // Render weapon selection if we're a squad
            // Render mode selection if we're an officer or armour
            // Render compass if we are an object

            Vector3 taskWorldPos;
            Unit *unit = g_app->m_location->GetUnit(task->m_objId);
            Entity *entity = g_app->m_location->GetEntity(task->m_objId);
            if( unit ) taskWorldPos = unit->m_centrePos;
            if( entity ) taskWorldPos = entity->m_pos;

            if( taskWorldPos != g_zeroVector ) RenderCompass( iconCentre.x, iconCentre.y, 
                                                              taskWorldPos, 
                                                              task->m_id == g_app->m_location->GetMyTeam()->m_taskManager->m_currentTaskId );

        }
        
        iconX += iconSize;
        iconX += iconGap;
    }

}


void TaskManagerInterfaceGestures::RenderCompass( float _screenX, float _screenY, Vector3 const &_worldPos, bool _selected )
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
    float compassSize = 53.0f;

    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/compass.bmp", true, false ) );
    glEnable( GL_TEXTURE_2D );
    
    g_app->m_renderer->SetupMatricesFor2D();  
    SetupRenderMatrices( ScreenTaskManager );
    
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glColor4f       ( 0.8f, 0.8f, 0.8f, 0.0f );        

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex2dv( (screenPos - compassRight * compassSize + compassVector * compassSize).GetData() );
        glTexCoord2i(1,1);      glVertex2dv( (screenPos + compassRight * compassSize + compassVector * compassSize).GetData() );
        glTexCoord2i(1,0);      glVertex2dv( (screenPos + compassRight * compassSize - compassVector * compassSize).GetData() );
        glTexCoord2i(0,0);      glVertex2dv( (screenPos - compassRight * compassSize - compassVector * compassSize).GetData() );
    glEnd();

    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    if( _selected ) glColor4f( 1.0f, 1.0f, 0.3f, 0.8f );
    else            glColor4f( 1.0f, 1.0f, 1.0f, 0.6f );          

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex2dv( (screenPos - compassRight * compassSize + compassVector * compassSize).GetData() );
        glTexCoord2i(1,1);      glVertex2dv( (screenPos + compassRight * compassSize + compassVector * compassSize).GetData() );
        glTexCoord2i(1,0);      glVertex2dv( (screenPos + compassRight * compassSize - compassVector * compassSize).GetData() );
        glTexCoord2i(0,0);      glVertex2dv( (screenPos - compassRight * compassSize - compassVector * compassSize).GetData() );
    glEnd();

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable( GL_TEXTURE_2D );
}


void TaskManagerInterfaceGestures::RenderOverview()
{
    float boxW = 100;
    float boxX = (m_screenW/2.0f) - (boxW/2) + (m_screenX*boxW);
    float boxY = 95;
    float boxH = 16;
    float screenCentre = m_screenW/2.0f;

    g_gameFont.SetRenderShadow(true);
    glColor4ub( 255, 255, 150, 30 );
    g_gameFont.DrawText2DCentre( screenCentre - boxW, 105, 12, LANGUAGEPHRASE("taskmanager_research") );
    g_gameFont.DrawText2DCentre( screenCentre, 105, 12, LANGUAGEPHRASE("taskmanager_taskmanager") );
    g_gameFont.DrawText2DCentre( screenCentre + boxW, 105, 12, LANGUAGEPHRASE("taskmanager_objectives") );
    g_gameFont.SetRenderShadow(false);                                            

    glColor4f( 0.0f, 0.0f, 0.5f, 0.6f );
    glLineWidth(1.0f);
    glBegin( GL_LINE_LOOP );
        glVertex2f( boxX, boxY );
        glVertex2f( boxX+boxW, boxY );
        glVertex2f( boxX+boxW, boxY+boxH );
        glVertex2f( boxX, boxY+boxH );
    glEnd();
    glLineWidth(1.0f);

    glColor4f( 0.0f, 0.0f, 0.5f, 0.2f );
    glBegin( GL_QUADS );
        glVertex2f( boxX, boxY );
        glVertex2f( boxX+boxW, boxY );
        glVertex2f( boxX+boxW, boxY+boxH );
        glVertex2f( boxX, boxY+boxH );
    glEnd();    
}


void TaskManagerInterfaceGestures::RenderGestures()
{
	START_PROFILE( "Render Gestures");
    
#define BITMAPW 64
#define BITMAPH 48

    //
    // Render the gesture to a small texture

    if( !s_gestures )
    {
		START_PROFILE( "Create Bitmap");
        s_gestures = new BitmapRGBA(BITMAPW,BITMAPW);
		END_PROFILE( "Create Bitmap");
		START_PROFILE( "ConvertToTexture");
        s_textureId = s_gestures->ConvertToTexture(false);
		END_PROFILE( "ConvertToTexture");
    }

	START_PROFILE( "Clear");
    s_gestures->Clear( RGBAColour(0,0,0) );
	END_PROFILE( "Clear");

    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    float prevX, prevY;
    g_app->m_gesture->GetMouseSample( 0, &prevX, &prevY );
    prevX = ( prevX / screenW ) * BITMAPW;
    prevY = ( prevY / screenH ) * BITMAPH;

    float timeNow = GetHighResTime();

	START_PROFILE( "Draw lines");
    for( int i = 0; i < g_app->m_gesture->GetNumMouseSamples(); ++i )
    {
        float sampleX, sampleY, time;
        g_app->m_gesture->GetMouseSample( i, &sampleX, &sampleY, &time );

        sampleX = ( sampleX / screenW ) * BITMAPW;
        sampleY = ( sampleY / screenH ) * BITMAPH;
        
        float timeFactor = timeNow - time;
        if( timeFactor < 0 ) timeFactor = 0;
        int alpha = ( timeFactor >= 3.0f ? 0 : 255 * (1.0f - timeFactor/3.0f) );
        if( alpha < 0 ) alpha = 0;
        s_gestures->DrawLine( prevX, prevY, sampleX, sampleY, RGBAColour(alpha,alpha,alpha) );

        prevX = sampleX;
        prevY = sampleY;
    }
	END_PROFILE( "Draw lines");
    
    s_gestures->ApplyBlurFilter(1.4f);
    

    //
    // Put the gesture texture onto the screen
    
	START_PROFILE( "Draw texture");
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, s_textureId );
	glTexImage2D    ( GL_TEXTURE_2D, 0, 4, s_gestures->m_width, s_gestures->m_height, 0, 
                      GL_RGBA, GL_UNSIGNED_BYTE, s_gestures->m_pixels);
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );    
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glColor4f       ( 0.2f, 0.2f, 0.9f, 1.0f );

    glBegin( GL_QUADS );
        glTexCoord2f(0.0f,0.0f);         glVertex2i(0,0);
        glTexCoord2f(1.0f,0.0f);         glVertex2i(screenW,0);
        glTexCoord2f(1.0f,0.75f);        glVertex2i(screenW,screenH);
        glTexCoord2f(0.0f,0.75f);        glVertex2i(0,screenH);
    glEnd();

	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );    
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glColor4f       ( 0.7f, 0.7f, 1.0f, 1.0f );

    glBegin( GL_QUADS );
        glTexCoord2f(0.0f,0.0f);         glVertex2i(0,0);
        glTexCoord2f(1.0f,0.0f);         glVertex2i(screenW,0);
        glTexCoord2f(1.0f,0.75f);        glVertex2i(screenW,screenH);
        glTexCoord2f(0.0f,0.75f);        glVertex2i(0,screenH);
    glEnd();

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );   
    glDisable       ( GL_TEXTURE_2D );
	END_PROFILE( "Draw texture");


    //
    // Some debug text

/*
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    
    int symbolId = g_app->m_gesture->GetSymbolID();
    int taskType = MapGestureToTask( symbolId );

    g_editorFont.DrawText2D( g_app->m_renderer->ScreenW() - 300, 
                              g_app->m_renderer->ScreenH() - 100,
                              20, "Symbol %d (%s)",
                              taskType,
                              taskType != -1 ? Task::GetTaskName(taskType) : "none" );

    g_editorFont.DrawText2D( g_app->m_renderer->ScreenW() - 300, 
                              g_app->m_renderer->ScreenH() - 75,
                              20, "confidence %d%%",
                              int (g_app->m_gesture->GetConfidence() * 100) );

    double mahalanobis = g_app->m_gesture->GetMahalanobis();
    g_editorFont.DrawText2D( g_app->m_renderer->ScreenW() - 300, 
                              g_app->m_renderer->ScreenH() - 50,
                              20, "deviation %d (%s)",                                                          
                              (int) mahalanobis,
                              (g_app->m_gesture->GetMahalanobis() != 0.0f &&
                               g_app->m_gesture->GetMahalanobis() < GESTURE_MAHALANOBISTHRESHOLD ? "pass" : "fail") );
*/

	END_PROFILE( "Render Gestures");
}


void TaskManagerInterfaceGestures::RenderObjectives()
{    
    SetupRenderMatrices( ScreenObjectives );
    
    //
    // Render background fill

    glEnable            ( GL_TEXTURE_2D );
    glBindTexture       ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_red.bmp" ) );    
    glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    float y = 116;
    float height = m_screenH - y - 26;

    glColor4f( 1.0f, 1.0f, 1.0f, 0.85f );
    
    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex2f( 0, y );
        glTexCoord2i(5,0);      glVertex2f( m_screenW, y );
        glTexCoord2i(5,1);      glVertex2f( m_screenW, y+height );
        glTexCoord2i(0,1);      glVertex2f( 0, y+height );
    glEnd();

    glDisable           ( GL_TEXTURE_2D );
      


    g_gameFont.SetRenderOutline(true);
    glColor4f( 0.8f, 0.8f, 0.8f, 0.0f );
    g_gameFont.DrawText2DCentre( m_screenW/2.0f, 30, 50, LANGUAGEPHRASE("taskmanager_objectives") );
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_gameFont.SetRenderOutline(false);
    g_gameFont.DrawText2DCentre( m_screenW/2.0f, 30, 50, LANGUAGEPHRASE("taskmanager_objectives") );

    float textX = 50;
    float completeX = m_screenW - 200;
    float textY = 190;
    float textH = 25;
        
    float boxX = 30;
    float boxW = m_screenW - boxX * 2;

    //
    // Render primary objectives

    float mouseX = g_target->X();
    float mouseY = g_target->Y();
    ConvertMousePosition( mouseX, mouseY );


    for( int o = 0; o < 2; ++o )
    {
        LList<GlobalEventCondition *> *objectives = NULL;
        if( o == 0 ) objectives = &g_app->m_location->m_levelFile->m_primaryObjectives;
        else         objectives = &g_app->m_location->m_levelFile->m_secondaryObjectives;
        

        int numObjectives = objectives->Size();
        if( numObjectives == 0 ) continue;
        
        //
        // Title bar

        float boxY = textY - textH;
        float boxH = numObjectives * (textH*1.5f) + textH*0.75f;
        float titleHeight = 20.0f;

        glShadeModel( GL_SMOOTH );
        glBegin( GL_QUADS );
            glColor4ub( 199, 214, 220, 255 );
            glVertex2f( boxX, boxY-titleHeight );
            glVertex2f( boxX+boxW, boxY-titleHeight );
            glColor4ub( 112, 141, 168, 255 );
            glVertex2f( boxX+boxW, boxY);
            glVertex2f( boxX, boxY);
        glEnd();
        glShadeModel( GL_FLAT );

        g_gameFont.SetRenderShadow(true);
        glColor4ub( 255, 255, 150, 30 );
        UnicodeString theString = ( o == 0 ? 
                                    LANGUAGEPHRASE("taskmanager_primarys") : 
                                    LANGUAGEPHRASE("taskmanager_secondarys") );
        g_gameFont.DrawText2DCentre( boxX + boxW/2.0f, boxY-titleHeight/2+1, 13, theString );
        g_gameFont.DrawText2DCentre( boxX + boxW/2.0f, boxY-titleHeight/2+1, 13, theString );
        g_gameFont.SetRenderShadow(false);

        // 
        // Background box
    
        glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );
        glBegin( GL_QUADS );
            glVertex2f( boxX, boxY );
            glVertex2f( boxX+boxW, boxY );
            glVertex2f( boxX+boxW, boxY+boxH );
            glVertex2f( boxX, boxY+boxH );
        glEnd();

        glColor4ub( 199, 214, 220, 255 );
        glLineWidth(2.0f);
        glBegin( GL_LINE_LOOP );
            glVertex2f( boxX, boxY-titleHeight );
            glVertex2f( boxX+boxW, boxY-titleHeight );
            glVertex2f( boxX+boxW, boxY+boxH );
            glVertex2f( boxX, boxY+boxH );        
        glEnd();
        glLineWidth(1.0f);

    
        for( int i = 0; i < numObjectives; ++i )
        {
            float objectiveId = i + 10;
            if( o == 1 ) objectiveId *= -1;
            ScreenZone *zone = new ScreenZone( "Objective", "help_explainobjective",
                                               m_screenW+boxX, textY-textH/2, boxW, textH*1.5f, objectiveId );
            m_newScreenZones.PutData( zone );

        
            GlobalEventCondition *condition = objectives->GetData(i);       
            bool completed = condition->Evaluate();
        
            g_gameFont.SetRenderOutline(true);
            glColor4f( 0.8f, 0.8f, 0.8f, 0.0f );        
            g_gameFont.DrawText2D( textX, textY, textH*0.65f, LANGUAGEPHRASE( condition->m_stringId ) );
            g_gameFont.SetRenderOutline(false);
            if( completed ) glColor4f( 0.8f, 0.8f, 1.0f, 1.0f );
            else            glColor4f( 1.0f, 0.2f, 0.2f, 1.0f );
            g_gameFont.DrawText2D( textX, textY, textH*0.65f, LANGUAGEPHRASE( condition->m_stringId ) );

            char const *completedString = ( completed ? 
                                            "taskmanager_complete" : 
                                            "taskmanager_incomplete" );
            g_gameFont.SetRenderOutline(true);
            glColor4f( 0.8f, 0.8f, 0.8f, 0.0f );        
            g_gameFont.DrawText2D( completeX, textY, textH*0.75f, LANGUAGEPHRASE(completedString) );
            g_gameFont.SetRenderOutline(false);
            if( completed ) glColor4f( 0.8f, 0.8f, 1.0f, 1.0f );
            else            glColor4f( 1.0f, 0.2f, 0.2f, 1.0f );
            g_gameFont.DrawText2D( completeX, textY, textH*0.75f, LANGUAGEPHRASE(completedString) );
        
            if( condition->m_type == GlobalEventCondition::BuildingOnline )
            {
                Building *building = g_app->m_location->GetBuilding( condition->m_id );
                if( building )
                {
                    UnicodeString objectiveCounter;
					building->GetObjectiveCounter(objectiveCounter);
                    g_gameFont.DrawText2D( completeX, textY+textH*0.75f, textH/3, objectiveCounter );
                }
            }

            textY += textH * 1.5f;
        }

        textY += 70;
    }


    //
    // Render navigation arrows

    if( m_screenX > 0.8f )
    {
        glColor4f( 0.0f, 0.0f, 0.5f, 0.7f );
        if( fmodf(g_gameTime, 2.0f) < 1.0f )
        {
            glColor4f( 0.0f, 0.0f, 0.5f, 0.4f );
        }
        glBegin( GL_TRIANGLES );
            glVertex2f( 10, 103 );
            glVertex2f( 19, 94 );
            glVertex2f( 19, 112 );
        glEnd();

        float screenCentre = m_screenW/2.0f + m_screenW;
        float boxW = 100;
        float boxY = 95;
        float boxH = 16;

        ScreenZone *zoneResearch    = new ScreenZone( "ShowScreen", "newcontrols_showresearch",    screenCentre-boxW*1.5f, boxY, boxW, boxH, 3 );
        ScreenZone *zoneOverview    = new ScreenZone( "ShowScreen", "newcontrols_showtaskmanager", screenCentre-boxW*0.5f, boxY, boxW, boxH, 1 );
        ScreenZone *zoneObjectives  = new ScreenZone( "ShowScreen", "newcontrols_showobjectives",  screenCentre+boxW*0.5f, boxY, boxW, boxH, 2 );
        ScreenZone *zoneLeft        = new ScreenZone( "ShowScreen", "newcontrols_showtaskmanager",  m_screenW, boxY, 30, boxH, 1 );
        
        m_newScreenZones.PutData( zoneResearch );
        m_newScreenZones.PutData( zoneOverview );
        m_newScreenZones.PutData( zoneObjectives );
        m_newScreenZones.PutData( zoneLeft );
    }
}


void TaskManagerInterfaceGestures::RenderResearch()
{
    SetupRenderMatrices( ScreenResearch );

    //
    // Render background fill

    glEnable            ( GL_TEXTURE_2D );
    glBindTexture       ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_red.bmp" ) );    
    glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    float y = 116;
    float height = m_screenH - y - 26;

    glColor4f( 1.0f, 1.0f, 1.0f, 0.85f );
    
    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex2f( 0, y );
        glTexCoord2i(5,0);      glVertex2f( m_screenW, y );
        glTexCoord2i(5,1);      glVertex2f( m_screenW, y+height );
        glTexCoord2i(0,1);      glVertex2f( 0, y+height );
    glEnd();

    glDisable           ( GL_TEXTURE_2D );
      

    g_gameFont.SetRenderOutline(true);
    glColor4f( 0.8f, 0.8f, 0.8f, 0.0f );
    g_gameFont.DrawText2DCentre( m_screenW/2.0f, 30, 50, LANGUAGEPHRASE("taskmanager_research") );
    g_gameFont.SetRenderOutline(false);
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_gameFont.DrawText2DCentre( m_screenW/2.0f, 30, 50, LANGUAGEPHRASE("taskmanager_research") );
    //g_gameFont.DrawText2DCentre( m_screenW/2.0f, 80, 20, "ResearchPoints : %d", g_app->m_globalWorld->m_research->m_researchPoints );

    float mouseX = g_target->X();
    float mouseY = g_target->Y();
    ConvertMousePosition( mouseX, mouseY );

    int numItemsResearched = 0;
    for( int i = 0; i < GlobalResearch::NumResearchItems; ++i )
    {
        if( g_app->m_globalWorld->m_research->HasResearch( i ) )        
        {
            ++numItemsResearched;
        }
    }

    float iconY = 120;
    float totalSize = (m_screenH - 116 - 30) / (float) numItemsResearched;
    totalSize = min( totalSize, 50.0f );
    
    for( int i = 0; i < GlobalResearch::NumResearchItems; ++i )
    {
        if( g_app->m_globalWorld->m_research->HasResearch( i ) )        
        {
            float iconX = 50;
            float gapSize = 3;
            float iconSize = totalSize - gapSize;
            
            //
            // Render selection boxes

            if( g_app->m_globalWorld->m_research->m_currentResearch == i )
            {
                glColor4f( 0.05f, 0.05f, 0.5f, 0.4f );
                glBegin( GL_QUADS );
                    glVertex2i( 30, iconY-1 );
                    glVertex2i( m_screenW-30, iconY-1 );
                    glVertex2i( m_screenW-30, iconY + iconSize + 1 );
                    glVertex2i( 30, iconY + iconSize + 1 );
                glColor4f( 0.5f, 0.5f, 0.7f, 0.8f );
                    glVertex2i( 30, iconY-1 );
                    glVertex2i( m_screenW-30, iconY-1 );
                    glVertex2i( m_screenW-30, iconY + iconSize + 1 );
                    glVertex2i( 30, iconY + iconSize + 1 );
                glEnd();
            }
           

            char tooltipId[256];
            sprintf( tooltipId, "newcontrols_research_%s", GlobalResearch::GetTypeName(i) );
            ScreenZone *zone = new ScreenZone( "Research", tooltipId, 
                                              -m_screenW+30, iconY, m_screenW-60, iconSize, i );
            m_newScreenZones.PutData( zone );
                                               


            //
            // Render the shadow

            glEnable        ( GL_TEXTURE_2D );
            glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/icon_shadow.bmp" ) );
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
            glDepthMask     ( false );
            glColor4f       ( 0.5f, 0.5f, 0.5f, 0.0f );        

            float shadowSize = iconSize * 1.1f;

            glBegin         ( GL_QUADS );
                glTexCoord2i( 0, 1 );       glVertex2f( iconX, iconY );
                glTexCoord2i( 1, 1 );       glVertex2f( iconX+shadowSize, iconY);
                glTexCoord2i( 1, 0 );       glVertex2f( iconX+shadowSize, iconY+shadowSize );
                glTexCoord2i( 0, 0 );       glVertex2f( iconX, iconY+shadowSize );
            glEnd();

            glDisable       ( GL_TEXTURE_2D );

            
            //
            // Render the task symbol

            char iconFilename[256];
            sprintf( iconFilename, "icons/icon_%s.bmp", GlobalResearch::GetTypeName(i) );
            unsigned int texId = g_app->m_resource->GetTexture( iconFilename );
            if( texId != -1 )
            {
                glEnable        ( GL_TEXTURE_2D );
                glBindTexture   ( GL_TEXTURE_2D, texId );
    	        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
                glColor4f       ( 1.0f, 1.0f, 1.0f, 1.0f );
                glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
            
                glBegin         ( GL_QUADS );
                    glTexCoord2i( 0, 1 );       glVertex2f( iconX, iconY );
                    glTexCoord2i( 1, 1 );       glVertex2f( iconX+iconSize, iconY);
                    glTexCoord2i( 1, 0 );       glVertex2f( iconX+iconSize, iconY+iconSize );
                    glTexCoord2i( 0, 0 );       glVertex2f( iconX, iconY+iconSize );
                glEnd();
            
                glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glDisable       ( GL_TEXTURE_2D );
            }
            

            //
            // Render research progress

            int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( i );
            int researchProgress = g_app->m_globalWorld->m_research->CurrentProgress( i );
			UnicodeString name;
			GlobalResearch::GetTypeNameTranslated(i, name);
                        
            g_gameFont.SetRenderOutline(true);
            glColor4f( 0.8f, 0.8f, 0.8f, 0.0f );
            g_gameFont.DrawText2D( iconX+iconSize+10, iconY+iconSize/2, 20, name  );
            g_gameFont.SetRenderOutline(false);
            glColor4f(1.0f,1.0f,1.0f,1.0f);
            g_gameFont.DrawText2D( iconX+iconSize+10, iconY+iconSize/2, 20, name );

            float boxX = 300;
            float boxY = iconY+iconSize*0.25f;
            float boxH = iconSize * 0.4f;
            float boxScale = 0.6f;

            glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_grey.bmp" ) );
    	    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
            glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
            glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );            
            
            for( int level = 1; level < 4; ++level )
            {
                int requiredProgress = g_app->m_globalWorld->m_research->RequiredProgress(level);

                float shadowOffset = 2.0f;
                glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );
                glBegin( GL_QUADS );
                    glVertex2i( boxX, boxY );
                    glVertex2i( boxX+requiredProgress*boxScale+shadowOffset, boxY );
                    glVertex2i( boxX+requiredProgress*boxScale+shadowOffset, boxY+boxH+shadowOffset );
                    glVertex2i( boxX, boxY+boxH+shadowOffset );
                glEnd();

                if( researchLevel > level )
                {
                    glColor4f( 0.7f, 0.7f, 0.9f, 1.0f );
                    glEnable( GL_TEXTURE_2D );
                    glBegin( GL_QUADS );
                        glTexCoord2i(0,0);      glVertex2i( boxX, boxY );
                        glTexCoord2i(10,0);     glVertex2i( boxX+requiredProgress*boxScale, boxY );
                        glTexCoord2i(10,1);     glVertex2i( boxX+requiredProgress*boxScale, boxY+boxH );
                        glTexCoord2i(0,1);      glVertex2i( boxX, boxY+boxH );
                    glEnd();         
                    float alpha = 1.0f;
                    if( g_app->m_globalWorld->m_research->m_currentResearch == i )
                    {
                        alpha = iv_abs(iv_sin(g_gameTime*2));
                    }
                    glColor4f( 0.9f, 0.9f, 1.0f, alpha );
                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
                    glBegin( GL_QUADS );
                        glTexCoord2i(0,0);      glVertex2i( boxX, boxY );
                        glTexCoord2i(10,0);     glVertex2i( boxX+requiredProgress*boxScale, boxY );
                        glTexCoord2i(10,1);     glVertex2i( boxX+requiredProgress*boxScale, boxY+boxH );
                        glTexCoord2i(0,1);      glVertex2i( boxX, boxY+boxH );
                        glTexCoord2i(0,0);      glVertex2i( boxX, boxY );
                        glTexCoord2i(10,0);     glVertex2i( boxX+requiredProgress*boxScale, boxY );
                        glTexCoord2i(10,1);     glVertex2i( boxX+requiredProgress*boxScale, boxY+boxH );
                        glTexCoord2i(0,1);      glVertex2i( boxX, boxY+boxH );
                    glEnd();         
                    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                    glDisable( GL_TEXTURE_2D );                    
                }

                if( researchLevel == level && researchProgress > 0 )
                {
                    glColor4f( 0.7f, 0.7f, 1.0f, 1.0f );
                    glEnable( GL_TEXTURE_2D );
                    glBegin( GL_QUADS );
                        glTexCoord2i(0,0);      glVertex2i( boxX, boxY );
                        glTexCoord2i(10,0);      glVertex2i( boxX+researchProgress*boxScale, boxY );
                        glTexCoord2i(10,1);      glVertex2i( boxX+researchProgress*boxScale, boxY+boxH );
                        glTexCoord2i(0,1);      glVertex2i( boxX, boxY+boxH );
                    glEnd();                
                    if( g_app->m_globalWorld->m_research->m_currentResearch == i )
                    {
                        float alpha = iv_abs(iv_sin(g_gameTime*2));
                        glColor4f( 0.9f, 0.9f, 1.0f, alpha );
                        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
                        glBegin( GL_QUADS );
                            glTexCoord2i(0,0);      glVertex2i( boxX, boxY );
                            glTexCoord2i(10,0);      glVertex2i( boxX+researchProgress*boxScale, boxY );
                            glTexCoord2i(10,1);      glVertex2i( boxX+researchProgress*boxScale, boxY+boxH );
                            glTexCoord2i(0,1);      glVertex2i( boxX, boxY+boxH );
                        glEnd();
                        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                    }

                    glDisable( GL_TEXTURE_2D );
                }

                glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
                glLineWidth(1.0f);
                glBegin( GL_LINE_LOOP );
                    glVertex2i( boxX, boxY );
                    glVertex2i( boxX+requiredProgress*boxScale, boxY );
                    glVertex2i( boxX+requiredProgress*boxScale, boxY+boxH );
                    glVertex2i( boxX, boxY+boxH );
                glEnd();

                boxX += requiredProgress*boxScale;
                boxX += 3;
            }

            float texX = 530;

            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            g_gameFont.DrawText2D( texX, iconY+iconSize/2, 12, "v%d.0", researchLevel );


            //
            // Render gesture

            float gestureX = m_screenW - 50 - iconSize;

            bool gestureRequired = ( i != GlobalResearch::TypeDarwinian &&
                                     i != GlobalResearch::TypeLaser &&
                                     i != GlobalResearch::TypeTaskManager );

            if( gestureRequired )
            {
                sprintf( iconFilename, "icons/gesture_%s.bmp", GlobalResearch::GetTypeName(i) );
                texId = g_app->m_resource->GetTexture( iconFilename );
                if( texId != -1 )
                {
                    glEnable        ( GL_TEXTURE_2D );
                    glBindTexture   ( GL_TEXTURE_2D, texId );
    	            glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
                
                    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
                    glColor4f       ( 0.7f, 0.7f, 0.7f, 0.0f );

                    float shadowOffset = 1.0f;
                    glBegin         ( GL_QUADS );
                        glTexCoord2i( 0, 1 );       glVertex2f( gestureX+shadowOffset, iconY+shadowOffset );
                        glTexCoord2i( 1, 1 );       glVertex2f( gestureX+iconSize+shadowOffset, iconY+shadowOffset);
                        glTexCoord2i( 1, 0 );       glVertex2f( gestureX+iconSize+shadowOffset, iconY+iconSize+shadowOffset );
                        glTexCoord2i( 0, 0 );       glVertex2f( gestureX+shadowOffset, iconY+iconSize+shadowOffset );
                    glEnd();
                
                    glColor4f       ( 1.0f, 1.0f, 1.0f, 1.0f );
                    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
            
                    glBegin         ( GL_QUADS );
                        glTexCoord2i( 0, 1 );       glVertex2f( gestureX, iconY );
                        glTexCoord2i( 1, 1 );       glVertex2f( gestureX+iconSize, iconY);
                        glTexCoord2i( 1, 0 );       glVertex2f( gestureX+iconSize, iconY+iconSize );
                        glTexCoord2i( 0, 0 );       glVertex2f( gestureX, iconY+iconSize );
                    glEnd();
            
                    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                    glDisable       ( GL_TEXTURE_2D );
                }
            }

            iconY += totalSize;
        }
    }


    //
    // Render navigation arrows

    if( m_screenX < -0.9f )
    {
        glColor4f( 0.0f, 0.0f, 0.5f, 0.7f );
        if( fmodf(g_gameTime, 2.0f) < 1.0f )
        {
            glColor4f( 0.0f, 0.0f, 0.5f, 0.4f );
        }
        glBegin( GL_TRIANGLES );
            glVertex2f( m_screenW-10, 103 );
            glVertex2f( m_screenW-19, 94 );
            glVertex2f( m_screenW-19, 112 );
        glEnd();


        float screenCentre = m_screenW/2.0f - m_screenW;
        float boxW = 100;
        float boxY = 95;
        float boxH = 16;

        ScreenZone *zoneResearch    = new ScreenZone( "ShowScreen", "newcontrols_showresearch",    screenCentre-boxW*1.5f, boxY, boxW, boxH, 3 );
        ScreenZone *zoneOverview    = new ScreenZone( "ShowScreen", "newcontrols_showtaskmanager", screenCentre-boxW*0.5f, boxY, boxW, boxH, 1 );
        ScreenZone *zoneObjectives  = new ScreenZone( "ShowScreen", "newcontrols_showobjectives",  screenCentre+boxW*0.5f, boxY, boxW, boxH, 2 );
        ScreenZone *zoneRight       = new ScreenZone( "ShowScreen", "newcontrols_showtaskmanager",  -30, boxY, 30, boxH, 1 );
        
        m_newScreenZones.PutData( zoneResearch );
        m_newScreenZones.PutData( zoneOverview );
        m_newScreenZones.PutData( zoneObjectives );
        m_newScreenZones.PutData( zoneRight );

    }
}


bool TaskManagerInterfaceGestures::ControlEvent( TMControl _type ) {
	switch ( _type ) {
		case TMTerminate: return g_inputManager->controlEvent( ControlGesturesTaskManagerEndTask );
		case TMDisplay:   return g_inputManager->controlEvent( ControlGesturesTaskManagerDisplay );
		default:          return false;
	}
}

bool TaskManagerInterfaceGestures::AdviseCreateControlHelpBlue()
{
	return false;
}

bool TaskManagerInterfaceGestures::AdviseCreateControlHelpGreen()
{
	return false;
}

bool TaskManagerInterfaceGestures::AdviseOverSelectableZone()
{
	return true;
}

bool TaskManagerInterfaceGestures::AdviseCloseControlHelp()
{
	return m_visible;
}




