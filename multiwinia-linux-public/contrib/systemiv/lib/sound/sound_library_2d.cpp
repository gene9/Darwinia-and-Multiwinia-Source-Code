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
