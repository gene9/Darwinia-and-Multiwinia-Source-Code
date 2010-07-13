#ifndef _included_checkbox_h
#define _included_checkbox_h

#include "lib/eclipse/eclipse.h"



class CheckBox : public EclButton
{
public:
    bool *m_value;

public:
    CheckBox();
    
    void RegisterBool( bool *_value );
    
    void Render( int realX, int realY, bool highlighted, bool clicked );

    void MouseUp();
};



#endif

