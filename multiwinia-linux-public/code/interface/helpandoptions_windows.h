
#ifndef _included_helpoptionswindow_h
#define _included_helpoptionswindow_h

#include "interface/darwinia_window.h"
#include "interface/mainmenus.h"

#include "lib/tosser/llist.h"

class HelpAndOptionsWindow : public GameOptionsWindow
{
public:
    HelpAndOptionsWindow();
    void Create();
};

class SelectHelpWindow : public GameOptionsWindow
{
public:
    SelectHelpWindow();
    void Create();
};

class CrateButton : public DarwiniaButton
{
private:
	bool					m_noTexture;
	char					m_textureName[256];
	int						m_crateNumber;
	double					m_startTime;
	unsigned int			m_imageWidth;
	unsigned int			m_imageHeight;
	LList<UnicodeString*>	*m_wrapped;	
public:
	float					m_yOffset;
	bool					m_reset;
	UnicodeString			m_crateName;
public:
	CrateButton( int _crateNumber, float _fontSize );
	~CrateButton();

	void Render( int realX, int realY, bool highlighted, bool clicked );
};

class CrateHelpWindow : public GameOptionsWindow
{
private:
	float	m_crateX;
	float	m_crateY;
	float	m_crateW;
	float	m_crateH;
	float	m_crateFontSize;
    float	m_crateGap;
	int	m_crateButtonOrderStartIndex;

	LList<CrateButton*>	m_crateButtons;
public:
	ScrollBar	*m_scrollBar;

	CrateHelpWindow();
	~CrateHelpWindow();

	void Create();
};

class GeneralHelpWindow : public GameOptionsWindow
{
public:
    int m_page;

public:
    GeneralHelpWindow();
    void Create();

    void ChangePage( int _pageNum );
};

class TextButton : public DarwiniaButton
{
public:
    char *m_string;
    bool m_rightAligned;
    bool m_shadow;
    UnicodeString m_unicode;
public:
    TextButton(char *_string);
    TextButton(UnicodeString _unicode );
    void Render( int realX, int realY, bool highlighted, bool clicked );
    void ChangeString( char *_newString );
};

class GameTypeHelpWindow : public GameOptionsWindow
{
public:
    int     m_gameType;
    int     m_page;

public:
    GameTypeHelpWindow( char *_name, int _typeId );
    void Create();

    void ChangePage( int _pageNum );

    static void GetLabelPos( int _gameType, int _labelId, int &_x, int &_y );
};

#endif