#ifndef _included_demoendwindow_h
#define _included_demoendwindow_h

#include "interface/darwinia_window.h"


class DemoEndWindow : public DarwiniaWindow
{
protected:
    float m_timer;
    float m_fadeInTime;

public:
    bool m_saveGame;

public:
    DemoEndWindow( float _fadeInTime, bool _saveGame );

    float GetAlpha();
    bool ShowExitButton();

    void Create();
    void Render( bool hasFocus );
};



#endif