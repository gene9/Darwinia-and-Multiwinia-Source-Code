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
:	m_type(TypeNowt),
	m_string(NULL),
	m_int(NULL),
	m_float(NULL),
    m_char(NULL),
	m_inputBoxWidth(0),
    m_callback(NULL),
	m_lowBound(0.0f),
	m_highBound(1e4)
{
	m_buf[0] = '\0';
}


void InputField::SetCallback(DarwiniaButton *button)
{
    m_callback = button;
}


void InputField::Render( int realX, int realY, bool highlighted, bool clicked )
{
    glColor4f( 0.1f, 0.0f, 0.0f, 0.5f );
    float editAreaWidth = m_w * 0.4f;
    glBegin( GL_QUADS );
        glVertex2f( realX + m_w - editAreaWidth, realY );
        glVertex2f( realX + m_w, realY );
        glVertex2f( realX + m_w, realY+m_h );
        glVertex2f( realX + m_w - editAreaWidth, realY+m_h );
    glEnd();

    glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
    glBegin( GL_LINES );                        // top
        glVertex2f( realX + m_w - editAreaWidth, realY );
        glVertex2f( realX + m_w, realY );
    glEnd();

    glBegin( GL_LINES );                        // left
        glVertex2f( realX + m_w - editAreaWidth, realY );
        glVertex2f( realX + m_w - editAreaWidth, realY+m_h );
    glEnd();

    glColor4ub( 100, 34, 34, 150 );
    //glColor4f( 0.9f, 0.3f, 0.3f, 0.3f );        // right
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
		if (fmodf(GetHighResTime(), 1.0f) < 0.5f)
		{
			int cursorOffset = strlen(m_buf) * PIXELS_PER_CHAR + 2;
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

	if (!highlighted && fmodf(GetHighResTime(), 1.0f) < 0.5f)
	{
	    switch(m_type)
	    {
	        case TypeChar:	    sprintf(m_buf, "%d", (int) *m_char);		    break;
	        case TypeInt:	    sprintf(m_buf, "%d", *m_int);		            break;
	        case TypeFloat:     sprintf(m_buf, "%.2f", *m_float);               break;
	        case TypeString:    strncpy(m_buf, m_string, sizeof(m_buf) - 1);    break;
	    }
		m_inputBoxWidth = strlen(m_buf) * PIXELS_PER_CHAR + 7;
    }
	DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;
	fieldX = realX + m_w - parent->GetMenuSize(m_inputBoxWidth);
	g_editorFont.DrawText2D( fieldX + 2, realY + 10, parent->GetMenuSize(DEF_FONT_SIZE), m_buf);
}


void InputField::MouseUp()
{
	m_parent->BeginTextEdit(m_name);
}


void InputField::Keypress ( int keyCode, bool shift, bool ctrl, bool alt )
{
	if (strcmp(m_parent->m_currentTextEdit, "None") == 0)	return;

	int len = strlen(m_buf);
	if (keyCode == KEY_BACKSPACE)
	{
		if (len > 0) m_buf[len - 1] = '\0';
        if( m_type == TypeString ) 
        {
			strcpy(m_string, m_buf);
            Refresh();
        }
	}
	else if (keyCode == KEY_ENTER)
	{
		m_parent->EndTextEdit();

		if (m_float)
		{
			*m_float = atof(m_buf);
		}
		else if (m_int)
		{
			*m_int = atoi(m_buf);
		}
		else if (m_string)	
		{
			strcpy(m_string, m_buf);
		}
        else if(m_char)
        {
            *m_char = (unsigned char) atoi(m_buf);
        }

        ClampToBounds();
        Refresh();
	}
	else if (keyCode == KEY_STOP)
	{
		if (len < sizeof(m_buf) - 1)
		{
			strcat(m_buf, ".");
		}

        if( m_type == TypeString ) 
        {
			strcpy(m_string, m_buf);
            Refresh();
        }
	}
	else if (keyCode >= KEY_0 && keyCode <= KEY_9)
	{
		if (len < sizeof(m_buf) - 1)
		{
			char buf[2] = " ";
			buf[0] = keyCode & 0xff;
			strcat(m_buf, buf);
		}

        if( m_type == TypeString ) 
        {
			strcpy(m_string, m_buf);
            Refresh();
        }
	}
    else
    {
        if( m_type == TypeString && keyCode >= KEY_A && keyCode <= KEY_Z )
        {
            unsigned char ascii = keyCode & 0xff;
            if( !shift ) ascii -= ( 'A' - 'a' );
            int location = strlen(m_buf);
            m_buf[ location ] = ascii;
            m_buf[ location+1 ] = '\x0';         
        }

        if( m_type == TypeString ) 
        {
			strcpy(m_string, m_buf);
            Refresh();
        }
    }

	m_inputBoxWidth = strlen(m_buf) * PIXELS_PER_CHAR + 7;
}


void InputField::RegisterChar(unsigned char *_char)
{
	DarwiniaDebugAssert(m_type == TypeNowt);
	m_type = TypeChar;
	m_char = _char;
}


void InputField::RegisterInt(int *_int)
{
	DarwiniaDebugAssert(m_type == TypeNowt);
	m_type = TypeInt;
	m_int = _int;
}


void InputField::RegisterFloat(float *_float)
{
	DarwiniaDebugAssert(m_type == TypeNowt);
	m_type = TypeFloat;
	m_float = _float;
}


void InputField::RegisterString(char *_string)
{
	DarwiniaDebugAssert(m_type == TypeNowt);
	m_type = TypeString;
	m_string = _string;
}


void InputField::ClampToBounds()
{
	switch(m_type)
	{
		case TypeChar:
			if (*m_char > m_highBound)			*m_char = m_highBound;
			else if (*m_char < m_lowBound)		*m_char = m_lowBound;
			break;
		case TypeInt:
			if (*m_int > m_highBound)			*m_int = m_highBound;
			else if (*m_int < m_lowBound)		*m_int = m_lowBound;
			break;
		case TypeFloat:
			if (*m_float > m_highBound)			*m_float = m_highBound;
			else if (*m_float < m_lowBound)		*m_float = m_lowBound;
			break;
	}
}


void InputField::Refresh()
{
	switch(m_type)
	{
		case TypeChar:
			sprintf(m_buf, "%d", (int) *m_char);
			break;
		case TypeInt:
			sprintf(m_buf, "%d", *m_int);
			break;
		case TypeFloat:
			sprintf(m_buf, "%.2f", *m_float);
			break;
		case TypeString:
			strncpy(m_buf, m_string, sizeof(m_buf) - 1);
			break;
	}

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
    m_change(0.0f),
    m_mouseDownStartTime(-1.0f)
{
}


#define INTEGER_INCREMENT_PERIOD 0.1f

void InputScroller::Render( int realX, int realY, bool highlighted, bool clicked )
{
    DarwiniaButton::Render( realX, realY, highlighted, clicked );

    if( m_mouseDownStartTime > 0.0f &&
		m_inputField &&
		g_inputManager->controlEvent( ControlEclipseLMousePressed ) &&
		g_target->X() >= realX &&
        g_target->X() < realX + m_w &&
        g_target->Y() >= realY &&
        g_target->Y() < realY + m_h )
    {
		float change = m_change;
        if( g_inputManager->controlEvent( ControlScrollSpeedup ) ) change *= 5.0f;

		float timeDelta = GetHighResTime() - m_mouseDownStartTime;
		if (m_inputField->m_type == InputField::TypeChar)
		{
			if (timeDelta > INTEGER_INCREMENT_PERIOD)
			{
				*m_inputField->m_char += change;
				m_mouseDownStartTime += INTEGER_INCREMENT_PERIOD;
			}
		}
		else if (m_inputField->m_type == InputField::TypeInt)
		{
			if (timeDelta > INTEGER_INCREMENT_PERIOD)
			{
				*m_inputField->m_int += change;
				m_mouseDownStartTime += INTEGER_INCREMENT_PERIOD;
			}
		}
		else if (m_inputField->m_type == InputField::TypeFloat)
		{
			change = change * timeDelta * 0.2f;
            *m_inputField->m_float += change;
		}

		m_inputField->ClampToBounds();
        m_inputField->Refresh();
    }
}


void InputScroller::MouseDown()
{
	if ( g_inputManager->controlEvent( ControlEclipseLMousePressed ) )
	{
	    m_mouseDownStartTime = GetHighResTime() - INTEGER_INCREMENT_PERIOD;
	}
}


void InputScroller::MouseUp()
{
    m_mouseDownStartTime = -1.0f;
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
    ColourWindow *cw = new ColourWindow( LANGUAGEPHRASE("editor_coloureditor") );
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

    CreateValueControl( LANGUAGEPHRASE("editor_red"),   InputField::TypeChar, r, y,    1, 0, 255, m_callback, -1, m_w - 80 );
    CreateValueControl( LANGUAGEPHRASE("editor_green"), InputField::TypeChar, g, y+=h, 1, 0, 255, m_callback, -1, m_w - 80 );
    CreateValueControl( LANGUAGEPHRASE("editor_blue"),  InputField::TypeChar, b, y+=h, 1, 0, 255, m_callback, -1, m_w - 80 );
    CreateValueControl( LANGUAGEPHRASE("editor_alpha"), InputField::TypeChar, a, y+=h, 1, 0, 255, m_callback, -1, m_w - 80 );
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
