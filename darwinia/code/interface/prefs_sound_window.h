
#ifndef _included_prefssoundwindow_h
#define _included_prefssoundwindow_h

#include "interface/darwinia_window.h"


class PrefsSoundWindow : public DarwiniaWindow
{
public:
    int     m_soundLib;
    int     m_mixFreq;
    int     m_numChannels;
    int     m_useHardware3D;
    int     m_swapStereo;
    int     m_dspEffects;
    int     m_memoryUsage;

public:
    PrefsSoundWindow();

    void Create();
    void Render( bool _hasFocus );
};


#endif