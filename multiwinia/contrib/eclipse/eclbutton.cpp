
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
    m_parent(NULL),
	m_inactive(false)
{
	strcpy ( m_name, "New Button" );
	SetTooltip ( UnicodeString(" ") );
}

EclButton::~EclButton ()
{

	//if ( m_caption ) delete m_caption;	
	//if ( m_tooltip ) delete m_tooltip;

}

void EclButton::SetProperties ( const char *_name, int _x, int _y, int _w, int _h,
		 	   				    const UnicodeString &_caption, const UnicodeString &_tooltip )
{
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

	if ( !&_caption )
	{
		// Could use LANGUAGEPHRASE here instead, but that's cross projecs, so probably shouldn't...
		SetCaption( UnicodeString(_name) );
	}
	else
	{
		SetCaption ( _caption );
	}

	SetTooltip ( _tooltip );
	
}

void EclButton::SetCaption ( const UnicodeString &_caption )
{
	m_caption = _caption;
}

void EclButton::SetTooltip(const UnicodeString &_tooltip )
{
	m_tooltip = _tooltip;
}


/***** Old code
void EclButton::SetCaption ( UnicodeString *_caption )
{

	if ( m_caption ) delete m_caption;
	if ( _caption ) {
		m_caption = _caption;
	} else {
		m_caption = new UnicodeString("");
	}

}

void EclButton::SetTooltip ( UnicodeString *_tooltip )
{

	if ( !_tooltip ) _tooltip = new UnicodeString("");
	if ( m_tooltip ) delete m_tooltip;
	m_tooltip = _tooltip;

}*/

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

void EclButton::Keypress ( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii)
{
}





