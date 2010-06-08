#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>

#include "lib/text_renderer.h"
#include "lib/resource.h"
#include "lib/input/input.h"

#include "interface/darwinia_window.h"
#include "interface/input_field.h"

#include "app.h"
#include "renderer.h"
#include "globals.h"
#include "lib/input/control_bindings.h"

#include "lib/targetcursor.h"



// ****************************************************************************
// Class DarwiniaButton
// ****************************************************************************


DarwiniaButton::DarwiniaButton()
:   EclButton(),
    m_fontSize(11.0f),
    m_centered(false),
	m_disabled(false),
	m_highlightedThisFrame(false),
	m_mouseHighlightMode(false)
{
}

void DarwiniaButton::SetShortProperties(char const *_name, int _x, int _y, 
										 int _w, int _h, char *_caption, char *_tooltip)
{
	if ( _w == -1 )
	{
		_w = strlen(_name) * 7 + 9;
	}

	if ( _h == -1 )
	{
		_h = 15;
	}

	SetProperties((char *)_name, _x, _y, _w, _h, _caption, _tooltip);    
}

void DarwiniaButton::SetDisabled( bool _disabled )
{
	m_disabled = _disabled;
}



void DarwiniaButton::Render( int realX, int realY, bool highlighted, bool clicked )
{
//    if      ( clicked )         glColor4f( 0.9f, 0.9f, 1.0f, 0.6f );    
//    else if ( highlighted )     glColor4f( 0.9f, 0.9f, 0.9f, 0.3f );    
//    else                        glColor4f( 0.5f, 0.5f, 0.5f, 0.2f );
    
	float y = 7.5 + realY + (m_h - m_fontSize) / 2;

	DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;

    UpdateButtonHighlight();

    if( !m_mouseHighlightMode )
	{
		highlighted = false;
	}

	if( parent->m_buttonOrder[parent->m_currentButton] == this )
	{
		highlighted = true;
	}

    if( highlighted || clicked )
    {
        glShadeModel( GL_SMOOTH );
        glBegin( GL_QUADS );
            glColor4ub( 199, 214, 220, 255 );
            if( clicked ) glColor4ub( 255, 255, 255, 255 );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glColor4ub( 112, 141, 168, 255 );
            if( clicked ) glColor4ub( 162, 191, 208, 255 );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();    

        glShadeModel( GL_FLAT );

        g_editorFont.SetRenderShadow(true);
		
		if (m_disabled) 
			glColor4ub( 128, 128, 75, 30 );
		else
			glColor4ub( 255, 255, 150, 30 );
			
        if( m_centered )
        {
            g_editorFont.DrawText2DCentre( realX + m_w/2, y, m_fontSize, m_caption );
            g_editorFont.DrawText2DCentre( realX + m_w/2, y, m_fontSize, m_caption );
        }
        else
        {
            g_editorFont.DrawText2D( realX + 5, y, m_fontSize, m_caption );
            g_editorFont.DrawText2D( realX + 5, y, m_fontSize, m_caption );
        }
        g_editorFont.SetRenderShadow(false);
    }
    else
    {
        glColor4ub( 107, 37, 39, 64 );    
        //glColor4ub( 82, 56, 102, 64 );
        glBegin( GL_QUADS );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();    

        glLineWidth(1.0f);
        glBegin( GL_LINES );
            glColor4ub( 100, 34, 34, 200 );     
            glVertex2f( realX, realY + m_h );
            glVertex2f( realX, realY );

            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );

            glColor4f( 0.1f, 0.0f, 0.0f, 1.0f );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );        

            glVertex2f( realX + m_w, realY + m_h );        
            glVertex2f( realX, realY + m_h );
        glEnd();

		if (m_disabled) 
			glColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
		else
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
			
        if( m_centered )
        {
            g_editorFont.DrawText2DCentre( realX + m_w/2, y, m_fontSize, m_caption );
        }
        else
        {
            g_editorFont.DrawText2D( realX + 5, y, m_fontSize, m_caption );
        }
    }

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
   
    EclButton::Render( realX, realY, highlighted, clicked );
}

