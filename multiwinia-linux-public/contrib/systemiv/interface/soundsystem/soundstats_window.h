#ifndef _included_soundstatswindow_h
#define _included_soundstatswindow_h

#include "interface/components/core.h"

class ScrollBar;


class SoundStatsWindow : public InterfaceWindow
{
protected:
    ScrollBar *m_scrollBar;
        
public:
    SoundStatsWindow();
    ~SoundStatsWindow();

    void Create();
    void Render( bool hasFocus );
};



#endif
