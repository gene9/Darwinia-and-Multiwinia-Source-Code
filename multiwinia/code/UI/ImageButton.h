#ifndef __IMAGEBUTTON__
#define __IMAGEBUTTON__

#include "../game_menu.h"

class ImageButton : public DarwiniaButton
{
public:
    char *m_filename;
    bool    m_border;
public:
    ImageButton( char *_filename );
    void Render( int realX, int realY, bool highlighted, bool clicked );

};

#endif
