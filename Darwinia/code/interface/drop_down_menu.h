#ifndef INCLUDED_DROP_DOWN_MENU_H
#define INCLUDED_DROP_DOWN_MENU_H


#include <limits.h>

#include "interface/darwinia_window.h"


class DropDownOptionData
{
public:
	DropDownOptionData(const char *_word, int _value);
	~DropDownOptionData();

	char	*m_word;
	int		m_value;
};


// ****************************************************************************
// Class DropDownMenu
// ****************************************************************************

class DropDownWindow : public DarwiniaWindow
{
public:
    char m_parentName[256];
    static DropDownWindow *s_window;

public:
    DropDownWindow( char *_name, char *_parentName );
    void Update();

    // There can be only one
    static DropDownWindow *CreateDropDownWindow( char *_name, char *_parentName );
    static void RemoveDropDownWindow();
};


class DropDownMenu : public DarwiniaButton
{
protected:
    LList   <DropDownOptionData *> m_options;
    int     m_currentOption;
    int     *m_int;
	bool	m_sortItems;
	int		m_nextValue;	// Used by AddOption if a value isn't passed in as an argument

public:
    DropDownMenu(bool _sortItems = false);
    ~DropDownMenu();

    void            Empty               ();
    void            AddOption           ( char const *_option, int _value = INT_MIN );
    int             GetSelectionValue   ();
	char const *	GetSelectionName	();
    virtual void    SelectOption        ( int _option );
	bool			SelectOption2		( char const *_option );

    void    CreateMenu();
    void    RemoveMenu();
    bool    IsMenuVisible();

    void    RegisterInt( int *_int );

	int		FindValue(int _value);	// Returns an index into m_options

    void Render     ( int realX, int realY, bool highlighted, bool clicked );
    void MouseUp    ();
};


// ****************************************************************************
// Class DropDownMenuOption
// ****************************************************************************

class DropDownMenuOption : public BorderlessButton
{
public:
    char    *m_parentWindowName;
    char    *m_parentMenuName;
//    int     m_menuIndex;
	int		m_value;			// An integer that the client specifies - often a value from an enum

public:
    DropDownMenuOption();
	~DropDownMenuOption();

    void SetParentMenu( EclWindow *_window, DropDownMenu *_menu, int _value );

    void Render ( int realX, int realY, bool highlighted, bool clicked );
    void MouseUp();
};


#endif
