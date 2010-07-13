#ifndef __MAPDATA__
#define __MAPDATA__

#include "game_menu.h"

class MapData
{
public:
    char    *m_fileName;
    char    *m_levelName;
    char    *m_description;
    char    *m_author;
    char    *m_difficulty;

    int     m_numPlayers;
    int     m_playTime;
    bool    m_coop;
    bool    m_forceCoop;
	bool    m_officialMap;
    int		m_mapId;            // crc-32 of map file

    bool    m_customMap;

	bool	m_gameTypes[MAX_GAME_TYPES];

    MapData();
    ~MapData();

	void	CalculeMapId();
};

#endif
