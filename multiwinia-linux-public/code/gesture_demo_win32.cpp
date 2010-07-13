#include "lib/universal_include.h"

#if defined(GESTURE_EDITOR) && defined(USE_SEPULVEDA_HELP_TUTORIAL)

//#include <SDL_thread.h>

#include "lib/debug_utils.h"
#include "lib/filesys/text_stream_readers.h"

#include "app.h"
#include "gesture_demo.h"
#include "main.h"
#include "renderer.h"

static DWORD WINAPI Win32RecordingThread(void *_param)
{
	GestureRecordingThread();
	return 0;
}

void RecordGestureDemo()
{
	HANDLE threadHandle; 
	unsigned long threadId;

    threadHandle = CreateThread( 
        NULL,                        // default security attributes 
        0,                           // use default stack size  
        Win32RecordingThread,        // thread function 
        NULL,		                 // argument to thread function 
        0,                           // use default creation flags 
        &threadId);                  // returns the thread identifier 
 
 	AppReleaseAssert(threadHandle, "Couldn't create gesture recording thread");
	SetThreadPriority(threadHandle, THREAD_PRIORITY_TIME_CRITICAL);	
}

#endif // GESTURE_EDITOR && USE_SEPULVEDA_HELP_TUTORIAL
