#include "lib/universal_include.h"

#include "../game_menu.h"
#include "level_file.h"
#include "MapData.h"

// *************************
// Map Data Class
// *************************

MapData::MapData() 
:   m_fileName(NULL), 
    m_levelName(NULL),
    m_description(NULL),
    m_author(NULL),
    m_difficulty(NULL),
    m_numPlayers(0),
    m_playTime(0),
    m_coop(false),
	m_mapId(-1),
    m_officialMap(false),
    m_forceCoop(false),
    m_customMap(false)
{
	memset( m_gameTypes, 0, sizeof( m_gameTypes ) );
}

MapData::~MapData() 
{
	SAFE_DELETE( m_fileName ); 
	SAFE_DELETE( m_levelName ); 
	SAFE_DELETE( m_description ); 
	SAFE_DELETE( m_author ); 
	SAFE_DELETE( m_difficulty ); 
}

void MapData::CalculeMapId()
{
	m_mapId = -1;

	if( !m_fileName )
	{
		return;
	}

	m_mapId = LevelFile::CalculeCRC( m_fileName );
}
