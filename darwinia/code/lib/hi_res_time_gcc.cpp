#include "lib/universal_include.h"

#include <math.h>
#include "main.h"

#include "hi_res_time.h"


static unsigned int g_ticksPerSec = 0;
static double g_tickInterval = 1.0;
static double g_lastGetHighResTime = 0.0;
static double g_timeShift = 0.0;
static bool g_usingFakeTimeMode = false;
static double g_fakeTime;



// *** InitialiseHighResTime
void InitialiseHighResTime()
{
	// Start be getting the frequency the Performance Counter uses.
	// We need to use the Performance Counter to calibrate the Pentium
	// Counter, if we are going to use it.
    LARGE_INTEGER count;
    QueryPerformanceFrequency(&count);
    g_tickInterval = 1.0 / (double)count.QuadPart;
}


inline double GetLowLevelTime()
{
#if USE_PENTIUM_COUNTER
	static double wraps = 0.0;		// Each time the counter wraps, we increment this value by the wrap period
	static unsigned int lastCount = 0;
	unsigned int newCount = GetPentiumCounter();
	
	// Has the counter wrapped?
	if (newCount < lastCount)
	{
		wraps += g_tickInterval * pow(2, 32);
	}
	lastCount = newCount;
	
	return (double)newCount * g_tickInterval + wraps;
#else
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart * g_tickInterval;
#endif
}


// *** GetHighResTime
double GetHighResTime()
{
    if (g_usingFakeTimeMode)
    {
	    g_lastGetHighResTime = g_fakeTime;
        return g_fakeTime;
    }

	double timeNow = GetLowLevelTime();
    timeNow -= g_timeShift;

//    if( timeNow > g_lastGetHighResTime + 1.0f )
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
