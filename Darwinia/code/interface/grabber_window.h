#ifndef _included_grabberwindow_h
#define _included_grabberwindow_h

#ifdef AVI_GENERATOR

#include "interface/darwinia_window.h"


class GrabberWindow : public DarwiniaWindow
{
public:
    GrabberWindow( char *_name );

    void Create();
};

#endif // AVI_GENERATOR

#endif
