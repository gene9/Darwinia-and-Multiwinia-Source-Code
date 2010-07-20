#include "lib/universal_include.h"
#include "lib/vector3.h"
#include "lib/debug_render.h"
#include "lib/window_manager.h"
#include "lib/input/input.h"
#include "lib/text_renderer.h"
#include "lib/math_utils.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/language_table.h"

#include "worldobject/darwinian.h"

#include "loaders/soul_loader.h"

#include "app.h"
#include "renderer.h"
#include "global_world.h"

#include "sound/soundsystem.h"


SoulLoader::SoulLoader()
:   Loader()
{
    float fov = 20.0f;
    float nearPlane = 1.0f;
    float farPlane = 10000.0f;
    float screenW = g_app->m_renderer->ScreenW();
    float screenH = g_app->m_renderer->ScreenH();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, screenW / screenH, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Vector3 pos(0,0,0);
	Vector3 front(0,0,-1);
	Vector3 up(0,1,0);
    Vector3 forwards = pos + front;
	gluLookAt(pos.x, pos.y, pos.z,
              forwards.x, forwards.y, forwards.z,
              up.x, up.y, up.z);

    glDisable( GL_CULL_FACE );
    glDisable( GL_TEXTURE_2D );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );

    float fogCol[] = {  0, 0, 0, 0 };

    glFogf      ( GL_FOG_DENSITY, 1.0f );
    glFogf      ( GL_FOG_START, 50.0f );
    glFogf      ( GL_FOG_END, 1500.0f );
    glFogfv     ( GL_FOG_COLOR, fogCol );
    glFogi      ( GL_FOG_MODE, GL_LINEAR );
    glEnable    ( GL_FOG );

    m_spawnTimer = 0.0f;
    m_time = GetHighResTime();
    m_startTime = m_time;
}


void SoulLoader::AdvanceAllSpirits( float _time )
{
    //
    // Spawn more spirits

    m_spawnTimer -= _time;
    if( m_spawnTimer <= 0.0f )
    {
        SoulLoaderSpirit *spirit = new SoulLoaderSpirit();
        spirit->m_pos = Vector3( sfrand(200.0f), -170.0f, -frand(1000.0f) );

        m_spirits.PutData( spirit );

        m_spawnTimer = 0.6f;
    }


    //
    // Move all spirits

    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        SoulLoaderSpirit *spirit = m_spirits[i];
        spirit->Advance( _time );

        if( spirit->m_pos.y > 200.0f )
        {
            delete spirit;
            m_spirits.RemoveData(i);
            --i;
        }
    }
}


void SoulLoader::RenderAllSpirits()
{
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    glDepthMask( false );

    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        SoulLoaderSpirit *spirit = m_spirits[i];
        spirit->Render();
    }
}


void SoulLoader::RenderMessage( float _time )
{
    float timePassed = m_time - m_startTime;

    #define NUMMESSAGES 10
    static bool    setup       = false;
    static float   startTime   [NUMMESSAGES];
    static float   duration    [NUMMESSAGES];
    static float   size        [NUMMESSAGES];
    static float   sizeScale   [NUMMESSAGES];
    static Vector3 position    [NUMMESSAGES];

    static SoulLoaderSpirit *spirits[NUMMESSAGES];

    if( !setup )
    {
        setup = true;
        float time = 10.0f;
        float gap = 5.0f;
        float fixedDuration = 30.0f;
        for( int i = 0; i < 8; ++i )
        {
            time += gap * ( 1.0f + sfrand(0.4f) );
            startTime[i] = time;
            duration[i] = fixedDuration;
            size[i] = 3.0f;
            sizeScale[i] = 0.7f;
            position[i].Set( sfrand(80.0f), sfrand(60.0f), -700.0f );
            spirits[i] = new SoulLoaderSpirit();
            spirits[i]->m_pos.Set( sfrand(150.0f), -170.0f, -700.0f );
        }

        time += 30.0f;
        startTime[8] = time;
        duration[8] = 30;
        size[8] = 5;
        sizeScale[8] = 0.15f;
        position[8].Set( 0, 0, -300.0f );

        startTime[9] = time + 10;
        duration[9] = 20;
        size[9] = 2;
        sizeScale[9] = 0.3f;
        position[9].Set( 0, -5, -300.0f );
    }

    for( int i = 0; i < NUMMESSAGES; ++i )
    {
        float fadeDuration = duration[i] * 0.2f;
        float fadeUp = startTime[i] + fadeDuration * 1.0f;
        float fadeOut = startTime[i] + fadeDuration * 4.0f;

        float endTime = startTime[i] + duration[i];
        if( timePassed >= startTime[i] && timePassed <= endTime )
        {
            float thisSize = size[i];
            float thisSizeScale = 1.0f + sizeScale[i] * ( timePassed - startTime[i] ) / duration[i];;
            thisSize *= thisSizeScale;
            float alpha = 1.0f;
            if( timePassed < fadeUp ) alpha = 1.0f - ( fadeUp - timePassed ) / fadeDuration;
            if( timePassed > fadeOut ) alpha = 1.0f - ( timePassed - fadeOut ) / fadeDuration;

            if( i < 8 )
            {
                spirits[i]->Advance(_time);
                position[i] = spirits[i]->m_pos;
            }

            for( int j = 0; j < 4; ++j )
            {
                if ( j == 0 )   glColor4f( 1.0f, 1.0f, 1.0f, alpha * 0.75f );
                else            glColor4f( 1.0f, 1.0f, 1.0f, alpha * 0.3f );
                Vector3 pos = position[i];
                pos += Vector3( sinf(m_time+j), cosf(m_time/2+j), sinf(m_time/3+j*2) ) * thisSize * 0.06f;

                char msgId[256];
                sprintf( msgId, "bootloader_soul_%d", i );
                char *msg = LANGUAGEPHRASE(msgId);
                g_gameFont.DrawText3DCentre( pos, thisSize, msg );
            }
        }
    }
}


