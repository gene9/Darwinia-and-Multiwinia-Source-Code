#ifndef _included_treewindow_h
#define _included_treewindow_h


#ifdef LOCATION_EDITOR


#include "interface/darwinia_window.h"


class TreeWindow : public DarwiniaWindow
{
public:
    int m_selectionId;

public:
    TreeWindow( char *_name );

    void Create();
    void Update();
};


#endif // LOCATION_EDITOR

#endif
