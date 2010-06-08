#include "lib/universal_include.h"

#include <stdio.h>

#include "lib/eclipse/eclipse.h"
#include "lib/gucci/input.h"
#include "lib/gucci/window_manager.h"
#include "lib/render/renderer.h"
#include "lib/render/styletable.h"
#include "lib/language_table.h"
#include "lib/hi_res_time.h"
#include "lib/math/math_utils.h"

#include "core.h"
#include "inputfield.h"


InterfaceWindow::InterfaceWindow( char *name, char *title, bool titleIsLanguagePhrase )
:   EclWindow(EclGenerateUniqueWindowName(name), title, titleIsLanguagePhrase)
{
    m_creationTime = GetHighResTime();
}

void InterfaceWindow::Create()
{
    CloseButton *cb = new CloseButton();
    cb->SetProperties( "Close", m_w - 17, 4, 13, 13, " ", "tooltip_close_window", false, true );
    RegisterButton( cb );
}

void InterfaceWindow::Remove()
{
    m_buttons.EmptyAndDelete();
}


void InterfaceWindow::CreateValueControl( char *name, int x, int y, int w, int h,
                                          int dataType, void *value, float change, float _lowBound, float _highBound,
                                          InterfaceButton *callback, char *tooltip, bool tooltipIsLanguagePhrase )
{
    InputField *input = new InputField();
    
    if( dataType == InputField::TypeString )
    {
        input->SetProperties( name, x, y, w, h, name, tooltip, false, tooltipIsLanguagePhrase );
    }
    else
    {
        input->SetProperties( name, x+h+2, y, w-h*2-4, h, name, tooltip, false, tooltipIsLanguagePhrase );
    }

	input->m_lowBound = _lowBound;
	input->m_highBound = _highBound;
    input->SetCallback( callback );
    switch( dataType )
    {
        case InputField::TypeChar:      input->RegisterChar     ( (unsigned char *) value );    break;
        case InputField::TypeInt:       input->RegisterInt      ( (int *) value );              break;
        case InputField::TypeFloat:     input->RegisterFloat    ( (float *) value );            break;
        case InputField::TypeString:    input->RegisterString   ( (char *) value );             break;
    }
    
    RegisterButton( input );

    if( dataType != InputField::TypeString )
    {
        char nameLeft[64];
        sprintf( nameLeft, "%s left", name );
        InputScroller *left = new InputScroller();
        left->SetProperties( nameLeft, input->m_x - h-2, y, h, h, " ", "dialog_decrease", false, true );
        left->m_inputField = input;
        left->m_change = -change;
        RegisterButton( left );

        char nameRight[64];
        sprintf( nameRight, "%s right", name );
        InputScroller *right = new InputScroller();
        right->SetProperties( nameRight, input->m_x + input->m_w+2, y, h, h, " ", "dialog_increase", false, true );
        right->m_inputField = input;
        right->m_change = change;
        RegisterButton( right );
    }
}

void InterfaceWindow::RemoveValueControl( char *name )
{
    RemoveButton( name );

    char nameLeft[64];
    sprintf( nameLeft, "%s left", name );
    RemoveButton( nameLeft );

    char nameRight[64];
    sprintf( nameRight, "%s right", name );
    RemoveButton( nameRight );
}


