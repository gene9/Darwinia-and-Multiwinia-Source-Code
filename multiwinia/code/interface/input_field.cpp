#include "lib/universal_include.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/input/input.h"
#include "lib/targetcursor.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"

#include "lib/input/keydefs.h" // Key code definitions (bit of a hack)

#include "app.h"
#include "globals.h"
#include "main.h"

#include "interface/input_field.h"


#define PIXELS_PER_CHAR	7

// ****************************************************************************
// Class InputField
// ****************************************************************************

InputField::InputField()
:	m_value( NULL ),
	m_type( TypeNowt ),
	m_ownsValue( false ),
	m_inputBoxWidth(0),
    m_callback(NULL)
{
}

InputField::~InputField()
{
	if( m_ownsValue )
	{
		delete m_value;
		m_value = false;
	}
}


void InputField::SetCallback(DarwiniaButton *button)
{
    m_callback = button;
}


void InputField::Render( int realX, int realY, bool highlighted, bool clicked )
{
    glColor4f( 0.1, 0.0, 0.0, 0.5 );
    float editAreaWidth = m_w * 0.4;
    glBegin( GL_QUADS );
        glVertex2f( realX + m_w - editAreaWidth, realY );
        glVertex2f( realX + m_w, realY );
        glVertex2f( realX + m_w, realY+m_h );
        glVertex2f( realX + m_w - editAreaWidth, realY+m_h );
    glEnd();

    glColor4f( 0.0, 0.0, 0.0, 1.0 );
    glBegin( GL_LINES );                        // top
        glVertex2f( realX + m_w - editAreaWidth, realY );
        glVertex2f( realX + m_w, realY );
    glEnd();

    glBegin( GL_LINES );                        // left
        glVertex2f( realX + m_w - editAreaWidth, realY );
        glVertex2f( realX + m_w - editAreaWidth, realY+m_h );
    glEnd();

    glColor4ub( 100, 34, 34, 150 );
    //glColor4f( 0.9, 0.3, 0.3, 0.3 );        // right
    glBegin( GL_LINES );
        glVertex2f( realX + m_w, realY );
        glVertex2f( realX + m_w, realY + m_h );
    glEnd();
    
    glBegin( GL_LINES );                        // bottom
        glVertex2f( realX + m_w - editAreaWidth, realY+m_h );
        glVertex2f( realX + m_w, realY + m_h );
    glEnd();

	int fieldX = realX + m_w - m_inputBoxWidth;

	if (m_parent->m_currentTextEdit && (strcmp(m_parent->m_currentTextEdit, m_name) == 0) )
	{
		BorderlessButton::Render(realX, realY, true, clicked);
		if (fmodf(GetHighResTime(), 1.0) < 0.5)
		{
			int cursorOffset = m_buf.WcsLen() * PIXELS_PER_CHAR + 2;
			glBegin( GL_LINES );
				glVertex2f( fieldX + cursorOffset, realY + 2 );
				glVertex2f( fieldX + cursorOffset, realY + m_h - 2 );
			glEnd();
		}
	}
	else
	{
		BorderlessButton::Render(realX, realY, highlighted, clicked);
	}


    // Edit by Chris - so we don't need to keep Refreshing, just generate the Buffer here every second

	if (!highlighted && fmodf(GetHighResTime(), 1.0) < 0.5)
	{
		if( m_value )
			m_value->ToUnicodeString( m_buf );

		m_inputBoxWidth = m_buf.Length() * PIXELS_PER_CHAR + 7;
    }
	DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;
	fieldX = realX + m_w - parent->GetMenuSize(m_inputBoxWidth);
	g_editorFont.DrawText2D( fieldX + 2, realY + 10, parent->GetMenuSize(DEF_FONT_SIZE), m_buf);
}


void InputField::MouseUp()
{
	m_parent->BeginTextEdit(m_name);
}


void InputField::Keypress ( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii )
{
	if (strcmp(m_parent->m_currentTextEdit, "None") == 0)	return;
    if( ascii == '%' ) return; 

	int len = m_buf.Length();
	if (keyCode == KEY_BACKSPACE)
	{
		if (len > 0) 
			m_buf.Truncate( len );

        if( m_type == TypeString ||
            m_type == TypeUnicodeString ) 
        {
			if( m_value )
				m_value->FromUnicodeString( m_buf );
            Refresh();
        }
	}
	else if (keyCode == KEY_ENTER || keyCode == KEY_ESC)
	{
		m_parent->EndTextEdit();

		m_value->FromUnicodeString( m_buf );
        Refresh();
	}
	else if (keyCode == KEY_STOP)
	{
		m_buf = m_buf + UnicodeString( "." );

        if( m_type == TypeString ||
            m_type == TypeUnicodeString ) 
        {
			m_value->FromUnicodeString( m_buf );
            Refresh();
        }
	}
	else if (keyCode >= KEY_0 && keyCode <= KEY_9)
	{
		char buf[2] = " ";
		buf[0] = keyCode & 0xff;

		m_buf = m_buf + UnicodeString( buf );

        if( m_type == TypeString ||
            m_type == TypeUnicodeString ) 
        {
			m_value->FromUnicodeString( m_buf );
            Refresh();
        }
	}
    else
    {
        if( m_type == TypeString && keyCode >= KEY_A && keyCode <= KEY_Z )
        {
            unsigned char ascii = keyCode & 0xff;
            if( !shift ) ascii -= ( 'A' - 'a' );

			char buf[2] = " ";
			buf[0] = ascii;

			m_buf = m_buf + buf;
        }
        else
        {
            if( !ctrl )
            {
				wchar_t buf[2] = L" ";
				buf[0] = keyCode;

				m_buf = m_buf + buf;
            }
        }

        if( m_type == TypeString ||
            m_type == TypeUnicodeString ) 
        {
		    m_value->FromUnicodeString( m_buf );
            Refresh();
        }
    }

	m_inputBoxWidth = m_buf.Length() * PIXELS_PER_CHAR + 7;
}

