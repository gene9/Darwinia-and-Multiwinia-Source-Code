#include "lib/universal_include.h"

#ifdef USE_SEPULVEDA_HELP_TUTORIAL

#include "lib/debug_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/input/input.h"
#include "lib/targetcursor.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/resource.h"

#include "app.h"
#include "gesture_demo.h"
#include "main.h"
#include "renderer.h"


#define GESTURE_DEMO_SAMPLE_PERIOD	40			// ms
#define GESTURE_DEMO_SAMPLE_FREQ	(1000/40)	// hz


//*****************************************************************************
// Class GestureDemo
//*****************************************************************************

GestureDemo::GestureDemo(TextReader *_in)
{
	m_coords.SetStepDouble();

	GestureMouseCoords coords;
	while (_in->ReadLine())
	{
		coords.x = atof(_in->GetNextToken());
		coords.y = atof(_in->GetNextToken());
		m_coords.PutData(coords);
	}
}


float GestureDemo::GetTotalDuration()
{
	float totalDuration = m_coords.NumUsed() * GESTURE_DEMO_SAMPLE_PERIOD;
	return totalDuration * 0.001;
}


GestureMouseCoords GestureDemo::GetCoords(float _timeSinceStart)
{
	float indexAsDouble = _timeSinceStart * GESTURE_DEMO_SAMPLE_FREQ;
	int lowIndex = floorf(indexAsDouble);
	int highIndex = ceilf(indexAsDouble);

	GestureMouseCoords coords;

	if (highIndex >= m_coords.NumUsed())
	{
		coords = m_coords[m_coords.NumUsed() - 1];
	}
	else
	{
		float factor1 = indexAsDouble - lowIndex;
		float factor2 = 1.0 - factor1;
		coords.x = factor1 * m_coords[lowIndex].x + factor2 * m_coords[highIndex].x;
		coords.y = factor1 * m_coords[lowIndex].y + factor2 * m_coords[highIndex].y;
	}

	float scale = g_app->m_renderer->ScreenH();
	coords.x *= scale;
	coords.y *= scale;

	return coords;
}


//*****************************************************************************
// Recording Gesture Demo Stuff
//*****************************************************************************

void GestureRecordingThread()
{
	TextFileWriter *out = g_app->m_resource->GetTextFileWriter("gesture_demos/new_gesture_demo.txt", false);
	AppReleaseAssert(out, "Couldn't open file %s for writing", "gesture_demos/new_gesture_demo.txt");
	float scale = 1.0 / (float)g_app->m_renderer->ScreenH();

	// Wait for the start of the gesture
	while(1)
	{
		if ( g_inputManager->controlEvent( ControlGestureActive ) )
		{
			break;
		}
		Sleep(GESTURE_DEMO_SAMPLE_PERIOD);
	}

	while ( g_inputManager->controlEvent( ControlGestureActive ) )
	{
		Sleep(GESTURE_DEMO_SAMPLE_PERIOD);
		float x = g_target->X();
		x *= scale;
		float y = g_target->Y();
		y *= scale;
		out->printf( "%.3f %.3f\n", x, y);
	}

	delete out;
}

#endif // USE_SEPULVEDA_HELP_TUTORIAL
