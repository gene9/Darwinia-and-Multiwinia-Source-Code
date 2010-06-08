

// Button class
// Part of the Eclipse interface library
// By Christopher Delay

#ifndef _included_eclbutton_h
#define _included_eclbutton_h

#include "lib/unicode/unicode_string.h"
#include "lib/language_table.h"

#define SIZE_ECLBUTTON_NAME     256

class EclWindow;



class EclButton
{

public:

	char				m_name [SIZE_ECLBUTTON_NAME];
	int					m_x;
	int					m_y;
	int					m_w;
	int					m_h;
	UnicodeString		m_caption;
	UnicodeString		m_tooltip;
	bool				m_inactive;

protected:

    EclWindow *m_parent;

public:

	EclButton ();
	virtual ~EclButton ();

    virtual void SetProperties   ( const char *_name, int _x, int _y, int _w, int _h,
			        		       const UnicodeString &_caption, const UnicodeString &_tooltip=UnicodeString() );

	virtual void SetCaption      ( const UnicodeString &_caption );
	virtual void SetTooltip      ( const UnicodeString &_tooltip );
    virtual void SetParent       ( EclWindow *_parent );

	virtual void Render     ( int realX, int realY, bool highlighted, bool clicked );
	virtual void MouseUp    ();
	virtual void MouseDown  ();
	virtual void MouseMove  ();
    virtual void Keypress   ( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii);

};

#endif
