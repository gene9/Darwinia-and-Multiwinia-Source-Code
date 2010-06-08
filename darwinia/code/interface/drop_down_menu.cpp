#include "lib/universal_include.h"
#include "lib/text_renderer.h"
#include "lib/string_utils.h"

#include <string.h>

#include "app.h"
#include "renderer.h"

#include "interface/drop_down_menu.h"


// ****************************************************************************
// Class DropDownOptionData
// ****************************************************************************

DropDownOptionData::DropDownOptionData(const char *_word, int _value)
	: m_word(NewStr(_word)), m_value(_value)
{
}

DropDownOptionData::~DropDownOptionData()
{
	delete [] m_word;
}


// ****************************************************************************
// Class DropDownWindow
// ****************************************************************************

DropDownWindow *DropDownWindow::s_window = NULL;

DropDownWindow::DropDownWindow( char *_name, char *_parentName )
:   DarwiniaWindow(_name)
{
    strcpy( m_parentName, _parentName );
}


void DropDownWindow::Update()
{
    DarwiniaWindow::Update();

    EclWindow *parent = EclGetWindow( m_parentName );
    if( !parent )
    {
        RemoveDropDownWindow();
    }
}


DropDownWindow *DropDownWindow::CreateDropDownWindow( char *_name, char *_parentName )
{
    if( s_window )
    {
        EclRemoveWindow( s_window->m_name );
        s_window = NULL;
    }

    s_window = new DropDownWindow( _name, _parentName );
    return s_window;
}


void DropDownWindow::RemoveDropDownWindow()
{
    if( s_window )
    {
        EclRemoveWindow( s_window->m_name );
        s_window = NULL;
    }
}



// ****************************************************************************
// Class DropDownMenu
// ****************************************************************************


DropDownMenu::DropDownMenu(bool _sortItems)
:   DarwiniaButton(),
    m_currentOption(-1),
    m_int(NULL),
	m_sortItems(_sortItems),
	m_nextValue(0)
{
}


DropDownMenu::~DropDownMenu()
{
    Empty();
}


void DropDownMenu::Empty()
{
    m_options.EmptyAndDelete();
	m_nextValue = 0;
    
    SelectOption( -1 );
}


void DropDownMenu::AddOption( char const *_word, int _value )
{
	if (_value == INT_MIN)
	{
		_value = m_nextValue;
		m_nextValue++;
	}

	DropDownOptionData *option = new DropDownOptionData( _word, _value );

	if (m_sortItems)
	{
		int size = m_options.Size();
		int i;
		for (i = 0; i < size; ++i)
		{
			if (stricmp(_word, m_options[i]->m_word) < 0)
			{
				break;
			}
		}
		m_options.PutDataAtIndex(option, i);
	}
	else
	{
	    m_options.PutDataAtEnd(option);
	}
}


void DropDownMenu::SelectOption( int _value )
{
	m_currentOption = FindValue(_value);
    
    if( m_currentOption < 0 || m_currentOption >= m_options.Size() )
    {
        SetCaption( m_name );
    }
    else
    {
        SetCaption( m_options[m_currentOption]->m_word );
    }

    if( m_int && _value != -1 ) *m_int = _value;
}


bool DropDownMenu::SelectOption2(char const *_option)
{
	for (int i = 0; i < m_options.Size(); ++i)
	{
		char *itemName = m_options[i]->m_word;
		if (stricmp(itemName, _option) == 0)
		{
			SelectOption(m_options[i]->m_value);
			return true;
		}
	}

	return false;
}


int DropDownMenu::GetSelectionValue()
{
	if (m_currentOption >= 0 && m_currentOption < m_options.Size())
	{
		return m_options[m_currentOption]->m_value;
	}
	
	return -1;
}


char const *DropDownMenu::GetSelectionName()
{
	if (m_currentOption < 0 || m_currentOption > m_options.Size())
	{
		return NULL;
	}
	return m_options[m_currentOption]->m_word;
}


void DropDownMenu::RegisterInt( int *_int )
{
    m_int = _int;

    SelectOption( *m_int );
}


int DropDownMenu::FindValue(int _value)
{
	for (int i = 0; i < m_options.Size(); ++i)
	{
		if (m_options[i]->m_value == _value)
		{
			return i;
		}
	}

	return -1;
}


