#include "lib/universal_include.h"
#include "lib/debug_utils.h"

#include "sound_library_3d.h"

#include <memory.h>


//*****************************************************************************
// Class SoundLibrary3d
//*****************************************************************************

SoundLibrary3d *g_soundLibrary3d = NULL;


SoundLibrary3d::SoundLibrary3d()
:   m_masterVolume(0),
    m_hw3dDesired(false),
    m_mainCallback(NULL),
	m_listenerPos(0,0,0),
	m_sampleRate(-1),
    m_numChannels(0),
    m_numMusicChannels(0)
{
}


SoundLibrary3d::~SoundLibrary3d()
{
}


void SoundLibrary3d::EnableCallback(bool _enabled)
{
	// do nothing in the base class
}

void SoundLibrary3d::SetMainCallback( bool (*_callback) (unsigned int, signed short *, unsigned int, int *) )
{
    m_mainCallback = _callback;
}


int SoundLibrary3d::GetSampleRate()
{
    return m_sampleRate; 
}


int	SoundLibrary3d::GetNumChannels() const
{ 
    return m_numChannels; 
}


int SoundLibrary3d::GetNumMusicChannels() const
{
    return m_numMusicChannels;
}


void SoundLibrary3d::SetMasterVolume( int _volume )
{
	AppReleaseAssert(_volume >=0 && _volume <= 255, "Invalid value passed to SoundLibrary3d::SetMasterVolume");

	// Converts 0-255 into 1/100ths of a decibel of attenuation
	m_masterVolume = (255 - _volume) * -32;
}


int SoundLibrary3d::GetMasterVolume() const
{
	return int(m_masterVolume / 32.0f) + 255;
}


void SoundLibrary3d::SetDopplerFactor( float _doppler )
{
}


void SoundLibrary3d::WriteSilence( signed short *_data, unsigned int _numSamples )
{
    memset( _data, 0, _numSamples * 2 );
}

