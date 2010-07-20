#include "lib/universal_include.h"
#include "lib/input/input.h"
#include "lib/input/win32_eventhandler.h"
#include "lib/profiler.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include "lib/hi_res_time.h"
#include "lib/language_table.h"

#include <eclipse.h>

#include "interface/mainmenus.h"

#include "startsequence.h"
#include "app.h"
#include "camera.h"
#include "user_input.h"
#include "renderer.h"
#include "global_world.h"

#include "sound/soundsystem.h"


StartSequence::StartSequence()
{
    m_startTime = GetHighResTime();

    float screenRatio = (float) g_app->m_renderer->ScreenH() / (float) g_app->m_renderer->ScreenW();
    int screenH = 800 * screenRatio;

    float x = 150;
    float y = screenH*4/5.0f;
    float size = 10.0f;

    RegisterCaption( LANGUAGEPHRASE( "intro_1" ),  x, y, size, 3, 15 );
    RegisterCaption( LANGUAGEPHRASE( "intro_2" ),  x, y+15, 20, 8, 15 );
	RegisterCaption( LANGUAGEPHRASE( "intro_3" ),  x, y+40, size, 30, 45 );
    RegisterCaption( LANGUAGEPHRASE( "intro_4" ),  x, y+50, size, 42, 45 );
    RegisterCaption( LANGUAGEPHRASE( "intro_5" ),  x, y, size, 54, 65 );
    RegisterCaption( LANGUAGEPHRASE( "intro_6" ),  x, y, size, 66, 74 );
    RegisterCaption( LANGUAGEPHRASE( "intro_7" ),  x, y+15, size, 72, 74 );
    RegisterCaption( LANGUAGEPHRASE( "intro_8" ),  x, y, size, 74, 90 );
    RegisterCaption( LANGUAGEPHRASE( "intro_9" ),  x, y+15, 15, 82, 90 );
    RegisterCaption( LANGUAGEPHRASE( "intro_10" ), x, y+30, 15, 86, 90 );
}


void StartSequence::RegisterCaption( char *_caption, float _x, float _y, float _size,
                                     float _startTime, float _endTime )
{
    StartSequenceCaption *caption = new StartSequenceCaption();


    caption->m_caption = strdup( _caption );
    caption->m_x = _x;
    caption->m_y = _y;
    caption->m_size = _size;
    caption->m_startTime = _startTime;
    caption->m_endTime = _endTime;

    m_captions.PutData( caption );
}


bool StartSequence::Advance()
{
    static bool started = false;
    if( GetHighResTime() > m_startTime && !started )
    {
        started = true;
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "StartSequence", SoundSourceBlueprint::TypeMusic );
	    g_app->m_camera->SetDebugMode(Camera::DebugModeAuto);
        g_app->m_camera->RequestMode(Camera::ModeSphereWorldIntro);
    }

    g_inputManager->PollForEvents();

	if( !g_eventHandler->WindowHasFocus() )
    {
		Sleep(1);
		g_app->m_userInput->Advance();
		return false;
    }

    if( g_inputManager->controlEvent( ControlSkipMessage ) ||
		g_app->m_requestQuit ||
		( GetHighResTime() - m_startTime ) > 90 )
    {
        g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Music StartSequence" );
        return true;
    }

    g_app->m_userInput->Advance();
    g_app->m_camera->Advance();
    g_app->m_soundSystem->Advance();
#ifdef PROFILER_ENABLED
	g_app->m_profiler->Advance();
#endif // PROFILER_ENABLED

    g_app->m_renderer->Render();

    return false;
}


