
#ifndef _included_prefsgraphicswindow_h
#define _included_prefsgraphicswindow_h

#include "interface/darwinia_window.h"
#include "interface/mainmenus.h"


class PrefsGraphicsWindow : public GameOptionsWindow
{
public:
    int     m_landscapeDetail;
    int     m_waterDetail;
    int     m_cloudDetail;
    int     m_buildingDetail;
    int     m_entityDetail;
    int     m_pixelEffectRange;
    
public:
    PrefsGraphicsWindow();
    ~PrefsGraphicsWindow();

    void Create();
    void Render( bool _hasFocus );
};


#endif