#include "lib/universal_include.h"

#include <string.h>

#include "lib/debug_utils.h"
#include "lib/string_utils.h"

#include "eclipse.h"
#include "eclbutton.h"

#pragma warning( disable : 4996 )

EclButton::EclButton ()
:   m_x(0),
    m_y(0),
    m_w(0),
    m_h(0),
    m_caption(NULL),
	m_captionIsLanguagePhrase(false),
    m_tooltip(NULL),
	m_tooltipIsLanguagePhrase(false),
    m_parent(NULL)
{
	strcpy ( m_name, "New Button" );
}

EclButton::~EclButton ()
{

	if ( m_caption ) delete [] m_caption;	
	if ( m_tooltip ) delete [] m_tooltip;

}

void EclButton::SetProperties ( const char *_name, int _x, int _y, int _w, int _h,
		 	   				    const char *_caption, const char *_tooltip,
		 	   				    bool _captionIsLanguagePhrase, bool _tooltipIsLanguagePhrase )
{

    if ( strlen(_name) > SIZE_ECLBUTTON_NAME )
    {
        AppReleaseAssert( false, "Button name too long : %s", _name );
    }
    else
    {
        strcpy ( m_name, _name );
    }

	m_x = _x;
	m_y = _y;
	m_w = _w;
	m_h = _h;
	if ( !_caption )
	{
		SetCaption ( _name, false );
	}
	else
	{
		SetCaption ( _caption, _captionIsLanguagePhrase );
	}
	if ( !_tooltip || _tooltip[0] == '\0' || ( _tooltip[0] == ' ' && _tooltip[1] == '\0' ) )
	{
		SetTooltip ( " ", false );
	}
	else
	{
		SetTooltip ( _tooltip, _tooltipIsLanguagePhrase );
	}
	
}

void EclButton::SetCaption ( const char *_caption, bool _captionIsLanguagePhrase )
{

	if ( m_caption ) delete [] m_caption;
	m_caption = new char [strlen (_caption) + 1];
	strcpy ( m_caption, _caption );

	m_captionIsLanguagePhrase = _captionIsLanguagePhrase;

}

void EclButton::SetTooltip ( const char *_tooltip, bool _tooltipIsLanguagePhrase )
{
	if ( m_tooltip ) delete [] m_tooltip;

    if( _tooltip )
    {
	    m_tooltip = newStr ( _tooltip );
    }

	m_tooltipIsLanguagePhrase = _tooltipIsLanguagePhrase;
}

void EclButton::SetParent ( EclWindow *_parent )
{
    m_parent = _parent;
}

void EclButton::Render ( int realX, int realY, bool highlighted, bool clicked )
{
	// Unreferenced formal parameters
	((void)sizeof(realX)); ((void)sizeof(realY));
	((void)sizeof(highlighted)); ((void)sizeof(clicked));
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

void EclButton::Keypress ( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii )
{
	// Unreferenced formal parameters
	((void)sizeof(keyCode)); ((void)sizeof(shift));
	((void)sizeof(ctrl)); ((void)sizeof(alt));
	((void)sizeof(ascii));
}