void DarwiniaButton::UpdateButtonHighlight()
{
    DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;

	if( parent->m_buttonChangedThisUpdate )
	{
		m_mouseHighlightMode = false;
	}


	if( EclMouseInButton( m_parent, this ) )
	{
		if( !m_highlightedThisFrame )
		{
			parent->SetCurrentButton( this );
			m_highlightedThisFrame = true;
			m_mouseHighlightMode = true;
		}
	}
	else
	{
		m_highlightedThisFrame = false;
	}
}

// ============================================================================


BorderlessButton::BorderlessButton()
:   DarwiniaButton()
{
}


void BorderlessButton::SetShortProperties(char const *_name, int _x, int _y, 
										 int _w, int _h, char *_caption, char *_tooltip)
{
	if ( _w == -1 )
	{
		_w = strlen(_name) * 7 + 9;
	}

	if ( _h == -1 )
	{
		_h = 15;
	}

	SetProperties((char *)_name, _x, _y, _w, _h, _caption, _tooltip);    
}


void BorderlessButton::Render( int realX, int realY, bool highlighted, bool clicked )
{
	DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;
	if( parent->m_buttonOrder[parent->m_currentButton] == this )
	{
		clicked = true;
	}
    if( clicked )
    {
        glShadeModel( GL_SMOOTH );
        glBegin( GL_QUADS );
            glColor4ub( 199, 214, 220, 255 );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glColor4ub( 112, 141, 168, 255 );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();    
        glShadeModel( GL_FLAT );

        g_editorFont.SetRenderShadow(true);
        glColor4ub( 255, 255, 150, 30 );
        if( m_centered )
        {
            g_editorFont.DrawText2DCentre( realX + m_w/2, realY + 10, parent->GetMenuSize(m_fontSize), m_caption );
            g_editorFont.DrawText2DCentre( realX + m_w/2, realY + 10, parent->GetMenuSize(m_fontSize), m_caption );
        }
        else
        {
            g_editorFont.DrawText2D( realX + 5, realY + 10, parent->GetMenuSize(m_fontSize), m_caption );
            g_editorFont.DrawText2D( realX + 5, realY + 10, parent->GetMenuSize(m_fontSize), m_caption );
        }
        g_editorFont.SetRenderShadow(false);
    }
    else
    {
        glColor4ub( 107, 37, 39, 64 );
        
        if( highlighted )
        {
			parent->SetCurrentButton( this );
            glLineWidth(1.0f);
            glBegin( GL_LINES );
                glColor4ub( 100, 34, 34, 250 );           
                glVertex2f( realX, realY + m_h );
                glVertex2f( realX, realY );

                glVertex2f( realX, realY );
                glVertex2f( realX + m_w, realY );

                glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );    
                glVertex2f( realX + m_w, realY );
                glVertex2f( realX + m_w, realY + m_h );        

                glVertex2f( realX + m_w, realY + m_h );        
                glVertex2f( realX, realY + m_h );
            glEnd();
        }    

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        if( m_centered )
        {
            g_editorFont.DrawText2DCentre( realX + m_w/2, realY + 10, parent->GetMenuSize(m_fontSize), m_caption );    
        }
        else
        {
            g_editorFont.DrawText2D( realX + 5, realY + 10, parent->GetMenuSize(m_fontSize), m_caption );
        }

        if( highlighted )
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
            if( m_centered )
            {
                g_editorFont.DrawText2DCentre( realX + m_w/2, realY + 10, parent->GetMenuSize(m_fontSize), m_caption );
            }
            else
            {
                g_editorFont.DrawText2D( realX + 5, realY + 10, parent->GetMenuSize(m_fontSize), m_caption );
            }
        }
    }
    
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    EclButton::Render( realX, realY, highlighted, clicked );    
}



// ****************************************************************************
// Class DarwiniaWindow
// ****************************************************************************

DarwiniaWindow::DarwiniaWindow( char const *name )
:   EclWindow((char *)name),
	m_currentButton(0),
	m_buttonChangedThisUpdate(false),
    m_skipUpdate(false)
{
    SetTitle((char *)name);
    strupr( m_title );

	EclSetCurrentFocus( m_name );
}

