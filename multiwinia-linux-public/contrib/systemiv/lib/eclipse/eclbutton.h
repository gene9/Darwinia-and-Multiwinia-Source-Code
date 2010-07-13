

// Button class
// Part of the Eclipse interface library
// By Christopher Delay

#ifndef _included_eclbutton_h
#define _included_eclbutton_h


#define SIZE_ECLBUTTON_NAME     256

#ifndef NULL
#  define NULL 0
#endif

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
	bool    m_captionIsLanguagePhrase;
	char    *m_tooltip;
	bool    m_tooltipIsLanguagePhrase;

protected:
    EclWindow *m_parent;

public:
	EclButton ();
	virtual ~EclButton ();

    virtual void SetProperties   ( const char *_name, int _x, int _y, int _w, int _h,
	                               const char *_caption = NULL, const char *_tooltip = NULL, 
	                               bool _captionIsLanguagePhrase = false, bool _tooltipIsLanguagePhrase = false );

	virtual void SetCaption      ( const char *_caption, bool _captionIsLanguagePhrase = false );
	virtual void SetTooltip      ( const char *_tooltip, bool _tooltipIsLanguagePhrase = false );
    virtual void SetParent       ( EclWindow *_parent );

	virtual void Render     ( int realX, int realY, bool highlighted, bool clicked );
	virtual void MouseUp    ();
	virtual void MouseDown  ();
	virtual void MouseMove  ();
    virtual void Keypress   ( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii );

};

#endif