void InputField::RegisterNetworkValue( int _type, NetworkValue * value )
{
	AppDebugAssert(m_type == TypeNowt);

	m_type = _type;
	m_value = value;
	m_ownsValue = false;
}

void InputField::RegisterChar(unsigned char *_char, double _lowBound, double _highBound )
{
	AppDebugAssert(m_type == TypeNowt);
	BoundedNetworkValue *v = new LocalChar( *_char );
	v->SetBounds( _lowBound, _highBound );

	m_value = v;	
	m_ownsValue = true;
	m_type = TypeChar;
}


void InputField::RegisterBool(bool *_char, double _lowBound, double _highBound )
{
	AppDebugAssert(m_type == TypeNowt);
	BoundedNetworkValue *v = new LocalBool( *_char );
	v->SetBounds( _lowBound, _highBound );

	m_value = v;	
	m_ownsValue = true;
	m_type = TypeChar;
}


void InputField::RegisterInt(int *_int, double _lowBound, double _highBound)
{
	AppDebugAssert(m_type == TypeNowt);
	BoundedNetworkValue *v = new LocalInt( *_int );
	v->SetBounds( _lowBound, _highBound );

	m_value = v;	
	m_ownsValue = true;
	m_type = TypeInt;	
}


void InputField::RegisterFloat(float *_float, double _lowBound, double _highBound)
{
	AppDebugAssert(m_type == TypeNowt);
	BoundedNetworkValue *v = new LocalFloat( *_float );
	v->SetBounds( _lowBound, _highBound );

	m_value = v;	
	m_ownsValue = true;
	m_type = TypeFloat;
}

void InputField::RegisterDouble(double *_float, double _lowBound, double _highBound)
{
	AppDebugAssert(m_type == TypeNowt);
	BoundedNetworkValue *v = new LocalDouble( *_float );
	v->SetBounds( _lowBound, _highBound );

	m_value = v;	
	m_ownsValue = true;
	m_type = TypeDouble;
}

void InputField::RegisterString(char *_string, int _bufSize)
{
	AppDebugAssert(m_type == TypeNowt);
	m_value = new LocalString( _string, _bufSize );
	m_ownsValue = true;
	m_type = TypeString;
}

void InputField::RegisterUnicodeString( UnicodeString &_string )
{
	AppDebugAssert(m_type == TypeNowt);
	m_value = new LocalUnicodeString( _string );
	m_ownsValue = true;
	m_type = TypeUnicodeString;
}

void InputField::Refresh()
{
	m_value->ToUnicodeString( m_buf );

    if( m_callback )
    {
        m_callback->MouseUp();
    }
}


// ****************************************************************************
// Class InputScroller
// ****************************************************************************

InputScroller::InputScroller()
:   DarwiniaButton(),
    m_inputField(NULL),
    m_change(0.0),
    m_mouseDownStartTime(-1.0)
{
}


#define INTEGER_INCREMENT_PERIOD 0.1

void InputScroller::Render( int realX, int realY, bool highlighted, bool clicked )
{
    DarwiniaButton::Render( realX, realY, highlighted, clicked );

	CheckInput( realX, realY );
}