void DropDownMenu::Render( int realX, int realY, bool highlighted, bool clicked )
{
    if( IsMenuVisible() )
    {
        DarwiniaButton::Render( realX, realY, true, clicked );
    }
    else
    {
        DarwiniaButton::Render( realX, realY, highlighted, clicked );
    }

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    glBegin( GL_TRIANGLES );
        glVertex2i( realX + m_w - 12, realY + 5 );
        glVertex2i( realX + m_w - 3, realY + 5 );
        glVertex2i( realX + m_w - 7, realY + 9 );
    glEnd();
}


void DropDownMenu::CreateMenu()
{
    DropDownWindow *window = DropDownWindow::CreateDropDownWindow(m_name, m_parent->m_name);
    window->SetPosition( m_parent->m_x + m_x + m_w, m_parent->m_y + m_y );
    EclRegisterWindow( window );

    int screenH = g_app->m_renderer->ScreenH();
    int numColumnsRequired = 1 + ( m_h * m_options.Size() ) / (screenH * 0.8f);
    int numPerColumn =  ceil( (float) m_options.Size() / (float) numColumnsRequired );

    int index = 0;

    for( int col = 0; col < numColumnsRequired; ++col )
    {
        for( int i = 0; i < numPerColumn; ++i )
        {
            if( index >= m_options.Size() ) break;

            char *thisOption = m_options[index]->m_word;            
            char thisName[64];
            sprintf( thisName, "%s %d", m_name, index );

            int w = m_w - 4;

            DropDownMenuOption *menuOption = new DropDownMenuOption();
            menuOption->SetProperties( thisName, col*m_w+2, (i+1)*m_h, w, m_h, thisOption );
            menuOption->SetParentMenu( m_parent, this, m_options[index]->m_value );
            window->RegisterButton( menuOption );
			window->m_buttonOrder.PutData( menuOption );
			if( GetSelectionValue() == m_options[index]->m_value )
			{
				window->m_currentButton = index;
			}

            ++index;
        }
    }

    window->SetSize( m_w*numColumnsRequired, (numPerColumn+1) * m_h );    
}


void DropDownMenu::RemoveMenu()
{    
    DropDownWindow::RemoveDropDownWindow();
}


void DropDownMenu::MouseUp()
{
	if (m_disabled)
		return;
		
    if( IsMenuVisible() )
    {
        RemoveMenu();
    }
    else
    {
        CreateMenu();
    }
}


bool DropDownMenu::IsMenuVisible()
{
    return( EclGetWindow( m_name ) != NULL );
}


//*****************************************************************************
// Class DropDownMenuOption
//*****************************************************************************

DropDownMenuOption::DropDownMenuOption()
:   BorderlessButton(),
    m_parentWindowName(NULL),
    m_parentMenuName(NULL),
    m_value(-1)
{
}

DropDownMenuOption::~DropDownMenuOption()
{
	delete [] m_parentWindowName;
	delete [] m_parentMenuName;
}

void DropDownMenuOption::SetParentMenu( EclWindow *_window, DropDownMenu *_menu, int _value )
{
    if( m_parentWindowName )
    {
        delete [] m_parentWindowName;
        m_parentWindowName = NULL;
    }
    m_parentWindowName = NewStr( _window->m_name );

    if( m_parentMenuName )
    {
        delete [] m_parentMenuName;
        m_parentMenuName = NULL;
    }
    m_parentMenuName = NewStr( _menu->m_name );    

//    m_menuIndex = _index;
	m_value = _value;
}


void DropDownMenuOption::Render( int realX, int realY, bool highlighted, bool clicked )
{
    //BorderlessButton::Render( realX, realY, highlighted, clicked );
    //return;
    
    DarwiniaWindow *window = (DarwiniaWindow *)EclGetWindow( m_parentWindowName );
    if( window )
    {
        EclButton *button = window->GetButton( m_parentMenuName );
        if( button )
        {
            DropDownMenu *menu = (DropDownMenu *) button;
			if( window->m_buttonOrder[window->m_currentButton] == this )
            {
                BorderlessButton::Render( realX, realY, highlighted, true );
            }
            else
            {
                BorderlessButton::Render( realX, realY, highlighted, clicked);
            }
        }
    }
}


void DropDownMenuOption::MouseUp()
{
    EclWindow *window = EclGetWindow( m_parentWindowName );
    if( window )
    {
        EclButton *button = window->GetButton( m_parentMenuName );
        if( button )
        {
            DropDownMenu *menu = (DropDownMenu *) button;
            menu->SelectOption( m_value );
            menu->RemoveMenu();
        }
    }
}
