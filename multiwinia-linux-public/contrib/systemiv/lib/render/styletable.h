#ifndef _included_styletable_h
#define _included_styletable_h

/*
 * STYLE TABLE
 *
 *	A generic repository for visual styles
 *  Eg a colour table with named colour values
 *  Eg a font table with named font types and sizes
 *
 *
 */

#include "lib/tosser/llist.h"
#include "colour.h"

class Style;

#define STYLE_WINDOW_BACKGROUND         "WindowBackground"
#define STYLE_WINDOW_TITLEBAR           "WindowTitleBar"
#define STYLE_WINDOW_OUTERBORDER        "WindowOuterBorder"
#define STYLE_WINDOW_INNERBORDER        "WindowInnerBorder"

#define STYLE_BUTTON_BACKGROUND         "ButtonBackground"
#define STYLE_BUTTON_HIGHLIGHTED        "ButtonHighlighted"
#define STYLE_BUTTON_CLICKED            "ButtonClicked"
#define STYLE_BUTTON_BORDER             "ButtonBorder"

#define STYLE_INPUT_BACKGROUND          "InputBackground"
#define STYLE_INPUT_BORDER              "InputBorder"

#define STYLE_BOX_BACKGROUND            "BoxBackground"
#define STYLE_BOX_BORDER                "BoxBorder"

#define FONTSTYLE_WINDOW_TITLE          "FontWindowTitle"
#define FONTSTYLE_BUTTON                "FontButton"


// ============================================================================


class StyleTable
{
public:
    char            *m_filename;
    LList           <Style *> m_styles;

public:
    StyleTable();

    void    AddStyle            ( Style *_style );
    Style   *GetStyle           ( char *_name );

    Colour  GetPrimaryColour    ( char *_name );
    Colour  GetSecondaryColour  ( char *_name );

    void    Clear               ();
    bool    Load                ( char *_filename );
    bool    Save                ( char *_filename=NULL );
};


// ============================================================================


class Style
{
public:
    char    m_name[256];
    bool    m_font;
    
    Colour  m_primaryColour;
    Colour  m_secondaryColour;    
    
    char    m_fontName[256];
    
    bool    m_negative;
    bool    m_horizontal;
    bool    m_uppercase;
    float   m_size;

public:
    Style( char *_name );
};





extern StyleTable *g_styleTable;


#endif
