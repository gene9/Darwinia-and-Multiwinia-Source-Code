
#ifndef _included_prefsscreenwindow_h
#define _included_prefsscreenwindow_h

#include "interface/darwinia_window.h"


class PrefsScreenWindow : public DarwiniaWindow
{
public:
    int     m_resId;
    int     m_windowed;
    int     m_colourDepth;
    int     m_refreshRate;
    int     m_zDepth;

public:
    PrefsScreenWindow();

    void Create();
    void Render( bool _hasFocus );
};

void SetWindowed(bool _isWindowed, bool _isPermanent, bool &_isSwitchingWindowed);
void RestartWindowManagerAndRenderer();

#endif