DarwiniaWindow::~DarwiniaWindow()
{
	LList<EclWindow *> *windows = EclGetWindows();
	if( windows->GetData(0) )
	{
		EclSetCurrentFocus( windows->GetData(0)->m_name );
	}
}


void DarwiniaWindow::CreateValueControl( char const *name, int dataType, void *value, int y, 
										  float change, float _lowBound, float _highBound,
                                          DarwiniaButton *callback, int x, int w )
{
    if( x == -1 ) x = 10;
    if( w == -1 ) w = m_w - x * 2;

    InputField *input = new InputField();
    
    if( dataType == InputField::TypeString )
    {
        input->SetShortProperties( (char *)name, x, y, w, GetMenuSize(15) );
    }
    else
    {
        input->SetShortProperties( (char *)name, x, y, w-37, GetMenuSize(15) );
    }

	input->m_lowBound = _lowBound;
	input->m_highBound = _highBound;
    input->SetCallback( callback );
    switch( dataType )
    {
        case InputField::TypeChar:      input->RegisterChar( (unsigned char *) value ); break;
        case InputField::TypeInt:       input->RegisterInt( (int *) value );            break;
        case InputField::TypeFloat:     input->RegisterFloat( (float *) value );        break;
        case InputField::TypeString:    input->RegisterString( (char *) value );        break;
    }
    RegisterButton( input );

    if( dataType != InputField::TypeString )
    {
        char nameLeft[64];
        sprintf( nameLeft, "%s left", name );
        InputScroller *left = new InputScroller();
        left->SetProperties( nameLeft, input->m_x + input->m_w + 5, y, 15, 15, "<", "Value left" );
        left->m_inputField = input;
        left->m_change = -change;
        RegisterButton( left );

        char nameRight[64];
        sprintf( nameRight, "%s right", name );
        InputScroller *right = new InputScroller();
        right->SetProperties( nameRight, input->m_x + input->m_w + 22, y, 15, 15, ">", "Value right" );
        right->m_inputField = input;
        right->m_change = change;
        RegisterButton( right );
    }
}

void DarwiniaWindow::RemoveValueControl( char *name )
{
    RemoveButton( name );

    char nameLeft[64];
    sprintf( nameLeft, "%s left", name );
    RemoveButton( nameLeft );

    char nameRight[64];
    sprintf( nameRight, "%s right", name );
    RemoveButton( nameRight );
}

void DarwiniaWindow::CreateColourControl( char const *name, int *value, int y, DarwiniaButton *callback, int x, int w )
{
    if( x == -1 ) x = 10;
    if( w == -1 ) w = m_w - x;

    ColourWidget *cw = new ColourWidget();
    cw->SetShortProperties( (char *)name, x, y, w-13 );
    cw->SetCallback( callback );
    cw->SetValue( value );
    RegisterButton( cw );
}

void DarwiniaWindow::Create()
{	
	CloseButton *close = new CloseButton();
    close->SetProperties( "Close", m_w - 12, 2, 10, 10, " ", "Close this window" );
    close->m_iconised = true;
    RegisterButton( close );
}


void DarwiniaWindow::Remove()
{
    while( m_buttons.Size() > 0 )
    {
        EclButton *button = m_buttons.GetData(0);
        RemoveButton( button->m_name );
    }
    m_buttonOrder.Empty();
    m_currentButton = 0;
}

// Get the coordinates of the drawable area on the rectangle
int DarwiniaWindow::GetClientRectX1()
{
	return 2;
}

int DarwiniaWindow::GetClientRectX2()
{
	return m_w - 2;
}

int DarwiniaWindow::GetClientRectY1()
{
	return GetMenuSize(15) + 1;
}

int DarwiniaWindow::GetClientRectY2()
{
	return m_h - 2;
}

