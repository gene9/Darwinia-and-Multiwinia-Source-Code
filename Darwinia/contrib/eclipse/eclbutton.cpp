
// Source file for button class
// Part of the Eclipse interface library
// By Christopher Delay

//#include "lib/universal_include.h"


#include <stdio.h>
#include <string.h>

#include "eclipse.h"
#include "eclbutton.h"


EclButton::EclButton ()
:   m_x(0),
    m_y(0),
    m_w(0),
    m_h(0),
    m_caption(NULL),
    m_tooltip(NULL),
    m_parent(NULL)
{
	strcpy ( m_name, "New Button" );
	SetTooltip ( " " );
}

EclButton::~EclButton ()
{

	if ( m_caption ) delete m_caption;	
	if ( m_tooltip ) delete m_tooltip;

}

void EclButton::SetProperties ( char *_name, int _x, int _y, int _w, int _h,
		 	   				    char *_caption, char *_tooltip )
{

	if ( !_caption )
	{
		_caption = _name;
	}

    if ( strlen(_name) > SIZE_ECLBUTTON_NAME )
    {
    }
    else
    {
        strcpy ( m_name, _name );
    }

	m_x = _x;
	m_y = _y;
	m_w = _w;
	m_h = _h;
	SetCaption ( _caption );
	SetTooltip ( _tooltip );
	
}

void EclButton::SetCaption ( const char *_caption )
{

	if ( m_caption ) delete [] m_caption;
	if ( _caption ) {
		m_caption = new char [strlen (_caption) + 1];
		strcpy ( m_caption, _caption );
	} else {
		m_caption = new char[1];
		*m_caption = '\0';
	}

}

void EclButton::SetTooltip ( char *_tooltip )
{

	if ( !_tooltip ) _tooltip = "";
	if ( m_tooltip ) delete [] m_tooltip;
	m_tooltip = new char [strlen (_tooltip) + 1];
	strcpy ( m_tooltip, _tooltip );

}

void EclButton::SetParent ( EclWindow *_parent )
{
    m_parent = _parent;
}

void EclButton::Render ( int realX, int realY, bool highlighted, bool clicked )
{
}

void EclButton::MouseUp ()
{
}

void EclButton::MouseDown ()
{
}

void EclButton::MouseMove ()
{
}

void EclButton::Keypress ( int keyCode, bool shift, bool ctrl, bool alt )
{
}


