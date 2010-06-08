#ifndef INCLUDED_GESTURE_DEMO_H
#define INCLUDED_GESTURE_DEMO_H

#ifdef USE_SEPULVEDA_HELP_TUTORIAL

#include "lib/tosser/fast_darray.h"


class TextReader;


//*****************************************************************************
// Class GestureMouseCoords
//*****************************************************************************

class GestureMouseCoords
{
public:
	float x, y;
};


//*****************************************************************************
// Class GestureDemo
//*****************************************************************************

class GestureDemo
{
protected:
	FastDArray <GestureMouseCoords> m_coords;

public:
	GestureDemo(TextReader *_in);

	// Returns a mouse pos scaled for the current screen resolution
	GestureMouseCoords GetCoords(float _timeSinceStart);
	float GetTotalDuration();	// in seconds
};


//*****************************************************************************
// Gesture Demo Recorder
//*****************************************************************************

// Produces a file data/gesture_demos/new_gesture_demo.txt
void RecordGestureDemo();
void GestureRecordingThread();

#endif // USE_SEPULVEDA_HELP_TUTORIAL

#endif
