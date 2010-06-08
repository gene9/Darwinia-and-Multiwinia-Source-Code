#ifndef _included_gametimer_h
#define _included_gametimer_h

// Game Timer
//
// A network-safe timer to keep track of the exact time
// Allows the rate of time to change
// (eg proper pause support, 1/2 speed, 2x speed etc)
//
// Replaces g_gameTime, GetNetworkTime, SERVER_ADVANCE_PERIOD etc
//


#define    SERVER_ADVANCE_PERIOD       g_gameTimer.GetServerAdvancePeriod()
#define    g_predictionTime            g_gameTimer.GetPredictionTime()
#define    GetNetworkTime              g_gameTimer.GetGameTime



class GameTimer
{
public:
    double   m_gameSpeed;
    double   m_gameTime;
    double   m_accurateTime;
    double  m_lastClientAdvance;
    double  m_estimatedServerStartTime;
	
protected:
    double   m_requestedSpeed;
    double   m_requestedSpeedOfChange;
    double   m_requestedSpeedLength;

	double	m_lastCallToAccurateTime;

    bool    m_lockSpeedChange;

public:
    GameTimer();

    void    Reset();
    void    Tick();

    double   GetGameSpeed();
    double   GetGameTime();
    double   GetAccurateTime();                      // NOT net-safe, NOT thread-safe
	double   GetAccurateTime( double _futureTime );  // NOT net-safe, thread-safe
    double   GetServerAdvancePeriod();
    double   GetPredictionTime();
    double   GetRequestedGameSpeed();

    bool    IsPaused();

    void    RequestSpeedChange( double newSpeed, double speedOfChange, double totalTimeOfChange );          // speedOfChange : 1=immediate, < 1 = slower

    void DebugRender();

    void    LockSpeedChange();
};


inline double GameTimer::GetGameTime()
{
    return m_gameTime;
}


extern GameTimer g_gameTimer;


#endif
