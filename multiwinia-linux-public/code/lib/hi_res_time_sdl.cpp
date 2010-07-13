#include "lib/universal_include.h"

#include <SDL/SDL.h>

static unsigned int g_ticksPerSec = 0;
static double g_tickInterval = 1.0;
static double g_lastGetHighResTime = 0.0;
static double g_timeShift = 0.0;
static bool g_usingFakeTimeMode = false;
static double g_fakeTime;

// *** InitialiseHighResTime
void InitialiseHighResTime()
{
}



#include <sys/time.h>
#include <time.h>

// *** GetHighResTime
inline double GetLowLevelTime()
{
	static struct timeval firstTime;
	static bool gotFirst = false;

	struct timeval t;
	gettimeofday(&t, NULL);

	if (!gotFirst) {
		gotFirst = true;
		firstTime = t;
	}
	
	return (t.tv_sec - firstTime.tv_sec) + t.tv_usec / 1000000.0;
}

double GetHighResTime()
{
    if (g_usingFakeTimeMode)
    {
	    g_lastGetHighResTime = g_fakeTime;
        return g_fakeTime;
    }

	double timeNow = GetLowLevelTime();
    timeNow -= g_timeShift;

//    if( timeNow > g_lastGetHighResTime + 1.0 )
//    {
//        g_timeShift += timeNow - g_lastGetHighResTime;
//        timeNow -= timeNow - g_lastGetHighResTime;
//    }

    g_lastGetHighResTime = timeNow;
    return timeNow;
}


void SetFakeTimeMode()
{
	g_fakeTime = GetHighResTime();
	g_usingFakeTimeMode = true;
}


void SetRealTimeMode()
{
	g_usingFakeTimeMode = false;
	double realTime = GetLowLevelTime();
	g_timeShift = realTime - g_fakeTime;
}


void IncrementFakeTime(double _increment)
{
    g_fakeTime += _increment;
}
