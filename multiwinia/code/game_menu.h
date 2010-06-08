
#ifndef _included_gamemenu_h
#define _included_gamemenu_h

#include "interface/darwinia_window.h"
#include "interface/drop_down_menu.h"
#include "interface/input_field.h"
#include "interface/checkbox.h"

#include "multiwinia.h"
#include "network/networkvalue.h"

#include "lib/language_table.h"

#define IS_DEMO !g_app->IsFullVersion()
#define SERVERS_ON_SCREEN 13
#define MAX_GAME_TYPES 7

class GlobalInternet;
class Clouds;
class Directory;
class ScrollBar;

#define GAMETYPE_PROLOGUE   254
#define GAMETYPE_CAMPAIGN   255

void RenderHighlightBeam( float x, float centreY, float w, float h );
void GetFontSizes( float &_large, float &_med, float &_small );
void GetPosition_LeftBox( float &x, float &y, float &w, float &h );
void GetPosition_RightBox( float &x, float &y, float &w, float &h );
void GetPosition_TitleBar( float &x, float &y, float &w, float &h );
void GetButtonSizes( float &width, float &height, float &gap );
void DrawFillBox( float x, float y, float w, float h );


char *GetMapNameId( char *_filename );
int GetTimeOption();
char *GetMapDescriptionId( char *_filename );
void GameTypeToShortCaption( int _gameType, char* _dest );
void GameTypeToCaption( int _gameType, char *_dest );
void RemoveDelistedServers( LList<Directory *> *_serverList );
bool IsProtocolMatch( Directory *d );


class MapData;

// ============================================================================
// Game Menu

class PlayerNameString : public NetworkUnicodeString
{
public:
	PlayerNameString( const UnicodeString &_name = UnicodeString(), int _maxLength = 16 )
		: m_name( _name ), m_maxLength( _maxLength )
	{
	}

	const UnicodeString &Get() const
	{
		return m_name;
	}

	void Set( const UnicodeString &_x );

private:
	UnicodeString m_name;
	int m_maxLength;
};

class GameMenu
{
public:

    bool            m_menuCreated;
	DArray<MapData *> m_allMaps;
    DArray<MapData *> m_maps[MAX_GAME_TYPES];
    int             m_numDemoMaps[MAX_GAME_TYPES];

//    GlobalInternet  *m_internet;
    Clouds          *m_clouds;

public:
    GameMenu();
	~GameMenu();

    void Render();

    void CreateMenu();
    void DestroyMenu();

    void CreateMapList();
    bool AddMap( TextReader *_in, char *_filename );

    int GetNumPlayers();

    int GetMapIndex( int gameMode, int mapCrc );
	int GetMapIndex( int _gameMode, const char *_fileName );

	MapData *GetMap( int _gameMode, const char *_fileName );
	MapData *GetMap( int _gameMode, int _mapCrc );

    const char *GetMapName( int gameMode, int mapCrc );
	const char *GetMapName( int _gameMode, const char *_fileName );
};



class GameMenuTitleButton : public DarwiniaButton
{
public:
    void Render( int realX, int realY, bool highlighted, bool clicked );
};


// ============================================================================
// Game Menu Components

class GameMenuButton : public DarwiniaButton
{
public:
    bool    m_useEditorFont;
    //bool    m_inactive;
	float	m_ir, m_ig, m_ib, m_ia;		// Inactive Colour
    double  m_birthTime;
    int     m_transitionDirection;

	static EclButton *s_previousHighlightButton;

	//GameMenuButton();
	GameMenuButton( const UnicodeString &caption=UnicodeString() );


    void MouseUp();
    void Render( int realX, int realY, bool highlighted, bool clicked );

	void LockController();
};



class GameMenuDropDown : public DropDownMenu
{
public:
    bool    m_noBackground;
    bool    m_cycleMode;

    GameMenuDropDown( bool _sortItems = false );
    void Render( int realX, int realY, bool highlighted, bool clicked );
    void CreateMenu();
	void MouseUp();

private:
	int m_previousValue;
};



class GameMenuDropDownWindow : public DropDownWindow
{
public:
    GameMenuDropDownWindow( char *_name, char *_parentName );
    ~GameMenuDropDownWindow();

	void Create();
    void Render( bool hasFocus );

    static DropDownWindow *CreateGMDropDownWindow( char *_name, char *_parentName );
};


class GameMenuDropDownMenuOption : public DropDownMenuOption
{
public:
    bool    m_currentOption;
    GameMenuDropDownMenuOption();
    void Render( int realX, int realY, bool highlighted, bool clicked );
	void MouseUp();
};



class GameMenuInputField : public InputField
{
public:

    bool    m_noBackground;
    bool    m_renderStyle2;

    GameMenuInputField();
    void Render( int realX, int realY, bool highlighted, bool clicked );
    void Keypress( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii );
    void MouseUp();
};



class GameMenuInputScroller : public InputScroller
{
public:
    GameMenuInputScroller();
    void Render( int realX, int realY, bool hglighted, bool clicked );
};

class BackPageButton : public GameMenuButton
{
public:
    int m_pageId;

    BackPageButton( int _page );
	void MouseUp();
	void Render( int realX, int realY, bool highlighted, bool clicked );
};

class GameMenuCheckBox : public CheckBox
{
public:
    GameMenuCheckBox()
    :   CheckBox()
    {
    }

    void Render( int realX, int realY, bool highlighted, bool clicked );
};

#endif
