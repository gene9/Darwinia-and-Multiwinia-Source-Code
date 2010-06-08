
#ifndef _included_soundoptionswindow_h
#define _included_soundoptionswindow_h

#include "interface/components/core.h"

#define SOUNDOPTIONSWINDOW_USEHARDWARE3D
//#define SOUNDOPTIONSWINDOW_USEDSPEFFECTS


class SoundOptionsWindow : public InterfaceWindow
{
public:
    int     m_soundLib;
    int     m_mixFreq;
    int     m_numChannels;
    int     m_useHardware3D;
    int     m_swapStereo;
    int     m_dspEffects;
    int     m_memoryUsage;
    int     m_masterVolume;
    
public:
    SoundOptionsWindow();

    void Create();
    void Render( bool _hasFocus );
};


#endif

