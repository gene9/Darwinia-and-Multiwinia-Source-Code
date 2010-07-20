//#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>

#include "eclipse.h"
#include "eclwindow.h"


EclWindow::EclWindow( char *_name )
:   m_x(0),
    m_y(0),
    m_w(0),
    m_h(0),
    m_dirty(false),
    m_resizable(true)
{    
    SetName (_name);
    SetTitle ( "New Window" );
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

void EclWindow::SetName ( char *_name )
{

    if ( strlen(_name) > SIZE_ECLWINDOW_NAME )
    {
        return;
    }

    strcpy ( m_name, _name );

}

void EclWindow::SetTitle ( char *_title )
{
    if ( strlen(_title) > SIZE_ECLWINDOW_TITLE )
    {
        return;
    }

    strcpy ( m_title, _title );
    EclDirtyWindow( this );

}

void EclWindow::SetPosition ( int _x, int _y )
{
    m_x = _x;
    m_y = _y;
    EclDirtyWindow( this );
}

void EclWindow::SetSize ( int _w, int _h )
{
    m_w = _w;
    m_h = _h;
    EclDirtyWindow( this );
}

void EclWindow::SetMovable ( bool _movable )
{
    m_movable = _movable;
}

void EclWindow::MakeAllOnScreen()
{
	int screenW = EclGetScreenW();
	int screenH = EclGetScreenH();
    if( m_x < 10 ) m_x = 10;
    if( m_y < 10 ) m_y = 10;
    if( m_x + m_w > screenW - 10 ) m_x = screenW - m_w - 10;
    if( m_y + m_h > screenH - 10 ) m_y = screenH - m_h - 10;
}

void EclWindow::BeginTextEdit ( char *_name )
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
	
	if (button->m_y + button->m_h + 10 > m_h)
	{
		SetSize(m_w, button->m_y + button->m_h + 10);
		MakeAllOnScreen();
	}
    
	EclDirtyWindow ( this );
}

void EclWindow::RemoveButton ( char *_name )
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

EclButton *EclWindow::GetButton ( char *_name )
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

void EclWindow::Keypress ( int keyCode, bool shift, bool ctrl, bool alt )
{
    EclButton *currentTextEdit = GetButton( m_currentTextEdit );
    if( currentTextEdit )
    {
        currentTextEdit->Keypress( keyCode, shift, ctrl, alt );
    }
}

void EclWindow::MouseEvent ( bool lmb, bool rmb, bool up, bool down )
{
}

void EclWindow::Render ( bool hasFocus )
{
    for ( int i = m_buttons.Size()-1; i >= 0; --i )
    {
        EclButton *button = m_buttons.GetData (i);
        bool highlighted = EclMouseInButton( this, button ) || strcmp(m_currentTextEdit, button->m_name) == 0;
        bool clicked = ( hasFocus && strcmp ( EclGetCurrentClickedButton(), button->m_name ) == 0 );
        button->Render( m_x + button->m_x, m_y + button->m_y, highlighted, clicked );
    }
}