void DarwiniaWindow::Render ( bool hasFocus )
{
    //
    // Main body fill

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_red.bmp" ) );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    float texH = 1.0f;
    float texW = texH * 512.0f / 64.0f;

    //glColor4f( 0.3f, 0.3f, 0.4f, 0.95f );
    glColor4f( 1.0f, 1.0f, 1.0f, 0.96f );
    glBegin( GL_QUADS );
        glTexCoord2f( 0.0f, 0.0f );     glVertex2f( m_x, m_y );
        glTexCoord2f( texW, 0.0f );     glVertex2f( m_x + m_w, m_y );
        glTexCoord2f( texW, texH );     glVertex2f( m_x + m_w, m_y + m_h );
        glTexCoord2f( 0.0f, texH );     glVertex2f( m_x, m_y + m_h );
    glEnd();

    glDisable       ( GL_TEXTURE_2D );

    //
    // Title bar fill

    float titleBarHeight = GetClientRectY1() - 1;
    glShadeModel( GL_SMOOTH );
    glBegin( GL_QUADS );
        glColor4ub( 199, 214, 220, 255 );
        glVertex2f( m_x, m_y );
        glVertex2f( m_x+m_w, m_y );
        glColor4ub( 112, 141, 168, 255 );
        glVertex2f( m_x+m_w, m_y+titleBarHeight );
        glVertex2f( m_x, m_y+titleBarHeight );
    glEnd();
    glShadeModel( GL_FLAT );
    
    //glColor4ub( 112, 141, 168, 120 );
    //glBegin( GL_LINES );
    //    glVertex2f( m_x, m_y+1 );
    //    glVertex2f( m_x+m_w, m_y+1 );
    //glEnd();

    //
    // Border lines

    glLineWidth(2.0f);
    //glColor4ub( 100, 34, 34, 255 );
    glColor4ub( 199, 214, 220, 255 );
    glBegin( GL_LINES );                        // top
        glVertex2f( m_x, m_y );
        glVertex2f( m_x + m_w, m_y );
    glEnd();

    glBegin( GL_LINES );                        // left
        glVertex2f( m_x, m_y );
        glVertex2f( m_x, m_y+m_h );
    glEnd();

    //glColor4f( 0.0f, 0.0f, 0.1f, 1.0f );
    glBegin( GL_LINES );
        glVertex2f( m_x + m_w, m_y );           // right
        glVertex2f( m_x + m_w, m_y + m_h );
    glEnd();

    glBegin( GL_LINES );                        // bottom
        glVertex2f( m_x, m_y+m_h );
        glVertex2f( m_x + m_w, m_y + m_h );
    glEnd();

    glLineWidth(1.0f);
    glColor4ub( 42, 56, 82, 255 );
    glBegin( GL_LINE_LOOP );
        glVertex2f( m_x-2, m_y-2 );
        glVertex2f( m_x + m_w+1, m_y-2 );
        glVertex2f( m_x + m_w+1, m_y + m_h+1 );
        glVertex2f( m_x-2, m_y+m_h+1 );
    glEnd();

    //glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );    
    g_gameFont.SetRenderShadow(true);
    glColor4ub( 255, 255, 150, 30 );
	int y = m_y+9;
	int fontSize = GetMenuSize(12);
	if( g_app->m_largeMenus )
	{
		y = m_y + fontSize/2;
	}
    g_gameFont.DrawText2DCentre( m_x + m_w/2, y, fontSize, m_title );
    g_gameFont.DrawText2DCentre( m_x + m_w/2, y, fontSize, m_title );
    g_gameFont.SetRenderShadow(false);

    
    EclWindow::Render( hasFocus );
}

int DarwiniaWindow::GetMenuSize( int _value )
{
	if( g_app->m_largeMenus )
	{
		//int h = m_originalH;
		//float scale = float(m_h)/float(h);
		int screenH = g_app->m_renderer->ScreenH();
		float scale = 0.96f * (float(screenH)/460.0f);

		return _value * scale;
	}
	else 
	{
		return _value;
	}
}

void DarwiniaWindow::SetMenuSize(int _w, int _h)
{
	if( g_app->m_largeMenus )
	{
		int screenH = g_app->m_renderer->ScreenH();

		float ratio = 0.96f * (float(screenH)/460.0f);

		_h *= ratio;
		_w *= ratio;
	}
	
	SetSize( _w, _h );
}