void SoulLoader::Run()
{
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "LoaderSoul", SoundSourceBlueprint::TypeMusic );

    while( !g_inputManager->controlEvent( ControlSkipMessage ) )
    {
        if( g_app->m_requestQuit ) break;
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        float oldTime = m_time;
        m_time = GetHighResTime();
        float timeThisFrame = m_time - oldTime;

        AdvanceAllSpirits( timeThisFrame );
        RenderAllSpirits();
        RenderMessage( timeThisFrame );

        g_windowManager->Flip();
		g_inputManager->Advance();
        g_inputManager->PollForEvents();
        Sleep(1);

        AdvanceSound();
    }

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Music LoaderSoul" );
}


SoulLoaderSpirit::SoulLoaderSpirit()
{
    m_positionOffset = frand(10.0f);
    m_xaxisRate = frand(2.0f);
    m_yaxisRate = frand(2.0f);
    m_zaxisRate = frand(2.0f);

    m_colour.Set( 100, 200, 100, 255 );

    m_life = 0.0f;
}


void SoulLoaderSpirit::Advance( float _time )
{
    //
    // Make me float around slowly

    m_life += _time;
    m_positionOffset += _time;
    m_xaxisRate += frand(1.0f);
    m_yaxisRate += frand(1.0f);
    m_zaxisRate += frand(1.0f);
    if( m_xaxisRate > 2.0f ) m_xaxisRate = 2.0f;
    if( m_xaxisRate < 0.0f ) m_xaxisRate = 0.0f;
    if( m_yaxisRate > 2.0f ) m_yaxisRate = 2.0f;
    if( m_yaxisRate < 0.0f ) m_yaxisRate = 0.0f;
    if( m_zaxisRate > 2.0f ) m_zaxisRate = 2.0f;
    if( m_zaxisRate < 0.0f ) m_zaxisRate = 0.0f;

    Vector3 vel;

    vel.x = sinf( m_positionOffset ) * m_xaxisRate;
    vel.z = sinf( m_positionOffset ) * m_zaxisRate;

    vel.y = 3.0f + frand(3.0f);

    m_pos += vel * _time;
}


void SoulLoaderSpirit::Render()
{
    int innerAlpha = 255;
    int outerAlpha = 150;
    float spiritInnerSize = 1.0;
    float spiritOuterSize = 3.0;

    if( m_life < 10.0f )
    {
        innerAlpha *= ( m_life / 10.0f );
        outerAlpha *= ( m_life / 10.0f );
    }

    Vector3 camUp(0,1,0);
    Vector3 camRight(1,0,0);

    float size = spiritInnerSize;
    glColor4ub(m_colour.r, m_colour.g, m_colour.b, innerAlpha );
    glBegin( GL_QUADS );
        glVertex3fv( (m_pos - camUp*size).GetData() );
        glVertex3fv( (m_pos + camRight*size).GetData() );
        glVertex3fv( (m_pos + camUp*size).GetData() );
        glVertex3fv( (m_pos - camRight*size).GetData() );
    glEnd();

    size = spiritOuterSize;
    glColor4ub(m_colour.r, m_colour.g, m_colour.b, outerAlpha );
    glBegin( GL_QUADS );
        glVertex3fv( (m_pos - camUp*size).GetData() );
        glVertex3fv( (m_pos + camRight*size).GetData() );
        glVertex3fv( (m_pos + camUp*size).GetData() );
        glVertex3fv( (m_pos - camRight*size).GetData() );
    glEnd();
}

