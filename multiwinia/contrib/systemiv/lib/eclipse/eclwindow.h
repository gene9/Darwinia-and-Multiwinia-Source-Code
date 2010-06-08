
/*
        A Window object.
        Essentially a container for Eclipse buttons.

        All co-ordinates are RELATIVE to the window

  */


#ifndef _included_eclwindow_h
#define _included_eclwindow_h

#define SIZE_ECLWINDOW_NAME     256
#define SIZE_ECLWINDOW_TITLE    256

#include "lib/tosser/llist.h"

class EclButton;


class EclWindow
{
public:
    int         m_x;
    int         m_y;
    int         m_w;
    int         m_h;

    char        m_name  [SIZE_ECLWINDOW_NAME];
    char        m_title [SIZE_ECLWINDOW_TITLE];
    bool        m_titleIsLanguagePhrase;

    bool        m_movable;
    int         m_minW;
    int         m_minH;

    LList       <EclButton *> m_buttons;

public:
    char        m_currentTextEdit[SIZE_ECLWINDOW_NAME];

public:
	EclWindow                       ( const char *_name, const char *_title = NULL, bool _titleIsLanguagePhrase = false );
    virtual ~EclWindow  ();

    void SetName                    ( const char *_name );
    void SetTitle                   ( const char *_title, bool _titleIsLanguagePhrase = false );
    void SetPosition                ( int _x, int _y );
    void SetSize                    ( int _w, int _h );
    void SetMovable                 ( bool _movable );

    void RegisterButton             ( EclButton *button );
    void RemoveButton               ( const char *_name );

    void BeginTextEdit              ( const char *_name );
    void EndTextEdit                ();

    virtual EclButton   *GetButton  ( const char *_name );
    virtual EclButton   *GetButton  ( int _x, int _y );                             

    virtual void Create ();
    virtual void Remove ();
    virtual void Update ();
    virtual void Render ( bool hasFocus );

    virtual void Keypress   ( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii );
    virtual void MouseEvent ( bool lmb, bool rmb, bool up, bool down );

};


#endif
