
#ifndef _included_gamemenu_h
#define _included_gamemenu_h

#include "interface/darwinia_window.h"
#include "interface/drop_down_menu.h"
#include "interface/input_field.h"

#define MAX_GAME_TYPES 6

class GlobalInternet;

class GameMenu
{
public:

    bool            m_menuCreated;
    GlobalInternet  *m_internet;

public:
    GameMenu();

    void Render();

    void CreateMenu();
    void DestroyMenu();
};

class GameMenuButton : public DarwiniaButton
{
public:
    char    *m_iconName;
public:
    GameMenuButton( char *_iconName );
    void Render( int realX, int realY, bool highlighted, bool clicked );
};

class GameMenuWindow : public DarwiniaWindow
{
public:
    int     m_currentPage;
    int     m_newPage;


    enum
    {
        PageMain = 0,
        PageDarwinia,
        PageMultiwinia,
        PageGameSetup,
        PageResearch,
        NumPages
    };

public:
    GameMenuWindow();

    void Create ();
    void Update();
    void Render ( bool _hasFocus );

    void SetupNewPage( int _page );
    void SetupMainPage();
    void SetupDarwiniaPage();

    void CreateMenuControl( char const *name, int dataType, void *value, int y,
							float change, float _lowBound, float _highBound,
                            DarwiniaButton *callback, int x, int w, float fontSize);

    void GetDefaultPositions( int *_x, int *_y, int *_gap );
};

#endif