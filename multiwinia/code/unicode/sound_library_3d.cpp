#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/profiler.h"

#include "sound/sound_library_3d.h"



//*****************************************************************************
// Class SoundLibrary3d
//*****************************************************************************

SoundLibrary3d *g_soundLibrary3d = NULL;


SoundLibrary3d::SoundLibrary3d()
:   m_masterVolume(0),
    m_hw3dDesired(false),
    m_mainCallback(NULL),
	m_musicCallback(NULL),
	m_musicChannelId(-1),
	m_listenerPos(0,0,0),
	m_sampleRate(-1),
    m_numChannels(0)
{
#ifdef PROFILER_ENABLED
    m_profiler = new Profiler;
#endif
}


SoundLibrary3d::~SoundLibrary3d()
{
}


void SoundLibrary3d::SetMainCallback( bool (*_callback) (unsigned int, signed short *, unsigned int, int *) )
{
    m_mainCallback = _callback;
}


void SoundLibrary3d::SetMusicCallback( bool (*_callback) (signed short *, unsigned int, int *) )
{
    m_musicCallback = _callback;
}


// 0 = silence, 255 = full volume
void SoundLibrary3d::SetMasterVolume( int _volume )
{
	AppReleaseAssert(_volume >=0 && _volume <= 255, "Invalid value passed to SoundLibrary3d::SetMasterVolume");

	// Converts 0-255 into 1/100ths of a decibel of attenuation
	m_masterVolume = (float)(255 - _volume) * -32.0f;
}


void SoundLibrary3d::WriteSilence( signed short *_data, unsigned int _numSamples )
{
    memset( _data, 0, _numSamples * 2 );
}
