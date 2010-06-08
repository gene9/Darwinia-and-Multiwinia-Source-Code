

// Button class
// Part of the Eclipse interface library
// By Christopher Delay

#ifndef _included_eclbutton_h
#define _included_eclbutton_h


#define SIZE_ECLBUTTON_NAME     256

class EclWindow;



class EclButton
{

public:

	char    m_name [SIZE_ECLBUTTON_NAME];
	int     m_x;
	int     m_y;
	int     m_w;
	int     m_h;
	char    *m_caption;
	char    *m_tooltip;

protected:

    EclWindow *m_parent;

public:

	EclButton ();
	virtual ~EclButton ();

    virtual void SetProperties   ( char *_name, int _x, int _y, int _w, int _h,
			        		       char *_caption=NULL, char *_tooltip=NULL );

	virtual void SetCaption      ( const char *_caption );
	virtual void SetTooltip      ( char *_tooltip );
    virtual void SetParent       ( EclWindow *_parent );

	virtual void Render     ( int realX, int realY, bool highlighted, bool clicked );
	virtual void MouseUp    ();
	virtual void MouseDown  ();
	virtual void MouseMove  ();
    virtual void Keypress   ( int keyCode, bool shift, bool ctrl, bool alt );

};

#endif
