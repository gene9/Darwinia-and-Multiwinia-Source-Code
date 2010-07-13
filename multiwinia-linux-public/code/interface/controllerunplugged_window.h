#ifndef _included_controllerunplugged_window_h
#define _included_controllerunplugged_window_h

#include "interface/mainmenus.h"

class ControllerUnpluggedWindow : public GameOptionsWindow
{
public:
    bool    m_dialogCreated;

public:
    ControllerUnpluggedWindow();

    void Update();
    void Render( bool _hasFocus );
};

#endif