
#ifndef _included_core_h
#define _included_core_h

#include "lib/eclipse/eclipse.h"
#include "lib/render/colour.h"

class InterfaceButton;


/*
 * ================
 * INTERFACE WINDOW
 * ================
 */
class InterfaceWindow : public EclWindow
{
public:
    float m_creationTime;

public:  
    InterfaceWindow( char *name, char *title = NULL, bool titleIsLanguagePhrase = false );
    
    void Create();
    void Remove();
    void Render( bool hasFocus );

    void Centralise();

    void CreateValueControl( char *name, int x, int y, int w, int h,
                             int dataType, void *value, float change, float _lowBound, float _highBound,
                             InterfaceButton *callback = NULL, char *tooltip = NULL, bool tooltipIsLanguagePhrase = false );

    void RemoveValueControl( char *name );

    static void RenderWindowShadow( float _x, float _y, float _h, float _w, float _size, float _alpha );
};


/*
 * ================
 * INTERFACE BUTTON
 * ================
 */
class InterfaceButton : public EclButton
{
public:   
    void Render( int realX, int realY, bool highlighted, bool clicked );
};


/*
 * ============
 * CLOSE BUTTON
 * ============
 */
class CloseButton : public InterfaceButton
{
public:
    CloseButton();
    void Render(int realX, int realY, bool highlighted, bool clicked );
    void MouseUp();
};



/*
 * ===========
 * TEXT BUTTON
 * ===========
 */
class TextButton : public EclButton
{
public:
    float m_fontSize;
    Colour m_fontCol;

public:
    TextButton();
    void Render( int realX, int realY, bool highlighted, bool clicked );
};


/*
 * =================
 * TEXT INPUT BUTTON
 * =================
 *
 * We would really recommend you use an InputField instead
 * as they have much more advanced functionality
 *
 */
class TextInputButton : public TextButton
{
public:
    void MouseUp();
    void Keypress ( int keyScancode, bool shift, bool ctrl, bool alt, unsigned char ascii );        
};


/*
 *  ============
 *	INVERTED BOX
 *  ============
 */

class InvertedBox : public EclButton
{
public:
    void Render( int realX, int realY, bool highlighted, bool clicked );
};


#endif