void StartSequence::Render()
{
    float screenRatio = (float) g_app->m_renderer->ScreenH() / (float) g_app->m_renderer->ScreenW();
    int screenH = 800 * screenRatio;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 800, screenH, 0);
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    float timeNow = GetHighResTime() - m_startTime;

    if( timeNow < 3.0f )
    {
        float alpha = 1.0f-timeNow/3.0f;
        glColor4f(0,0,0,alpha);
        glBegin( GL_QUADS );
            glVertex2i(0,0);
            glVertex2i(800,0);
            glVertex2i(800,screenH);
            glVertex2i(0,screenH);
        glEnd();
    }

    if( timeNow > 87 )
    {
        float alpha = ( timeNow - 87 ) / 2.0f;
        glColor4f(1,1,1,alpha);
        glBegin( GL_QUADS );
            glVertex2i(0,0);
            glVertex2i(800,0);
            glVertex2i(800,screenH);
            glVertex2i(0,screenH);
        glEnd();
    }

    Vector2 cursorPos;
    bool cursorFlash = false;
    float cursorSize = 0.0f;

    for( int i = 0; i < m_captions.Size(); ++i )
    {
        StartSequenceCaption *caption = m_captions[i];
        if( timeNow >= caption->m_startTime &&
            timeNow <= caption->m_endTime )
        {
            char theString[256];
            sprintf( theString, caption->m_caption );
            int stringLength = strlen(theString);
            int maxTimeLength = ( timeNow - caption->m_startTime ) * 20;
            if( maxTimeLength < stringLength )
            {
                theString[ maxTimeLength ] = '\x0';
            }

            glColor4f( 1.0f, 1.0f, 1.0f, 0.8f );
            g_gameFont.DrawText2D( caption->m_x, caption->m_y, caption->m_size, theString );

            int finishedLen = strlen(theString);
            int texW = g_gameFont.GetTextWidth( finishedLen, caption->m_size );
            cursorPos.Set( caption->m_x + texW, caption->m_y - 7.25f );
            cursorFlash = maxTimeLength > stringLength;
            cursorSize = caption->m_size;
        }
    }

    if( cursorPos != Vector2(0,0) )
    {
        if( !cursorFlash || fmod( timeNow, 1 ) < 0.5f )
        {
            glBegin( GL_QUADS );
                glVertex2f( cursorPos.x, cursorPos.y );
                glVertex2f( cursorPos.x + cursorSize*0.7f, cursorPos.y );
                glVertex2f( cursorPos.x + cursorSize*0.7f, cursorPos.y + cursorSize * 0.88f );
                glVertex2f( cursorPos.x, cursorPos.y + cursorSize * 0.88f );
            glEnd();
        }
    }

    g_app->m_renderer->SetupMatricesFor3D();


    //
    // Render grid behind darwinia

    float scale = 1000.0f;

    float fog = 0.0f;
    float fogCol[] = { fog, fog, fog, fog };

    int fogVal = 5810000;

    float r = 2.0f;
    float height = -400.0f;
    float gridSize = 100.0f;

    float xStart = -4000.0f*r;
    float xEnd = 4000.0f + 4000.0f*r;
    float zStart = -4000.0f*r;
    float zEnd = 4000.0f + 4000.0f*r;

    float fogColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    if( timeNow > 50.0f )
    {
        glPushMatrix();
        glScalef    ( scale, scale, scale );

        glFogf      ( GL_FOG_DENSITY, 1.0f );
        glFogf      ( GL_FOG_START, 0.0f );
        glFogf      ( GL_FOG_END, (float) fogVal );
        glFogfv     ( GL_FOG_COLOR, fogCol );
        glFogi      ( GL_FOG_MODE, GL_LINEAR );
        glEnable    ( GL_FOG );

        glEnable		(GL_LINE_SMOOTH);
        glEnable		(GL_BLEND);
        glBlendFunc		(GL_SRC_ALPHA, GL_ONE);
        glEnable        (GL_DEPTH_TEST);
        glLineWidth		(1.0f);
        glColor4f		(0.5, 0.5, 1.0, 0.5);

        float percentDrawn = 1.0f - (timeNow - 50.0f) / 10.0f;
        percentDrawn = max( percentDrawn, 0.0f );
        xEnd -= ( 8000 + 4000 * r * percentDrawn );
        zEnd -= ( 8000 + 4000 * r * percentDrawn );

        for( int x = xStart; x < xEnd; x += gridSize )
        {
            glBegin( GL_LINES );
                glVertex3f( x, height, zStart );
                glVertex3f( x, height, zEnd );
                glVertex3f( xStart, height, x );
                glVertex3f( xEnd, height, x );
            glEnd();
        }

        glDisable		(GL_LINE_SMOOTH);
	    glDisable		(GL_BLEND);
	    glBlendFunc		(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask		(true);

        g_app->m_globalWorld->SetupFog();
        glDisable( GL_FOG );

        glPopMatrix ();
    }
}