void DarwiniaWindow::Update()
{
	m_buttonChangedThisUpdate = false;
    if( m_skipUpdate )
    {
        m_skipUpdate = false;
        return;
    }

	if( strcmp(EclGetCurrentFocus(), m_name) == 0 )
	{
		if( g_inputManager->controlEvent(ControlMenuDown) )
		{
			m_buttonChangedThisUpdate = true;
			m_currentButton++;
			m_currentButton = min( m_currentButton, m_buttonOrder.Size()-1 );
		}
		if( g_inputManager->controlEvent(ControlMenuUp) )
		{
			m_buttonChangedThisUpdate = true;
			m_currentButton--;
			m_currentButton = max(0, m_currentButton);
		}

		if( g_inputManager->controlEvent(ControlMenuActivate) )
		{
			EclButton *b = m_buttonOrder.GetData(m_currentButton);
			if( b )
			{
				b->MouseUp();
			}
		}

        if( g_inputManager->controlEvent( ControlMenuClose ) &&
            !g_app->m_atMainMenu )
        {
            EclRemoveWindow( m_name );
        }
	}

}

void DarwiniaWindow::SetCurrentButton( EclButton *button )
{
	for( int i = 0; i < m_buttonOrder.Size(); ++i )
	{
		if( m_buttonOrder[i] == button )
		{
			m_currentButton = i;
			return;
		}
	}
}

// ****************************************************************************
// Class GameExitButton
// ****************************************************************************

void GameExitButton::MouseUp()
{
	g_app->m_requestQuit = true;
    //g_app->m_atMainMenu = true;
    //g_app->m_renderer->StartFadeOut();
}


// ****************************************************************************
// Class CloseButton
// ****************************************************************************

CloseButton::CloseButton()
:   DarwiniaButton(),
    m_iconised(false)
{
}

void CloseButton::MouseUp()
{
    EclRemoveWindow( m_parent->m_name );
}


void CloseButton::Render( int realX, int realY, bool highlighted, bool clicked )
{
    if( m_iconised )
    {
        if( highlighted || clicked )    glColor4ub( 160, 137, 139, 64 );    
        else                            glColor4ub( 60, 37, 39, 64 );    

        glBegin( GL_QUADS );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();    

        glLineWidth(1.0f);
        glBegin( GL_LINES );
            glColor4ub( 0, 0, 150, 100 );    
            glVertex2f( realX, realY + m_h );
            glVertex2f( realX, realY );

            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );

            glColor4f( 0.1f, 0.0f, 0.0f, 1.0f );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );        

            glVertex2f( realX + m_w, realY + m_h );        
            glVertex2f( realX, realY + m_h );
        glEnd();
    }
    else
    {
        DarwiniaButton::Render( realX, realY, highlighted, clicked );
    }
}


// ****************************************************************************
// Class InvertexBox
// ****************************************************************************

void InvertedBox::Render( int realX, int realY, bool highlighted, bool clicked )
{
    //DarwiniaButton::Render( realX, realY, highlighted, clicked );

    glColor4f( 0.05f, 0.0f, 0.0f, 0.4f );
    glBegin( GL_QUADS );
        glVertex2f( realX, realY );
        glVertex2f( realX + m_w, realY );
        glVertex2f( realX + m_w, realY + m_h );
        glVertex2f( realX, realY + m_h );
    glEnd();

    //
    // Border lines

    glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
    glBegin( GL_LINES );                        // top
        glVertex2f( realX, realY );
        glVertex2f( realX + m_w, realY );
    glEnd();

    glBegin( GL_LINES );                        // left
        glVertex2f( realX, realY );
        glVertex2f( realX, realY+m_h );
    glEnd();

    glColor4ub( 100, 34, 34, 150 );;        // right
    glBegin( GL_LINES );
        glVertex2f( realX + m_w, realY );
        glVertex2f( realX + m_w, realY + m_h );
    glEnd();
    
    glBegin( GL_LINES );                        // bottom
        glVertex2f( realX, realY+m_h );
        glVertex2f( realX + m_w, realY + m_h );
    glEnd();
}



void LabelButton::Render( int realX, int realY, bool highlighted, bool clicked )
{
	if (m_disabled)
		glColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
	else
		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_editorFont.DrawText2D( realX + 5, realY + 10, 11.0f, m_caption );    
}
