#ifndef _included_styleeditorwindow_h
#define _included_styleeditorwindow_h

#include "interface/components/core.h"

class ScrollBar;



class StyleEditorWindow : public InterfaceWindow
{
public:
    char        m_filename[256];
    ScrollBar   *m_scrollbar;

public:
    StyleEditorWindow();
    ~StyleEditorWindow();

    void Create();
    void Render( bool _hasFocus );
};


#endif
