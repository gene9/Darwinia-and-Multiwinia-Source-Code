#include "lib/universal_include.h"

#ifdef USE_SEPULVEDA_HELP_TUTORIAL

#include <SDL_thread.h>

#include "lib/debug_utils.h"
#include "lib/input/input.h"
#include "lib/filesys/text_stream_readers.h"

#include "app.h"
#include "gesture_demo.h"
#include "main.h"
#include "renderer.h"

static int SDLRecordingThread(void *_param)
{
	GestureRecordingThread();
	return 0;
}

void RecordGestureDemo()
{
	SDL_Thread *thread = SDL_CreateThread(SDLRecordingThread, 0);
 	AppReleaseAssert(thread, "Couldn't create gesture recording thread");
}

#endif
