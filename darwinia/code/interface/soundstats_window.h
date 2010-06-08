#ifndef _included_soundstatswindow_h
#define _included_soundstatswindow_h

#ifdef SOUND_EDITOR

#include "interface/darwinia_window.h"

class ScrollBar;

#define SOUND_STATS_WINDOW_NAME "Sound Stats"



class SoundStatsWindow : public DarwiniaWindow
{
protected:
    ScrollBar *m_scrollBar;

public:
    SoundStatsWindow( char *_name );
    ~SoundStatsWindow();

    void Create();
    void Render( bool hasFocus );
};

#endif // SOUND_EDITOR

#endif
