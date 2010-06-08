#ifndef INCLUDED_SAVE_ON_QUIT_WINDOW_H
#define INCLUDED_SAVE_ON_QUIT_WINDOW_H

#include "interface/darwinia_window.h"


class SaveOnQuitWindow : public DarwiniaWindow
{
public:
    SaveOnQuitWindow( char *_name );

    void Create();
	void Render(bool hasFocus);
};


#endif