void InterfaceWindow::Render ( bool hasFocus )
{
    g_renderer->SetClip( m_x, m_y, m_w, m_h );


    //
    // Draw the window bars

    Colour titleBarColA= g_styleTable->GetPrimaryColour( STYLE_WINDOW_TITLEBAR );
    Colour titleBarColB= g_styleTable->GetSecondaryColour( STYLE_WINDOW_TITLEBAR );
    Colour windowColA  = g_styleTable->GetPrimaryColour( STYLE_WINDOW_BACKGROUND );
    Colour windowColB  = g_styleTable->GetSecondaryColour( STYLE_WINDOW_BACKGROUND );
    Colour borderCol   = g_styleTable->GetPrimaryColour( STYLE_WINDOW_OUTERBORDER );
    Colour innerBorder = g_styleTable->GetPrimaryColour( STYLE_WINDOW_INNERBORDER );

    bool titleBarAlignment = g_styleTable->GetStyle(STYLE_WINDOW_TITLEBAR)->m_horizontal;
    bool windowAlignment   = g_styleTable->GetStyle(STYLE_WINDOW_BACKGROUND)->m_horizontal;

    g_renderer->RectFill ( m_x, m_y, m_w, 20, titleBarColA, titleBarColB, titleBarAlignment );
    g_renderer->RectFill ( m_x, m_y+20, m_w, m_h-21, windowColA, windowColB, windowAlignment );

    g_renderer->Rect     ( m_x, m_y, m_w-1, m_h-1, borderCol );
    g_renderer->Rect     ( m_x+1, m_y+20, m_w-4, m_h-22, innerBorder );
  

    //
    // Render the title caption

    Style *titleFont = g_styleTable->GetStyle( FONTSTYLE_WINDOW_TITLE );
    AppAssert(titleFont);
        
    if( stricmp( titleFont->m_fontName, "default" ) == 0 )
    {
        g_renderer->SetFont( NULL, false, titleFont->m_negative );        
    }
    else
    {
        g_renderer->SetFont( titleFont->m_fontName, false, titleFont->m_negative );
    }

    char caption[1024];
	if( m_titleIsLanguagePhrase )
	{
		snprintf( caption, sizeof(caption), LANGUAGEPHRASE(m_title) );
	}
	else
	{
		snprintf( caption, sizeof(caption), m_title );
	}
	caption[ sizeof(caption) - 1 ] = '\0';

    if( titleFont->m_uppercase ) strupr(caption);

    g_renderer->Text( m_x + 10, 
                      m_y + 11 - titleFont->m_size/2, 
                      titleFont->m_primaryColour, 
                      titleFont->m_size, 
                      caption );
    g_renderer->SetFont();

    
    //
    // Resizer widget bottom right

    g_renderer->Line    ( m_x+m_w-2, m_y+m_h-12, m_x+m_w-12, m_y+m_h-2, borderCol );
    g_renderer->Line    ( m_x+m_w-2, m_y+m_h-8, m_x+m_w-8, m_y+m_h-2, borderCol );

    //
    // Draw the buttons

    EclWindow::Render( hasFocus );
    
    g_renderer->ResetClip();


    //
    // Window shadow

    RenderWindowShadow( m_x+m_w, m_y, m_h, m_w, 15, g_renderer->m_alpha*0.7f );

    //
    // Focus glow

    if( hasFocus )
    {
        g_renderer->SetBlendMode( Renderer::BlendModeNormal );
        g_renderer->Rect( m_x-2, m_y-2, m_w+3, m_h+3, Colour(255,255,255,100), 1.0f );        
    }
}


void InterfaceWindow::Centralise()
{
    m_x = g_windowManager->WindowW()/2.0f - m_w/2.0f;
    m_y = g_windowManager->WindowH()/2.0f - m_h/2.0f;
}


void InterfaceWindow::RenderWindowShadow( float _x, float _y, float _h, float _w, float _size, float _alpha )
{
    glShadeModel( GL_SMOOTH );
    
    Colour strong(0,0,0,_alpha*255);
    Colour weak(0,0,0,0);

    g_renderer->RectFill( _x, _y+_size, _size, _h-_size, strong, weak, weak, strong );
    g_renderer->RectFill( _x, _y+_h, _size, _size, strong, weak, weak, weak );
    g_renderer->RectFill( _x-_w+_size, _y+_h, _w-_size, _size, strong, strong, weak, weak );
}


// ============================================================================
// InterfaceButton


void InterfaceButton::Render( int realX, int realY, bool highlighted, bool clicked )
{    
    char *styleName             = STYLE_BUTTON_BACKGROUND;
    if( highlighted ) styleName = STYLE_BUTTON_HIGHLIGHTED;
    if( clicked ) styleName     = STYLE_BUTTON_CLICKED;

    Colour primaryCol   = g_styleTable->GetPrimaryColour(styleName);
    Colour secondaryCol = g_styleTable->GetSecondaryColour(styleName);
    
    Colour borderPrimary    = g_styleTable->GetPrimaryColour(STYLE_BUTTON_BORDER);
    Colour borderSeconary   = g_styleTable->GetSecondaryColour(STYLE_BUTTON_BORDER);
    
    bool colourAlignment    = g_styleTable->GetStyle(styleName)->m_horizontal;

    g_renderer->RectFill    ( realX, realY, m_w, m_h, primaryCol, secondaryCol, colourAlignment );

    g_renderer->Line        ( realX, realY, realX+m_w, realY, borderPrimary );
    g_renderer->Line        ( realX, realY, realX, realY+m_h, borderPrimary );
    g_renderer->Line        ( realX, realY+m_h, realX+m_w, realY+m_h, borderSeconary );
    g_renderer->Line        ( realX+m_w, realY, realX+m_w, realY+m_h, borderSeconary );
    

    //
    // Render the caption

    Style *buttonFont = g_styleTable->GetStyle(FONTSTYLE_BUTTON);
    AppAssert(buttonFont);

    if( stricmp( buttonFont->m_fontName, "default" ) == 0 )
    {
        g_renderer->SetFont( NULL, false, buttonFont->m_negative );        
    }
    else
    {
        g_renderer->SetFont( buttonFont->m_fontName, false, buttonFont->m_negative );
    }
    
    Colour fontColour = g_styleTable->GetPrimaryColour(FONTSTYLE_BUTTON);

    char caption[1024];
	if( m_captionIsLanguagePhrase )
	{
		snprintf( caption, sizeof(caption), LANGUAGEPHRASE(m_caption) );
	}
	else
	{
		snprintf( caption, sizeof(caption), m_caption );
	}
	caption[ sizeof(caption) - 1 ] = '\0';

    if( buttonFont->m_uppercase ) strupr(caption);

    g_renderer->TextCentre( realX + m_w/2, 
                            realY + m_h/2 - buttonFont->m_size/2, 
                            fontColour, 
                            buttonFont->m_size, 
                            caption);
    g_renderer->SetFont();    


    //
    // Drop shadow

    InterfaceWindow::RenderWindowShadow( realX + m_w, realY, m_h, m_w, 4, g_renderer->m_alpha * 0.3f );

    glColor4ubv( fontColour.GetData() );
}


