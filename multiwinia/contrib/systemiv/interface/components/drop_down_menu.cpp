#include "lib/universal_include.h"
#include "lib/gucci/window_manager.h"
#include "lib/render/renderer.h"

#include <string.h>

#include "drop_down_menu.h"


// ****************************************************************************
// Class DropDownOptionData
// ****************************************************************************

DropDownOptionData::DropDownOptionData(const char *_word, int _value)
:   m_word(strdup(_word)), 
    m_value(_value)
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

DropDownWindow::DropDownWindow( char *_name, char *_title, bool _titleIsLanguagePhrase, char *_parentName )
:   InterfaceWindow(_name, _title, _titleIsLanguagePhrase)
{
    strcpy( m_parentName, _parentName );
}


void DropDownWindow::Update()
{
    InterfaceWindow::Update();

    EclWindow *parent = EclGetWindow( m_parentName );
    if( !parent )
    {
        RemoveDropDownWindow();
    }
}


DropDownWindow *DropDownWindow::CreateDropDownWindow( char *_name, char *_title, bool _titleIsLanguagePhrase, char *_parentName )
{
    if( s_window )
    {
        EclRemoveWindow( s_window->m_name );
        s_window = NULL;
    }

    s_window = new DropDownWindow( _name, _title, _titleIsLanguagePhrase, _parentName );
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
:   InterfaceButton(),
    m_currentOption(-1),
    m_int(NULL),
	m_sortItems(_sortItems),
	m_nextValue(0),
    m_orgcaption(NULL),
	m_orgcaptionIsLanguagePhrase(false)
{
}


DropDownMenu::~DropDownMenu()
{
    Empty();

	if( m_orgcaption ) delete [] m_orgcaption;	
}


void DropDownMenu::Empty()
{
    m_options.EmptyAndDelete();
	m_nextValue = 0;
    
    SelectOption( -1 );
}


void DropDownMenu::SetProperties( char *_name, int _x, int _y, int _w, int _h,
                                  char *_caption, char *_tooltip, 
                                  bool _captionIsLanguagePhrase, bool _tooltipIsLanguagePhrase )
{
	if( m_orgcaption ) delete [] m_orgcaption;
	if( _caption )
	{
		m_orgcaption = new char [strlen (_caption) + 1];
		strcpy ( m_orgcaption, _caption );
		m_orgcaptionIsLanguagePhrase = _captionIsLanguagePhrase;
	}
	else
	{
		m_orgcaption = NULL;
		m_orgcaptionIsLanguagePhrase = false;
	}

	EclButton::SetProperties( _name, _x, _y, _w, _h, _caption, _tooltip, _captionIsLanguagePhrase, _tooltipIsLanguagePhrase );
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
        SetCaption( m_name, false );
    }
    else
    {
        SetCaption( m_options[m_currentOption]->m_word, false );
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
        InterfaceButton::Render( realX, realY, true, clicked );
    }
    else
    {
        InterfaceButton::Render( realX, realY, highlighted, clicked );
    }
        
    float midPointY = realY + m_h/2.0f;

    glBegin( GL_TRIANGLES );
        glVertex2i( realX + m_w - 14, midPointY-4 );
        glVertex2i( realX + m_w - 6, midPointY-4 );
        glVertex2i( realX + m_w - 10, midPointY+4 );
    glEnd();
}


void DropDownMenu::CreateMenu()
{
    DropDownWindow *window = DropDownWindow::CreateDropDownWindow(m_name, m_orgcaption, m_orgcaptionIsLanguagePhrase, m_parent->m_name);
    window->SetPosition( m_parent->m_x + m_x + m_w, m_parent->m_y + m_y );

    int titleH = 22;
    int screenH = g_windowManager->WindowH();
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
            menuOption->SetProperties( thisName, col*m_w+2, titleH + i * m_h, w, m_h, thisOption, " ", false, false );
            menuOption->SetParentMenu( m_parent, this, m_options[index]->m_value );
            window->RegisterButton( menuOption );

            ++index;
        }
    }

    window->SetSize( m_w*numColumnsRequired, titleH + numPerColumn * m_h );    
    EclRegisterWindow( window );    
}


void DropDownMenu::RemoveMenu()
{    
    DropDownWindow::RemoveDropDownWindow();
}


void DropDownMenu::MouseUp()
{
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
:   TextButton(),
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
    m_parentWindowName = strdup( _window->m_name );

    if( m_parentMenuName )
    {
        delete [] m_parentMenuName;
        m_parentMenuName = NULL;
    }
    m_parentMenuName = strdup( _menu->m_name );    

//    m_menuIndex = _index;
	m_value = _value;
}


void DropDownMenuOption::Render( int realX, int realY, bool highlighted, bool clicked )
{
    //BorderlessButton::Render( realX, realY, highlighted, clicked );
    //return;
    
    EclWindow *window = EclGetWindow( m_parentWindowName );
    if( window )
    {
        EclButton *button = window->GetButton( m_parentMenuName );
        if( button )
        {
            DropDownMenu *menu = (DropDownMenu *) button;

            if( highlighted )
            {
                g_renderer->RectFill( realX, realY, m_w, m_h, Colour(100,100,200,100) );
            }

            if( menu->GetSelectionValue() == m_value )
            {
                g_renderer->RectFill( realX, realY, m_w, m_h, Colour(100,100,200,200) );
                TextButton::Render( realX, realY, highlighted, true );
            }
            else
            {
                TextButton::Render( realX, realY, highlighted, clicked);
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


