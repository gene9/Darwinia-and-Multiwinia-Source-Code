#include "lib/universal_include.h"
#include "lib/render/renderer.h"
#include "lib/render/styletable.h"

#include "checkbox.h"


CheckBox::CheckBox()
:   EclButton(),
    m_value(NULL)
{
}


void CheckBox::RegisterBool( bool *_value )
{
    m_value = _value;
}


void CheckBox::Render( int realX, int realY, bool highlighted, bool clicked )
{    
    Colour background   = g_styleTable->GetPrimaryColour(STYLE_INPUT_BACKGROUND);
    Colour borderA      = g_styleTable->GetPrimaryColour(STYLE_INPUT_BORDER);
    Colour borderB      = g_styleTable->GetSecondaryColour(STYLE_INPUT_BORDER);

    g_renderer->Line        ( realX, realY+1, realX+m_w, realY+1, borderA );
    g_renderer->Line        ( realX+1, realY, realX+1, realY+m_h, borderA );

    g_renderer->RectFill    ( realX, realY, m_w, m_h, background );
    g_renderer->Line        ( realX, realY, realX + m_w, realY, borderB );
    g_renderer->Line        ( realX, realY, realX, realY + m_h, borderB );
    g_renderer->Line        ( realX + m_w, realY, realX + m_w, realY + m_h, borderB );
    g_renderer->Line        ( realX, realY + m_h, realX + m_w, realY + m_h, borderB );    
    
    if( m_value && *m_value == true )
    {
        float inset = 4;
        Colour col(255,255,255,180);
        g_renderer->Line( realX+inset, realY+inset, realX+m_w-inset, realY+m_h-inset, col );
        g_renderer->Line( realX+inset, realY+m_h-inset, realX+m_w-inset, realY+inset, col );
    }
}


void CheckBox::MouseUp()
{
    if( m_value )
    {
        if( *m_value )
        {
            *m_value = false;
        }
        else
        {
            *m_value = true;
        }
    }
}

