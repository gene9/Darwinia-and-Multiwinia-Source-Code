#ifndef _included_gesturewindow_h
#define _included_gesturewindow_h


#ifdef GESTURE_EDITOR

#include "interface/darwinia_window.h"


class GestureWindow : public DarwiniaWindow
{
public:
    GestureWindow( char *_name );

    void Create();
    void Update();

    void Keypress( int keyCode, bool shift, bool ctrl, bool alt );
};


#endif


#endif
