#include "lib/universal_include.h"
#include "sound_library_2d.h"

SoundLibrary2d *g_soundLibrary2d = NULL;

SoundLibrary2d::SoundLibrary2d()
{
}


SoundLibrary2d::~SoundLibrary2d()
{
	g_soundLibrary2d = NULL;
}

// 
// SoundLibrary2dNoSound
//

void SoundLibrary2dNoSound::SetCallback(void (*_callback) (StereoSample *, unsigned int))
{
}

void SoundLibrary2dNoSound::StartRecordToFile(char const *_filename)
{
}

void SoundLibrary2dNoSound::EndRecordToFile()
{
}

void SoundLibrary2dNoSound::TopupBuffer()
{
}

void SoundLibrary2dNoSound::Stop()
{
}

