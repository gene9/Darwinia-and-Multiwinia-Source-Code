#ifndef INCLUDED_SOUND_PROFILE_WINDOW_H
#define INCLUDED_SOUND_PROFILE_WINDOW_H

#ifdef SOUND_EDITOR

#include "interface/darwinia_window.h"

#define SOUND_PROFILE_WINDOW_NAME "Sound Profile"



class SoundProfileWindow: public DarwiniaWindow
{
public:
    SoundProfileWindow( char *_name );
    ~SoundProfileWindow();

    void Create();
    void Render( bool hasFocus );
};

#endif // SOUND_EDITOR

#endif
