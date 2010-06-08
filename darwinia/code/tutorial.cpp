#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/text_renderer.h"
#include "lib/input/input.h"
#include "lib/language_table.h"

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
#include "global_world.h"
#include "team.h"
#include "unit.h"
#include "entity_grid.h"

#include "worldobject/controltower.h"
#include "worldobject/officer.h"


Tutorial::Tutorial()
:   m_chapter(0),
    m_nextChapterTimer(-1.0f),
    m_repeatMessageTimer(-1.0f),
    m_repeatMessage(NULL),
    m_repeatGesture(NULL)
{
}


int  Tutorial::GetCurrentChapter()
{
    return m_chapter;
}


void Tutorial::SetChapter( int _chapter )
{
    if( _chapter == 0 )
    {
        m_chapter = 0;
    }
    else if( _chapter > 0 )
    {
        m_chapter = _chapter-1;
        m_nextChapterTimer = GetHighResTime() + 3;
    }
    else
    {
        m_chapter = -1;
    }
}


void Tutorial::RepeatMessage( char *_stringId, float _repeatPeriod, char *_gestureDemo )
{
    if( m_repeatMessage )
    {
        delete m_repeatMessage;
        m_repeatMessage = NULL;
    }

    if( m_repeatGesture )
    {
        delete m_repeatGesture;
        m_repeatGesture = NULL;
    }

    if( _stringId )
    {
        m_repeatMessage = strdup( _stringId );
        m_repeatPeriod = _repeatPeriod;
        m_repeatMessageTimer = GetHighResTime() + m_repeatPeriod;
    }

    if( _gestureDemo )
    {
        m_repeatGesture = strdup( _gestureDemo );    
    }
}


bool Tutorial::IsRunning()
{
    return( m_chapter > 0 );
}


void Tutorial::Restart()
{
    g_app->m_sepulveda->ShutUp();
    
    m_chapter = 0;
    m_nextChapterTimer = -1.0f;
    m_repeatMessageTimer = -1.0f;

    if( m_repeatMessage )
    {
        delete m_repeatMessage;
        m_repeatMessage = NULL;
    }

    if( m_repeatGesture )
    {
        delete m_repeatGesture;
        m_repeatGesture = NULL;
    }
}


void Tutorial::Advance()
{
    //
    // Don't do anything until all teams are spawned

    if( g_app->m_levelReset ) return;
    if( !g_app->m_location ) return;
    if( !g_app->m_location->m_teams ) return;
    if( g_app->m_location->m_teams[2].m_teamType == Team::TeamTypeUnused ) return;
   

    //
    // Has the user skipped the tutorial?

    if( g_inputManager->controlEvent( ControlSkipTutorial ) )
    {
        g_app->m_sepulveda->ShutUp();
        g_app->m_sepulveda->Say( "tutorial_skipped" );
        delete g_app->m_tutorial;
        g_app->m_tutorial = NULL;
        g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
        return;
    }

    // 
    // Are we ready to start the tutorial?

    if( m_chapter == 0 && g_app->m_location && 
        g_app->m_renderer->IsFadeComplete() && 
        !g_app->m_script->IsRunningScript() )
    {
        TriggerChapter( 1 );
    }
    

    //
    // Don't do anything while scripts are running

    if( g_app->m_script->IsRunningScript() )
    {
        return;
    }


    //
    // Run the damn thing

    bool amIDone = AdvanceCurrentChapter();
    
    if( amIDone )
    {
        delete g_app->m_tutorial;
        g_app->m_tutorial = NULL;
        return;
    }
}
 

void Tutorial::Render()
{
    if( !g_app->m_editing && 
        g_app->m_location && 
        !g_app->m_renderer->m_renderingPoster &&
        ( (int) g_gameTime % 2 == 0 ) &&
        m_chapter > 0 )
    {
        g_gameFont.BeginText2D();
        g_gameFont.SetRenderOutline(true);        
        glColor4f(1.0f,1.0f,1.0f,0.0f);
        g_gameFont.DrawText2DCentre( g_app->m_renderer->ScreenW()/2.0f, 15, 15, LANGUAGEPHRASE("dialog_skiptutorial") );
        g_gameFont.SetRenderOutline(false);
        glColor4f(1.0f,1.0f,1.0f,1.0f);
        g_gameFont.DrawText2DCentre( g_app->m_renderer->ScreenW()/2.0f, 15, 15, LANGUAGEPHRASE("dialog_skiptutorial") );
        g_gameFont.EndText2D();    
    }
}


void Tutorial::TriggerChapter( int _chapter )
{
    m_chapter = _chapter;
    m_nextChapterTimer = -1.0f;

    RepeatMessage(NULL,0.0f);
    
    g_app->m_sepulveda->ShutUp();
}


bool Tutorial::AdvanceCurrentChapter()
{
    //
    // If the next chapter timer has been set, move on to the next chapter
    // if the right amount of time has passed

    if( m_nextChapterTimer > 0.0f )
    {
        if( GetHighResTime() >= m_nextChapterTimer )
        {
            TriggerChapter( m_chapter+1 );
        }
        return false;
    }
    
    
    //
    // If a repeat message has been set, say it when the time comes

    if( m_repeatMessage )
    {
        if( GetHighResTime() >= m_repeatMessageTimer )
        {
            g_app->m_sepulveda->Say( m_repeatMessage );
            if( m_repeatGesture ) g_app->m_sepulveda->DemoGesture( m_repeatGesture, 2.0f );
            m_repeatMessageTimer = GetHighResTime() + m_repeatPeriod;
        }
    }

    return true;
}


