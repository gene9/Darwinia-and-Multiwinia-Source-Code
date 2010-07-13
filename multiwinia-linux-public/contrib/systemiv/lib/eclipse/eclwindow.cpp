#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>

#include "lib/debug_utils.h"

#include "eclipse.h"
#include "eclwindow.h"

#pragma warning( disable : 4996 )

EclWindow::EclWindow( const char *_name, const char *_title, bool _titleIsLanguagePhrase )
:   m_x(0),
    m_y(0),
    m_w(0),
    m_h(0),
    m_minW(60),
    m_minH(40)
{    
    SetName ( _name );
	if( _title )
	{
		SetTitle ( _title, _titleIsLanguagePhrase );
	}
	else
	{
		SetTitle ( _name, false );
	}
    SetMovable ( true );
    strcpy( m_currentTextEdit, "None" );
}

EclWindow::~EclWindow()
{
    
    while ( m_buttons.GetData(0) )
    {
        EclButton *button = m_buttons.GetData(0);
        delete button;
        m_buttons.RemoveData(0);
    }
    m_buttons.Empty();

}

void EclWindow::SetName ( const char *_name )
{

    if ( strlen(_name) > SIZE_ECLWINDOW_NAME )
    {
        AppReleaseAssert( false, "Window name too long : %s", _name );
        return;
    }

    strcpy ( m_name, _name );

}

void EclWindow::SetTitle ( const char *_title, bool _titleIsLanguagePhrase )
{

    if ( strlen(_title) > SIZE_ECLWINDOW_TITLE )
    {
        AppReleaseAssert( false, "Window title too long : %s", _title );
        return;
    }

	m_titleIsLanguagePhrase = _titleIsLanguagePhrase;
    strcpy ( m_title, _title );

}

void EclWindow::SetPosition ( int _x, int _y )
{
    m_x = _x;
    m_y = _y;
}

void EclWindow::SetSize ( int _w, int _h )
{
    m_w = _w;
    m_h = _h;

    m_minW = _w;
    m_minH = _h;
}

void EclWindow::SetMovable ( bool _movable )
{
    m_movable = _movable;
}

void EclWindow::BeginTextEdit ( const char *_name )
{
    strcpy( m_currentTextEdit, _name );
}

void EclWindow::EndTextEdit()
{
    strcpy( m_currentTextEdit, "None" );
}

void EclWindow::RegisterButton ( EclButton *button )
{
    button->SetParent( this );
    m_buttons.PutDataAtStart ( button );
}

void EclWindow::RemoveButton ( const char *_name )
{
    for ( int i = 0; i < m_buttons.Size(); ++i )
    {
        EclButton *button = m_buttons.GetData (i);
        if ( strcmp ( button->m_name, _name ) == 0 )
        {
            m_buttons.RemoveData(i);
            delete button;
        }
    }            
}

EclButton *EclWindow::GetButton ( const char *_name )
{

    for ( int i = 0; i < m_buttons.Size(); ++i )
    {
        EclButton *button = m_buttons.GetData (i);
        if ( strcmp ( button->m_name, _name ) == 0 )
            return button;
    }
    
    return NULL;

}

EclButton *EclWindow::GetButton ( int _x, int _y )
{
    
    for ( int i = 0; i < m_buttons.Size(); ++i )
    {
        EclButton *button = m_buttons.GetData (i);
        
        if ( _x >= button->m_x && _x <= button->m_x + button->m_w &&
             _y >= button->m_y && _y <= button->m_y + button->m_h )
        {
            return button;
        }

    }
    
    return NULL;

}

void EclWindow::Create ()
{
}

void EclWindow::Remove ()
{
}

void EclWindow::Update ()
{
}

void EclWindow::Keypress ( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii )
{
    EclButton *currentTextEdit = GetButton( m_currentTextEdit );
    if( currentTextEdit )
    {
        currentTextEdit->Keypress( keyCode, shift, ctrl, alt, ascii );
    }
}

void EclWindow::MouseEvent ( bool lmb, bool rmb, bool up, bool down )
{
	// Unreferenced formal parameters
	((void)sizeof(lmb)); ((void)sizeof(rmb));
	((void)sizeof(up)); ((void)sizeof(down));
}

void EclWindow::Render ( bool hasFocus )
{
    for( int i = m_buttons.Size()-1; i >= 0; --i )
    {
        EclButton *button = m_buttons.GetData (i);
        bool highlighted = EclMouseInButton( this, button ) || strcmp(m_currentTextEdit, button->m_name) == 0;
        if( EclGetWindow() != this ) highlighted = false;
        bool clicked = ( hasFocus && strcmp ( EclGetCurrentClickedButton(), button->m_name ) == 0 );
        button->Render( m_x + button->m_x, m_y + button->m_y, highlighted, clicked );
    }
}


