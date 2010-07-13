#ifndef SOUND_LIBRARY_2D_MODE_H
#define SOUND_LIBRARY_2D_MODE_H

/*
 * There are two modes of operation for this library, either
 * invoke the callback directly from the sound thread
 * or wait for the main thread to call TopupBuffer.
 * The former is better from a latency and efficiency perspective,
 * however, Andy tells me that under Windows at least this has
 * cause reliability problems.
 *
 * Appears to be ok on the Mac so far.
 */

#define INVOKE_CALLBACK_FROM_SOUND_THREAD

#endif // SOUND_LIBRARY_2D_MODE_H
