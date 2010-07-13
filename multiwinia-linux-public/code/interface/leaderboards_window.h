#ifndef _included_leaderboardwindow_h
#define _included_leaderboardwindow_h

#include "interface/darwinia_window.h"
#include "interface/mainmenus.h"
#include "game_menu.h"

class LeaderboardWindow;
class LeaderBoardListButton;

class LeaderboardLoader
{
private:
	int		m_searchIndexOffset;

public:
	LeaderboardLoader			();
	~LeaderboardLoader			();

	bool	IsNewSearchNeeded	();
	void	DoLeaderboardSearch	();

	int		GetSearchOffset		();

	bool	ScrollUp			();
	bool	ScrollDown			();

	bool	GetRowsToShow		( char* _rowsArray, int _numRows );
};

class LeaderboardWindow : public GameOptionsWindow
{		
private:
	int		m_leaderboardType;
	int		m_numRowsShowing;
	float	m_buttonYPos;

	void	SetUpBoardButton(LeaderBoardListButton* _button);
public:
	int		m_cursorPosition;
    int						m_leaderboardScrollOffset;
	LeaderboardLoader		m_leaderboardLoader;
public:
    LeaderboardWindow(int _type);
    void Create();
	int  GetLeaderboardType();

    void Update();
	void SetCurrentButton( EclButton *_button );

	void GetRankTextBit(unsigned _rank, char* _buffer);
};

class LeaderboardSelectionWindow : public GameOptionsWindow
{		
private:
public:
    LeaderboardSelectionWindow();
    void Create();
};

#endif