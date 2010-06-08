#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/resource.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/filesys/text_stream_readers.h"

#include "achievement_tracker.h"
#include "location.h"
#include "level_file.h"

#include "app.h"

#define MAX_LEVEL_ID 49

#ifdef    INCLUDEGAMEMODE_ASSAULT
#define VALID_ACHIEVEMENT_LEVELS 49
#else
#define VALID_ACHIEVEMENT_LEVELS 41
#endif 


AchievementTracker::AchievementTracker()
:   m_numOfficialLevels(0),
	m_totalKills(-1),
	m_isMasterPlayer(false),
	m_hasLoaded(false)
{
    memset( m_won, 0, sizeof(m_won) );
    memset( m_visited, 0, sizeof(m_visited) );

	TextReader *in = g_app->m_resource->GetTextReader( ACHIEVEMENT_LEVEL_ENUM );
	if( !in )
	{
		AppDebugOut("Failed to load level_enum.txt\n");
	}
	else if( *in )
	{
		while( in->ReadLine() )
		{
			if( !in->TokenAvailable() ) continue;
			char *word = in->GetNextToken();
			int  value = atoi( in->GetNextToken() );

			m_levelList.PutData( word, value );
			m_levelListReverse[value] = word;
		}
	}
	delete in;
}

unsigned char AchievementTracker::GetViralAchievementMask()
{
	unsigned char ret = 0;
	if( m_isMasterPlayer ) ret |= VIRAL_MASTER_ACHIEVEMENT_MASK;
	AppDebugOut("Viral achievement mask is %d\n", ret);
	return ret;
}

void AchievementTracker::Load()
{
}

bool AchievementTracker::HasLoaded()
{
	return m_hasLoaded;
}

int AchievementTracker::GetTotalKills()
{
	return m_totalKills;
}

void AchievementTracker::AddToTotalKills(int kills)
{
	m_totalKills += kills;
	if( m_totalKills >= 1000000 ) g_app->GiveAchievement(App::DarwiniaAchievementGenocide);
	Save();
}

void AchievementTracker::Save()
{
    if( !g_app->IsFullVersion() ) return;
}

void AchievementTracker::AddVictoryOnLevel(char *_mapName)
{
    if( AlreadyWonLevel( _mapName ) ) return;

    int levelId = m_levelList.GetData( _mapName, -1 );  
    if( levelId == -1 ) return;
    if( levelId > MAX_LEVEL_ID ) return;

    int index = levelId / 8;
    int mod = levelId % 8;
    m_won[index] |= (1 << mod);

    Save();

    if( NumWonLevels() == VALID_ACHIEVEMENT_LEVELS )
    {
        g_app->GiveAchievement( App::DarwiniaAchievementMasterOfMultiwinia );
    }
}

void AchievementTracker::AddVisitedLevel(char *_mapName)
{
    if( AlreadyVisitedLevel( _mapName ) ) return;

    int levelId = m_levelList.GetData( _mapName, -1 );  
    if( levelId == -1 ) return;
    if( levelId > MAX_LEVEL_ID ) return;

    int index = levelId / 8;
    int mod = levelId % 8;
    m_visited[index] |= (1 << mod);

    Save();

    if( NumVisitedLevels() == VALID_ACHIEVEMENT_LEVELS )
    {
        g_app->GiveAchievement( App::DarwiniaAchievementExplorer );
    }
}

int AchievementTracker::GetLevelId( const char *_mapFilename )
{
	return m_levelList.GetData( _mapFilename, -1 );
}

int AchievementTracker::GetCurrentLevelId()
{	
	Location *location = g_app->m_location;
	if( !location )
		return -1;

	LevelFile *levelFile = location->m_levelFile;
	if( !levelFile )
		return -1;

	int levelId = GetLevelId( levelFile->m_mapFilename );

	if( levelId == -1 )
	{
		AppDebugOut("Failed to get level id for %s\n", levelFile->m_mapFilename);
	}

	return levelId;
}

const char *AchievementTracker::GetLevelName( int _id ) const
{
	LevelListReverseMap::const_iterator i = m_levelListReverse.find( _id );
	if( i == m_levelListReverse.end() )
	{
		AppDebugOut("AchievementTracker::GetLevelName(%d) - no such level id.\n", _id );
		return NULL;
	}
	else
		return i->second.c_str();
}




bool AchievementTracker::AlreadyWonLevel( char *_mapName )
{
    int levelId = m_levelList.GetData( _mapName, -1 );  
    if( levelId == -1 ) return false;

    return (m_won[ levelId / 8 ] >> (levelId % 8)) & 1;
}

bool AchievementTracker::AlreadyVisitedLevel(char *_mapName)
{
    int levelId = m_levelList.GetData( _mapName, -1 );  
    if( levelId == -1 ) return false;

    return (m_visited[ levelId / 8 ] >> (levelId % 8)) & 1;
}

void AchievementTracker::SetNumLevels( int _numLevels )
{
    m_numOfficialLevels = _numLevels;
}

unsigned char *AchievementTracker::GetWonLevels()
{
    return m_won;
}

unsigned char *AchievementTracker::GetVisitedLevels()
{
    return m_visited;
}

int AchievementTracker::NumWonLevels()
{
    int num = 0;
    for( int i = 0; i < 8; ++i )
    {
        for( int j = 0; j < 8; ++j )
        {
            if( ( m_won[i] >> j ) & 1 ) num++;
        }
    }

    return num;
}

int AchievementTracker::NumVisitedLevels()
{
    int num = 0;
    for( int i = 0; i < 8; ++i )
    {
        for( int j = 0; j < 8; ++j )
        {
            if( ( m_visited[i] >> j ) & 1 ) num++;
        }
    }

    return num;
}