// ============================================================================
// CloseButton


CloseButton::CloseButton()
:   InterfaceButton()
{
}


void CloseButton::Render(int realX, int realY, bool highlighted, bool clicked )
{
    InterfaceButton::Render(realX, realY, highlighted, clicked );
}


void CloseButton::MouseUp()
{
    EclRemoveWindow( m_parent->m_name );
}


// ============================================================================
// TextButton

TextButton::TextButton()
{
    m_fontCol = White;
    m_fontSize = 12;
}


void TextButton::Render( int realX, int realY, bool highlighted, bool clicked )
{
	if( m_captionIsLanguagePhrase )
	{
		g_renderer->Text ( realX + 5, 
						   realY + m_h/2 - m_fontSize/2, 
						   m_fontCol, 
						   m_fontSize, 
						   LANGUAGEPHRASE(m_caption) );
	}
	else
	{
		g_renderer->Text ( realX + 5, 
						   realY + m_h/2 - m_fontSize/2, 
						   m_fontCol, 
						   m_fontSize, 
						   m_caption );
	}
}


// ============================================================================
// TextInputButton


void TextInputButton::MouseUp()
{
    m_parent->BeginTextEdit( m_name );
}

void TextInputButton::Keypress ( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii )
{
    char *currentCaption = m_caption;

    if( keyCode == KEY_BACKSPACE )
    {
        if( strlen(currentCaption) > 0 )
        {
            char *newCaption = new char[ strlen(currentCaption) ];
            strncpy( newCaption, currentCaption, strlen(currentCaption) - 1 );
            newCaption[ strlen(currentCaption) - 1 ] = '\x0';
            SetCaption( newCaption, false );
            delete newCaption;
        }
    }
    else if( ascii > 0 )
    {
        char *newCaption = new char[ strlen(currentCaption) + 2 ];
        sprintf( newCaption, "%s%c", currentCaption, (char) ascii );
        SetCaption( newCaption, false );
        delete newCaption;
    }
    
}


// ============================================================================
// InvertedBox

void InvertedBox::Render( int realX, int realY, bool highlighted, bool clicked )
{    
    Colour fillCol          = g_styleTable->GetPrimaryColour(STYLE_BOX_BACKGROUND);
    Colour fillColS         = g_styleTable->GetSecondaryColour(STYLE_BOX_BACKGROUND);
    Colour borderPrimary    = g_styleTable->GetPrimaryColour(STYLE_BOX_BORDER);
    Colour borderSecondary  = g_styleTable->GetSecondaryColour(STYLE_BOX_BORDER);
    
    bool alignment = g_styleTable->GetStyle(STYLE_BOX_BACKGROUND)->m_horizontal;

    g_renderer->RectFill( realX, realY, m_w, m_h, fillCol, fillColS, alignment );
    
    g_renderer->Line    ( realX, realY, realX+m_w, realY, borderPrimary );                  // top
    g_renderer->Line    ( realX, realY, realX, realY+m_h, borderPrimary );                  // left
    g_renderer->Line    ( realX+m_w, realY, realX+m_w, realY+m_h, borderSecondary );        // right
    g_renderer->Line    ( realX, realY+m_h, realX+m_w, realY+m_h, borderSecondary );        // bottom

}


