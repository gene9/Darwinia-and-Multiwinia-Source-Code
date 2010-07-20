#include "lib/universal_include.h"
#include "lib/text_renderer.h"
#include "lib/math_utils.h"
#include "lib/vector2.h"
#include "lib/debug_utils.h"
#include "lib/resource.h"
#include "lib/bitmap.h"
#include "lib/profiler.h"
#include "lib/hi_res_time.h"
#include "lib/language_table.h"
#include "lib/debug_render.h"
#include "lib/ogl_extensions.h"
#include "lib/binary_stream_readers.h"
#include "lib/preferences.h"
#include "lib/targetcursor.h"

#include "network/clienttoserver.h"

#include "app.h"
#include "gamecursor.h"
#include "gesture.h"
#include "global_world.h"
#include "location.h"
#include "renderer.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "team.h"
#include "unit.h"
#include "sepulveda.h"
#include "level_file.h"
#include "main.h"
#include "routing_system.h"
#include "entity_grid.h"
#include "particle_system.h"
#include "helpsystem.h"
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



ScreenZone::ScreenZone( char *_name, char *_tooltip,
                        float _x, float _y, float _w, float _h,
                        int _data )
:   m_x(_x),
    m_y(_y),
    m_w(_w),
    m_h(_h),
    m_data(_data),
    m_scrollZone(-1),
	m_subZones(false)
{
    DarwiniaReleaseAssert( strlen(_name) < sizeof(m_name), "Button name too long : %s", _name );
    DarwiniaReleaseAssert( strlen(_tooltip) < sizeof(m_toolTip), "Tooltip too long : %s", _tooltip );
    strcpy( m_name, _name );
    strcpy( m_toolTip, _tooltip );
}


// ============================================================================

KeyboardShortcut::KeyboardShortcut( std::string _name, int _data, ControlType _controltype )
: ControlEventFunctor( _controltype ), m_name( _name ), m_data( _data ) {}


const char * KeyboardShortcut::name() { return m_name.c_str(); }


int KeyboardShortcut::data() { return m_data; }


// ============================================================================


TaskManagerInterface::TaskManagerInterface()
:   m_visible(false),
    m_highlightedTaskId(-1),
    m_lockTaskManager(false),
    m_quickUnitVisible(false)
{
}


void TaskManagerInterface::SetCurrentMessage( int _messageType, int _taskType, float _timer )
{
    m_currentMessageType = _messageType;
    m_currentTaskType = _taskType;
    m_messageTimer = GetHighResTime() + _timer;
}


void TaskManagerInterface::RunDefaultObjective ( GlobalEventCondition *_cond )
{
    switch( _cond->m_type )
    {
        case GlobalEventCondition::BuildingOnline:
        case GlobalEventCondition::BuildingOffline:
        {
            Building *building = g_app->m_location->GetBuilding( _cond->m_id );
            if( building )
            {
                g_app->m_sepulveda->HighlightBuilding( building->m_id.GetUniqueId(), "RunDefaultObjective" );
                g_app->m_camera->RequestBuildingFocusMode( building, 250.0f, 75.0f );
                g_app->m_sepulveda->Say( "objective_capturetrunk_1" );
                g_app->m_sepulveda->Say( "objective_capturetrunk_2" );
                g_app->m_sepulveda->Say( "objective_capturetrunk_3" );
                m_viewingDefaultObjective = true;
            }
            break;
        }

        case GlobalEventCondition::ResearchOwned:
        {
            Building *building = NULL;
            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *thisBuilding = g_app->m_location->m_buildings[i];
                    if( thisBuilding->m_type == Building::TypeResearchItem &&
                        ((ResearchItem *) thisBuilding)->m_researchType == _cond->m_id )
                    {
                        building = thisBuilding;
                        break;
                    }
                }
            }

            if( building )
            {
                g_app->m_sepulveda->HighlightBuilding( building->m_id.GetUniqueId(), "RunDefaultObjective" );
                g_app->m_camera->RequestBuildingFocusMode( building, 100.0f, 75.0f );
                g_app->m_sepulveda->Say( "objective_research_1" );
                g_app->m_sepulveda->Say( "objective_research_2" );
                m_viewingDefaultObjective = true;
            }
            break;
        }
    }
}

void TaskManagerInterface::SetVisible( bool _visible )
{
	m_visible = _visible;
}

void TaskManagerInterface::AdvanceTab()
{
    if( !m_visible ||
        g_inputManager->getInputMode() == INPUT_MODE_KEYBOARD )
    {
        int taskId = -1;
        int index = -1;
        bool changeTask = false;
        bool gesturesCycle = false;
        if( g_inputManager->controlEvent( ControlGesturesSwitchUnit ) &&
            g_prefsManager->GetInt( "ControlMethod", 0 ) == 0 )
        {
            gesturesCycle = true;
        }
        if(g_inputManager->controlEvent( ControlUnitCycleRight ) ||
            gesturesCycle )
        {
            changeTask = true;
            for( int i = 0; i < g_app->m_taskManager->m_tasks.Size(); ++i )
            {
                if( g_app->m_taskManager->m_tasks.ValidIndex(i) )
                {
                    if( g_app->m_taskManager->m_tasks[i]->m_id == g_app->m_taskManager->m_currentTaskId )
                    {
                        if( g_app->m_taskManager->m_tasks.ValidIndex(i+1) )
                        {
                            index = i+1;
                        }
                        else if( g_app->m_taskManager->m_tasks.ValidIndex(0) )
                        {
                            index = 0;
                        }
                        break;
                    }
                }
            }
        }

        if( g_inputManager->controlEvent( ControlUnitCycleLeft ) )
        {
            changeTask = true;
            for( int i = 0; i < g_app->m_taskManager->m_tasks.Size(); ++i )
            {
                if( g_app->m_taskManager->m_tasks.ValidIndex(i) )
                {
                    if( g_app->m_taskManager->m_tasks[i]->m_id == g_app->m_taskManager->m_currentTaskId )
                    {
                        if( g_app->m_taskManager->m_tasks.ValidIndex(i-1) )
                        {
                            index = i-1;
                        }
                        else if( g_app->m_taskManager->m_tasks.ValidIndex(g_app->m_taskManager->m_tasks.Size() - 1) )
                        {
                            index = g_app->m_taskManager->m_tasks.Size() - 1;
                        }
                        break;
                    }
                }
            }
        }

        if( changeTask )
        {
            if( index == -1 && g_app->m_taskManager->m_tasks.ValidIndex(0) )
            {
                index = 0;
            }

            if( g_app->m_taskManager->m_tasks.ValidIndex(index) )
            {
                if( g_app->m_taskManager->m_tasks[index]->m_type == GlobalResearch::TypeSquad )
                {
                    g_app->m_camera->RequestEntityTrackMode( g_app->m_taskManager->m_tasks[index]->m_objId );
                }
                else
                {
                    g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                }
                taskId = g_app->m_taskManager->m_tasks[index]->m_id;
                g_app->m_taskManager->SelectTask(taskId);
            }
        }
    }
}