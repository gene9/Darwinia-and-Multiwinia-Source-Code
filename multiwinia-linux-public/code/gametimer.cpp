#include "lib/universal_include.h"
#include "lib/input/input.h"
#include "lib/text_renderer.h"
#include "lib/hi_res_time.h"

#include "app.h"
#include "gametimer.h"
#include "main.h"


GameTimer g_gameTimer;


GameTimer::GameTimer()
:   m_gameSpeed(0.0f),
    m_gameTime(0.0f),
    m_lastClientAdvance(0.0f),
    m_estimatedServerStartTime(0.0f),
    m_accurateTime(0.0f),
    m_requestedSpeed(-1.0f),
    m_requestedSpeedOfChange(-1.0f),
    m_requestedSpeedLength(-1.0f),
	m_lastCallToAccurateTime(0.0),
    m_lockSpeedChange(false)
{
}


void GameTimer::Reset()
{
    m_estimatedServerStartTime = GetHighResTime();

    if( g_app->Multiplayer() )
    {
        m_gameSpeed = 0.0f;
    }
    else
    {
        m_gameSpeed = 1.0f;
    }

    m_gameTime = 0.0f;
    m_lastClientAdvance = 0.0f;
    m_requestedSpeed = -1;
    m_requestedSpeedOfChange = -1;

    m_lockSpeedChange = false;
}


bool GameTimer::IsPaused()
{
    return( m_gameSpeed < 0.01f );
}


void GameTimer::Tick()
{
    m_lastClientAdvance = GetHighResTime();

    if( !g_app->GamePaused() )
    {
        m_gameTime += GetServerAdvancePeriod();
    }


    //
    // Handle changes of game speed

    if( m_requestedSpeed >= 0.0 )
    {
        double diff = ( m_requestedSpeed - m_gameSpeed );
        double amountToAdd = diff * m_requestedSpeedOfChange;

        m_gameSpeed += amountToAdd;

        if( iv_abs( m_gameSpeed - m_requestedSpeed ) < 0.01 )
        {
            m_gameSpeed = m_requestedSpeed;
            m_requestedSpeed = -1.0;
        }
    }

    if( m_requestedSpeed < 0.0 || m_gameSpeed == m_requestedSpeed )
    {
        if( m_requestedSpeedLength >= 0.0 )
        {
            m_requestedSpeedLength -= 0.1;
            if( m_requestedSpeedLength <= 0.0 )
            {
                m_requestedSpeedLength = -1.0;
                m_requestedSpeed = 1.0;                
            }
        }
    }
}


double GameTimer::GetGameSpeed()
{
    return m_gameSpeed;
}


double GameTimer::GetRequestedGameSpeed()
{
    return m_requestedSpeed;
}


double GameTimer::GetAccurateTime()
{
    double timeNow = GetHighResTime();
    double timePassed = timeNow - m_lastCallToAccurateTime;

    m_lastCallToAccurateTime = timeNow;

    m_accurateTime += timePassed * m_gameSpeed;
    return m_accurateTime;
}

double GameTimer::GetAccurateTime( double _futureTime )
{
	double timePassed = (_futureTime - m_lastCallToAccurateTime);
	return m_accurateTime + timePassed * m_gameSpeed;
}


double GameTimer::GetServerAdvancePeriod()
{
    return 0.1 * m_gameSpeed;
}


double GameTimer::GetPredictionTime()
{
    if( g_app->GamePaused() )
    {
        return 0.0f;
    }

    double timeNow = GetHighResTime();
    double result = ( timeNow - m_lastClientAdvance );
    if( result > 1.0 ) result = 1.0;
    result *= m_gameSpeed;

    return result;
}


void GameTimer::RequestSpeedChange( double newSpeed, double speedOfChange, double totalTimeOfChange )
{
    if( m_lockSpeedChange ) return;

    m_requestedSpeed = newSpeed;
    m_requestedSpeedOfChange = speedOfChange;
    m_requestedSpeedLength = totalTimeOfChange;
}


void GameTimer::DebugRender()
{
    return;

    if( iv_abs( m_gameSpeed - 1.0f ) > 0.01f )
    {
        g_gameFont.DrawText2D( 10, 200, 20, "Game Speed : %2.2f", m_gameSpeed );
    }
}

void GameTimer::LockSpeedChange()
{
    m_lockSpeedChange = true;
}