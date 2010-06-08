#ifndef _included_achievement_tracker_h
#define _included_achievement_tracker_h

#include <map>
#include <string>

#include "lib/tosser/llist.h"
#include "lib/tosser/hash_table.h"

#include "app.h"

#define ACHIEVEMENT_RECORD_FILENAME "achievements.txt"
#define ACHIEVEMENT_LEVEL_ENUM    "levels/level_enum.txt"

// defines for the viral achievement masks
#define VIRAL_MASTER_ACHIEVEMENT_MASK	0x01

class AchievementTracker
{
public:
	typedef std::map<int, std::string> LevelListReverseMap;

    HashTable<int>			m_levelList;
    LevelListReverseMap		m_levelListReverse;

    unsigned char			m_visited[8];
    unsigned char			m_won[8];

	bool					m_isMasterPlayer;
	bool					m_hasLoaded;

protected:
    int	m_numOfficialLevels;
	int	m_totalKills;

public:
    AchievementTracker();

    void AddVisitedLevel( char *_mapName );
    void AddVictoryOnLevel( char *_mapName );

    bool AlreadyWonLevel( char *_mapName );
    bool AlreadyVisitedLevel( char *_mapName );

    unsigned char *GetWonLevels();
    unsigned char *GetVisitedLevels();

    int NumWonLevels();
    int NumVisitedLevels();
	int GetCurrentLevelId();
	const char *GetLevelName( int _id ) const;
	int GetLevelId( const char *_mapFilename );

    void SetNumLevels( int _numLevels );

    void Load();
    void Save();

	void AddToTotalKills(int kills);

	unsigned char GetViralAchievementMask();

	bool HasLoaded();

	int GetTotalKills();
};

#endif