void InputScroller::CheckInput( int realX, int realY )
{
    if( m_mouseDownStartTime > 0.0 &&
		m_inputField &&
		g_inputManager->controlEvent( ControlEclipseLMousePressed ) &&
		g_target->X() >= realX &&
        g_target->X() < realX + m_w &&
        g_target->Y() >= realY &&
        g_target->Y() < realY + m_h )
    {
		float change = m_change;
        if( g_inputManager->controlEvent( ControlScrollSpeedup ) ) change *= 5.0;

		double timeDelta = GetHighResTime() - m_mouseDownStartTime;
		if (m_inputField->m_type == InputField::TypeChar)
		{
			if (timeDelta > INTEGER_INCREMENT_PERIOD)
			{
				NetworkChar *v = (NetworkChar *) m_inputField->m_value;
				v->Set( v->Get() + change );
				m_mouseDownStartTime += INTEGER_INCREMENT_PERIOD;
			}
		}
		else if (m_inputField->m_type == InputField::TypeInt)
		{
			if (timeDelta > INTEGER_INCREMENT_PERIOD)
			{
				NetworkInt *v = (NetworkInt *) m_inputField->m_value;
                v->Set( v->Bound( v->Get() + change ) );
				m_mouseDownStartTime += INTEGER_INCREMENT_PERIOD;
			}
		}
		else if (m_inputField->m_type == InputField::TypeBool)
		{
			if (timeDelta > INTEGER_INCREMENT_PERIOD)
			{
				NetworkBool *v = (NetworkBool *) m_inputField->m_value;
                v->Set( v->Bound( v->Get() + change ) );
				m_mouseDownStartTime += INTEGER_INCREMENT_PERIOD;
			}
		}
		else if (m_inputField->m_type == InputField::TypeFloat)
		{
			change = change * timeDelta * 0.2;
			NetworkFloat *v = (NetworkFloat *) m_inputField->m_value;
			v->Set( v->Bound(v->Get() + change ));
		}
		else if (m_inputField->m_type == InputField::TypeDouble)
		{
			change = change * timeDelta * 0.2;
			NetworkDouble *v = (NetworkDouble *) m_inputField->m_value;
			v->Set( v->Bound(v->Get() + change ));
		}

        m_inputField->Refresh();
    }
}


void InputScroller::MouseDown()
{
	if ( g_inputManager->controlEvent( ControlEclipseLMouseDown ) )
	{
	    m_mouseDownStartTime = GetHighResTime() - INTEGER_INCREMENT_PERIOD;
	}
}


void InputScroller::MouseUp()
{
    m_mouseDownStartTime = -1.0;
}


// ****************************************************************************
// Class ColourWidget
// ****************************************************************************


ColourWidget::ColourWidget()
:   DarwiniaButton(),
    m_callback(NULL),
    m_value(NULL)
{
}


void ColourWidget::Render( int realX, int realY, bool highlighted, bool clicked )
{
    DarwiniaButton::Render( realX, realY, highlighted, clicked );

    glColor4ubv( (unsigned char *) m_value );
    glBegin( GL_QUADS );
        glVertex2i( realX + m_w - 30, realY + 4 );
        glVertex2i( realX + m_w - 5, realY + 4 );
        glVertex2i( realX + m_w - 5, realY + m_h - 3 );
        glVertex2i( realX + m_w - 30, realY + m_h - 3 );
    glEnd();
}


void ColourWidget::MouseUp()
{
    ColourWindow *cw = new ColourWindow( "editor_coloureditor" );
    cw->SetSize( 200, 100 );
    cw->SetValue( m_value );
    cw->SetCallback( m_callback );
    EclRegisterWindow( cw, m_parent );
}


void ColourWidget::SetValue(int *value)
{
    m_value = value;
}


void ColourWidget::SetCallback(DarwiniaButton *button)
{
    m_callback = button;
}



// ****************************************************************************
// Class ColourButton
// ****************************************************************************

ColourWindow::ColourWindow( char *_name )
:   DarwiniaWindow( _name ),
    m_callback(NULL),
    m_value(NULL)
{
}


void ColourWindow::SetValue(int *value)
{
    m_value = value;
}


void ColourWindow::SetCallback(DarwiniaButton *button)
{
    m_callback = button;
}


void ColourWindow::Create()
{
    DarwiniaWindow::Create();
    
    int y = 25;
    int h = 18;

    unsigned char *r = ((unsigned char *) m_value)+0;
    unsigned char *g = ((unsigned char *) m_value)+1;
    unsigned char *b = ((unsigned char *) m_value)+2;
    unsigned char *a = ((unsigned char *) m_value)+3;

    CreateValueControl( "editor_red", r, y,    1, 0, 255, m_callback, -1, m_w - 80 );
    CreateValueControl( "editor_green", g, y+=h, 1, 0, 255, m_callback, -1, m_w - 80 );
    CreateValueControl( "editor_blue", b, y+=h, 1, 0, 255, m_callback, -1, m_w - 80 );
    CreateValueControl( "editor_alpha", a, y+=h, 1, 0, 255, m_callback, -1, m_w - 80 );
}


void ColourWindow::Render( bool hasFocus )
{
    DarwiniaWindow::Render( hasFocus );

    glColor4ubv( (unsigned char *) m_value );
    glBegin( GL_QUADS );
        glVertex2i( m_x + m_w - 60, m_y + 25 );
        glVertex2i( m_x + m_w - 10, m_y + 25 );
        glVertex2i( m_x + m_w - 10, m_y + m_h - 10 );
        glVertex2i( m_x + m_w - 60, m_y + m_h - 10 );
    glEnd();
}
