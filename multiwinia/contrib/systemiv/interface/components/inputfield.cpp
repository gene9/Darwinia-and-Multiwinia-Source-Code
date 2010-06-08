#include "lib/universal_include.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <algorithm>

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/render/renderer.h"
#include "lib/render/styletable.h"
#include "lib/gucci/input.h"

#include "inputfield.h"


#define PIXELS_PER_CHAR	8

// ****************************************************************************
// Class InputField
// ****************************************************************************

InputField::InputField()
:	EclButton(),
    m_type(TypeNowt),
	m_string(NULL),
	m_int(NULL),
	m_float(NULL),
    m_char(NULL),
    m_callback(NULL),
	m_lowBound(0.0f),
	m_highBound(1e4),
    m_align(0),
    m_lastKeyPressTimer(0)
{
	m_buf[0] = '\0';
}


void InputField::SetCallback(InterfaceButton *button)
{
    m_callback = button;
}


void InputField::Render( int realX, int realY, bool highlighted, bool clicked )
{
    Colour background = g_styleTable->GetPrimaryColour(STYLE_INPUT_BACKGROUND);
    Colour borderA = g_styleTable->GetPrimaryColour(STYLE_INPUT_BORDER);
    Colour borderB = g_styleTable->GetSecondaryColour(STYLE_INPUT_BORDER);

    g_renderer->RectFill    ( realX, realY, m_w, m_h, background );
    g_renderer->Line        ( realX, realY, realX + m_w, realY, borderA );
    g_renderer->Line        ( realX, realY, realX, realY + m_h, borderA );
    g_renderer->Line        ( realX + m_w, realY, realX + m_w, realY + m_h, borderB );
    g_renderer->Line        ( realX, realY + m_h, realX + m_w, realY + m_h, borderB );

	if (!highlighted)
	{
	    switch(m_type)
	    {
	        case TypeChar:	    sprintf(m_buf, "%d", (int) *m_char);		    break;
	        case TypeInt:	    sprintf(m_buf, "%d", *m_int);		            break;
	        case TypeFloat:     sprintf(m_buf, "%.2f", *m_float);               break;
	        case TypeString:    strncpy(m_buf, m_string, sizeof(m_buf) - 1);    break;
	    }
    }

    float textWidth = g_renderer->TextWidth(m_buf, 12);
    bool tooWide = textWidth > m_w && m_type == TypeString;

    float fieldX = -1;
    if( m_align == -1 )             fieldX = realX+5;
    if( m_align == 0 )              fieldX = realX + m_w/2 - textWidth/2;
    if( m_align == 1 || tooWide )   fieldX = realX + m_w - textWidth - 5;
    

    float yPos = realY + (m_h-10)/2;

    g_renderer->SetClip( realX, realY, m_w, m_h );

    g_renderer->TextSimple( fieldX, yPos, White, 12, m_buf );

    char *focus = EclGetCurrentFocus();
	if (focus &&
        stricmp(focus, m_parent->m_name) == 0 &&
        m_parent->m_currentTextEdit && 
        strcmp(m_parent->m_currentTextEdit, m_name) == 0 )
	{
		if (fmodf(GetHighResTime(), 1.0f) < 0.5f ||
            GetHighResTime() < m_lastKeyPressTimer+1.0f )
		{
			float cursorX = fieldX + textWidth + 1;
            g_renderer->Line( cursorX, realY + 2, cursorX, realY + m_h - 2, White );
		}
	}

    g_renderer->ResetClip();
}


void InputField::MouseUp()
{
	m_parent->BeginTextEdit(m_name);
}


void InputField::Keypress ( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii )
{
    m_lastKeyPressTimer = GetHighResTime();

	if (strcmp(m_parent->m_currentTextEdit, "None") == 0)	return;

    if( ascii == '%' ) return;                  // This is a banned character because it breaks
                                                // any future calls to sprintf etc
    if( keyCode == KEY_ESC ||
        keyCode == KEY_TAB ||
        keyCode == KEY_UP ||
        keyCode == KEY_DOWN ||
        keyCode == KEY_LEFT || 
        keyCode == KEY_RIGHT )
    {
        // More banned keys
        return;
    }

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
    else if( keyCode == KEY_SHIFT ||
             keyCode == KEY_ALT )
    {
    }
    else
    {
        if( m_type == TypeString ||
            m_type == TypeInt ||
            m_type == TypeFloat )
        {
            if( strlen(m_buf) < sizeof(m_buf)-1 &&
                strlen(m_buf) < (int) m_highBound &&
                !g_keys[KEY_COMMAND] )
            {
                int location = strlen(m_buf);
                m_buf[ location ] = ascii;
                m_buf[ location+1 ] = '\x0';         

                if( m_type == TypeString )
                {
                    strcpy(m_string, m_buf);
                    Refresh();
                }
            }
            ClampToBounds();
        }
    }
}


void InputField::RegisterChar(unsigned char *_char)
{
	AppDebugAssert(m_type == TypeNowt ||
                   m_type == TypeChar );
	m_type = TypeChar;
	m_char = _char;
}


void InputField::RegisterInt(int *_int)
{
	AppDebugAssert(m_type == TypeNowt ||
                   m_type == TypeInt );
	m_type = TypeInt;
	m_int = _int;
}


void InputField::RegisterFloat(float *_float)
{
	AppDebugAssert(m_type == TypeNowt ||
                   m_type == TypeFloat );
	m_type = TypeFloat;
	m_float = _float;
}


void InputField::RegisterString(char *_string)
{
	AppDebugAssert(m_type == TypeNowt ||
                   m_type == TypeString );
	m_type = TypeString;
	m_string = _string;

    m_highBound = std::min( m_highBound, 32.0f );
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
        case TypeString:
            if( strlen(m_buf) > m_highBound )
            {
                m_buf[(int)m_highBound] = '\x0';
            }
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
:   InterfaceButton(),
    m_inputField(NULL),
    m_change(0.0f),
    m_mouseDownStartTime(-1.0f)
{
}


#define INTEGER_INCREMENT_PERIOD 0.1f

void InputScroller::Render( int realX, int realY, bool highlighted, bool clicked )
{
    InterfaceButton::Render( realX, realY, highlighted, clicked );

    //
    // Render arrow

    glBegin( GL_TRIANGLES );
    float midPoint = realY + m_h/2;
    if( m_change < 0 )
    {
        glVertex2f( realX + 6, midPoint );
        glVertex2f( realX + 14, midPoint-4 );
        glVertex2f( realX + 14, midPoint+4 );
    }
    else
    {
        glVertex2f( realX + m_w - 14, midPoint-4 );
        glVertex2f( realX + m_w - 6, midPoint );
        glVertex2f( realX + m_w - 14, midPoint + 4 );
    }
    glEnd();

    
    if( m_mouseDownStartTime > 0.0f &&
		m_inputField &&
		g_inputManager->m_mouseX >= realX &&
        g_inputManager->m_mouseX < realX + m_w &&
        g_inputManager->m_mouseY >= realY &&
        g_inputManager->m_mouseY < realY + m_h &&
        g_inputManager->m_lmb )
    {
		float change = m_change;
        if( g_keys[KEY_SHIFT] ) change *= 5.0f;

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
	if (m_mouseDownStartTime == -1.0f )
	{
	    m_mouseDownStartTime = GetHighResTime() - INTEGER_INCREMENT_PERIOD;
	}
}


void InputScroller::MouseUp()
{
    m_mouseDownStartTime = -1.0f;
}
