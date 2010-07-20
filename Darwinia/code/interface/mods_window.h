#ifndef _included_modswindow_h
#define _included_modswindow_h

#include "interface/darwinia_window.h"



class ModsWindow : public DarwiniaWindow
{
public:
    ModsWindow();

    void Create();
    void Render( bool _hasFocus );
};


class NewModWindow : public DarwiniaWindow
{
public:
    static char s_modName[256];

public:
    NewModWindow();

    void Create();
};


#endif