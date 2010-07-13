
#ifndef _included_prefssoundwindow_h
#define _included_prefssoundwindow_h

#include "interface/darwinia_window.h"
#include "interface/mainmenus.h"
#include "UI/SoundSliderButton.h"


class GameMenuDropDown;
#ifdef USE_SOUND_HW3D
class HW3DDropDownMenu;
#endif

class PrefsSoundWindow : public GameOptionsWindow
{
public:
    int     m_soundLib;
    int     m_mixFreq;
    int     m_numChannels;
#ifdef USE_SOUND_HW3D
    int     m_useHardware3D;
#endif
    int     m_swapStereo;
#ifdef USE_SOUND_DSPEFFECTS
    int     m_dspEffects;
#endif
#ifdef USE_SOUND_MEMORY
    int     m_memoryUsage;
#endif

protected:
    GameMenuDropDown *m_menuSoundLib;
    GameMenuDropDown *m_menuMixFreq;
    GameMenuDropDown *m_menuNumChannels;
	VolumeControl	*m_menuVolume;
#ifdef USE_SOUND_MEMORY
    GameMenuDropDown *m_menuMemoryUsage;
#endif
    GameMenuCheckBox *m_menuSwapStereo;
#ifdef USE_SOUND_HW3D
    HW3DDropDownMenu *m_menuHw3d;
#endif
#ifdef USE_SOUND_DSPEFFECTS
    GameMenuCheckBox *m_menuDspEffects;
#endif

protected:
    void ReadValues();

public:
    PrefsSoundWindow();

    void Create();
    void Render( bool _hasFocus );

    void RefreshValues();
};


#endif
