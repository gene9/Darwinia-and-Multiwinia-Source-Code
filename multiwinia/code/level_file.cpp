#include "lib/universal_include.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fstream>

#include "lib/unicode/unicode_text_stream_reader.h"
#include "lib/tosser/directory.h"

#include "lib/debug_utils.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/resource.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/preferences.h"
#include "lib/math/random_number.h"
#include "lib/math/filecrc.h"

#include "interface/prefs_other_window.h"

#include "worldobject/researchitem.h"
#include "worldobject/darwinian.h"
#include "worldobject/officer.h"
#include "worldobject/anthill.h"
#include "worldobject/incubator.h"
#include "worldobject/armour.h"
#include "worldobject/radardish.h"
#include "worldobject/rocket.h"
#include "worldobject/engineer.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/ai.h"
#include "worldobject/triffid.h"
#include "worldobject/switch.h"
#include "worldobject/laserfence.h"
#include "worldobject/generichub.h"

#include "app.h"
#include "camera.h"
#include "global_world.h"
#include "level_file.h"
#include "location.h"
#include "routing_system.h"
#include "team.h"
#include "unit.h"
#include "taskmanager.h"
#include "gametimer.h"
#include "mapfile.h"



//*****************************************************************************
// Class CamAnimNode
//*****************************************************************************

static char const *g_transitionModeNames[] = {
	"Move",
	"Cut"
};


int CamAnimNode::GetTransitModeId(char const *_word)
{
	for (int i = 0; i < TransitionNumModes; ++i)
	{
		if (stricmp(_word, g_transitionModeNames[i]) == 0)
		{
			return i;
		}
	}

	return -1;
}


char const *CamAnimNode::GetTransitModeName(int _modeId)
{
	AppDebugAssert(_modeId >= 0 && _modeId < TransitionNumModes);
	return g_transitionModeNames[_modeId];
}



//*****************************************************************************
// Class LevelFile
//*****************************************************************************

// ***************
// Private Methods
// ***************

void LevelFile::ParseMissionFile(char const *_filename)
{
	TextReader *in = NULL;              
    char fullFilename[256];

    if( !g_app->m_editing )
    {
        // Try to load a save game first
        sprintf( fullFilename, "%susers/%s/%s", g_app->GetProfileDirectory(), g_app->m_userProfileName, _filename );       
        if( DoesFileExist( fullFilename ) ) in = new UnicodeTextFileReader( fullFilename ); 

		if (in && !in->IsUnicode())
		{
			delete in;
			in = new TextFileReader( fullFilename ); 
		}
    }

    if( !in )
    {
        sprintf( fullFilename, "levels/%s", _filename );
        in = g_app->m_resource->GetTextReader(fullFilename);
    }

	AppReleaseAssert(in && in->IsOpen(), "Invalid level specified");

	while(in->ReadLine())
    {
		if (!in->TokenAvailable()) continue;
		char *word = in->GetNextToken();
    
		if (stricmp("Landscape_StartDefinition", word) == 0 ||
		    stricmp("LandscapeTiles_StartDefinition", word) == 0 ||
			stricmp("LandFlattenAreas_StartDefinition", word) == 0 ||
			stricmp("Lights_StartDefinition", word) == 0)
		{
			AppDebugAssert(0);
		}
		else if (stricmp("CameraMounts_StartDefinition", word) == 0)
		{
			ParseCameraMounts(in);
		}
		else if (stricmp("CameraAnimations_StartDefinition", word) == 0)
		{
			ParseCameraAnims(in);
		}
		else if (stricmp("Buildings_StartDefinition", word) == 0)
		{
			ParseBuildings(in, true);
		}
		else if (stricmp("InstantUnits_StartDefinition", word) == 0)
		{
			ParseInstantUnits(in);
		}
		else if (stricmp("Routes_StartDefinition", word) == 0) 
		{
			ParseRoutes(in);
		}
		else if (stricmp("PrimaryObjectives_StartDefinition", word) == 0)
		{
			ParsePrimaryObjectives(in);
		}
        else if (stricmp("RunningPrograms_StartDefinition", word) == 0)
        {
            ParseRunningPrograms(in);
        }
		else if (stricmp("Difficulty_StartDefinition", word) == 0)
		{
			ParseDifficulty(in);
		}
		else if (stricmp("LeaderboardStats_StartDefinition", word) == 0)
		{
			ParseLeaderboardStats(in);
		}
		else
		{
			// Looks like a damaged level file
			AppDebugAssert(0);
		}
    }
	
	delete in;
}

void LevelFile::ParseMWMFile( char const *_filename )
{
    char fullfilename[512];
    sprintf( fullfilename, "%s%s", g_app->GetMapDirectory(), _filename );
    
    MapFile *mapFile = new MapFile(fullfilename, true);
    TextReader *in = mapFile->GetTextReader();

    ParseMapFile( in );

    delete in;
    delete mapFile;
}

void LevelFile::ParseMapFile( TextReader *in )
{
	while(in->ReadLine())
    {
		if (!in->TokenAvailable()) continue;
		char *word = in->GetNextToken();

        if( stricmp("MultiwiniaOptions_StartDefinition", word ) == 0 )
        {
            ParseMultiwiniaOptions(in);
        }
		else if (stricmp("landscape_startDefinition", word) == 0)
		{
			ParseLandscapeData(in);
   		}
		else if (stricmp("landscapeTiles_startDefinition", word) == 0)
		{
			ParseLandscapeTiles(in);
		}
		else if (stricmp("landFlattenAreas_startDefinition", word) == 0)
		{
			ParseLandFlattenAreas(in);
		}
		else if (stricmp("Buildings_StartDefinition", word) == 0)
		{
			ParseBuildings(in, false);
		}
		else if (stricmp("Lights_StartDefinition", word) == 0)
		{
			ParseLights(in);
		}
		else if (stricmp("CameraMounts_StartDefinition", word) == 0)
		{
			ParseCameraMounts(in);
		}
		else if (stricmp("CameraAnimations_StartDefinition", word) == 0)
		{
			ParseCameraAnims(in);
		}
		else if (stricmp("Buildings_StartDefinition", word) == 0)
		{
			ParseBuildings(in, true);
		}
		else if (stricmp("InstantUnits_StartDefinition", word) == 0)
		{
			ParseInstantUnits(in);
		}
		else if (stricmp("Routes_StartDefinition", word) == 0) 
		{
			ParseRoutes(in);
		}
		else if (stricmp("PrimaryObjectives_StartDefinition", word) == 0)
		{
			ParsePrimaryObjectives(in);
		}
		else
		{
			AppDebugAssert(0);
		}
    }
}

void LevelFile::ParseMapFile(char const *_levelFilename)
{
    char fullFilename[256];
    sprintf( fullFilename, "levels/%s", _levelFilename );
	TextReader *in = g_app->m_resource->GetTextReader(fullFilename);
    if( !in )
    {
        sprintf( fullFilename, "%s%s", g_app->GetMapDirectory(), _levelFilename );
        in = new TextFileReader( fullFilename );
    }
	AppReleaseAssert(in && in->IsOpen(), "Invalid map file specified\n");

    ParseMapFile( in );

    delete in;

}


void LevelFile::ParseCameraMounts(TextReader *_in)
{
	while (_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;
		char *word = _in->GetNextToken();

		if (stricmp("CameraMounts_EndDefinition", word) == 0) return;
		
		CameraMount *cmnt = new CameraMount();
	
		// Read name
		strncpy(cmnt->m_name, word, CAMERA_MOUNT_MAX_NAME_LEN);
		
		// Read pos
		word = _in->GetNextToken();
		cmnt->m_pos.x = iv_atof(word);
		word = _in->GetNextToken();
		cmnt->m_pos.y = iv_atof(word);
		word = _in->GetNextToken();
		cmnt->m_pos.z = iv_atof(word);

		// Read front
		word = _in->GetNextToken();
		cmnt->m_front.x = iv_atof(word);
		word = _in->GetNextToken();
		cmnt->m_front.y = iv_atof(word);
		word = _in->GetNextToken();
		cmnt->m_front.z = iv_atof(word);
		cmnt->m_front.Normalise();

		// Read up
		word = _in->GetNextToken();
		cmnt->m_up.x = iv_atof(word);
		word = _in->GetNextToken();
		cmnt->m_up.y = iv_atof(word);
		word = _in->GetNextToken();
		cmnt->m_up.z = iv_atof(word);
		cmnt->m_up.Normalise();

		m_cameraMounts.PutData(cmnt);
	}
}


void LevelFile::ParseCameraAnims(TextReader *_in)
{
	while (_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;
		char *word = _in->GetNextToken();

		if (stricmp("CameraAnimations_EndDefinition", word) == 0) return;

		CameraAnimation *anim = new CameraAnimation();
		strncpy(anim->m_name, word, CAMERA_ANIM_MAX_NAME_LEN);

		while (_in->ReadLine())
		{
			if (!_in->TokenAvailable()) continue;
			word = _in->GetNextToken();
			if (stricmp(word, "End") == 0)
			{
				break;
			}

			CamAnimNode *node = new CamAnimNode();

			// Read camera mode
			node->m_transitionMode = CamAnimNode::GetTransitModeId(word);
			AppReleaseAssert(node->m_transitionMode >= 0 &&
						  node->m_transitionMode < Camera::ModeNumModes,
						  "Bad camera animation camera mode in level file %s", m_missionFilename);
			
			// Read mount name
			word = _in->GetNextToken();
			node->m_mountName = strdup(word);
			if (stricmp(node->m_mountName, MAGIC_MOUNT_NAME_START_POS))
			{
				AppReleaseAssert(GetCameraMount(node->m_mountName),
						  "Bad camera animation mount name in level file %s", m_missionFilename);
			}

			// Read time
			word = _in->GetNextToken();
			node->m_duration = iv_atof(word);
			AppReleaseAssert(node->m_duration >= 0.0 &&
						  node->m_duration < 60.0,
						  "Bad camera animation transition time in level file %s", m_missionFilename);

			anim->m_nodes.PutDataAtEnd(node);
		}

		m_cameraAnimations.PutData(anim);
	}
}

void LevelFile::ParseLeaderboardStats(TextReader *_in)
{
	AppDebugOut("Parsing LeaderboardStats_StartDefinition\n");
	while( _in->ReadLine() )
    {
        if( !_in->TokenAvailable() ) continue;
        char *word = _in->GetNextToken();

		if( stricmp("LeaderboardStats_EndDefinition", word ) == 0 )
		{
			AppDebugOut("Done parsing leaderboardstats info\n");
			return;
		}
        else if( stricmp( "Time", word ) == 0 )
        {
			char* restOLine = _in->GetRestOfLine();
			AppDebugOut("Rest of line is %s\n", restOLine);
			sscanf(restOLine, "%f", &m_leaderboardLevelTime);
			m_lastLeaderboardLevelTime = m_leaderboardLevelTime;
			AppDebugOut("Time %f\n", m_leaderboardLevelTime);
		}
		else if( stricmp( "Kills", word ) == 0 )
		{
			sscanf(_in->GetRestOfLine(), "%d", &(g_app->m_location->m_teams[2]->m_teamKills));
			AppDebugOut("Kills %d\n", g_app->m_location->m_teams[2]->m_teamKills);
		}
		else if( stricmp( "Survivors", word ) == 0 )
		{
			// Should not need survivors as that is in g_app->m_location->m_teams[0]->m_others.NumUsed();
			AppDebugOut("Survivors\n");
		}
	}
}

void LevelFile::ParseMultiwiniaOptions(TextReader *_in)
{
    while( _in->ReadLine() )
    {
        if( !_in->TokenAvailable() ) continue;
        char *word = _in->GetNextToken();

        if( stricmp("MultiwiniaOptions_EndDefinition", word ) == 0 ) return;
        else if( stricmp( "PopulationCap", word ) == 0 )
        {
			int populationCap = atoi( _in->GetNextToken() );
            m_populationCap = populationCap;
        }
        else if( stricmp( "DefenderPopulationCap", word ) == 0 )
        {
            m_defenderPopulationCap = atoi( _in->GetNextToken() );
        }
        else if( stricmp( "Effects", word ) == 0 )
        {
            while( _in->TokenAvailable() )
            {
                char *effect = _in->GetNextToken();
                if( strcmp( effect, "snow" ) == 0 ) m_effects[LevelEffectSnow] = true;
                if( strcmp( effect, "dustwind" ) == 0 ) m_effects[LevelEffectDustStorm] = true;
                if( strcmp( effect, "soulrain" ) == 0 ) m_effects[LevelEffectSoulRain] = true;
                if( strcmp( effect, "lightning" ) == 0 ) m_effects[LevelEffectLightning] = true;
                if( strcmp( effect, "SpecialLighting") == 0 ) m_effects[LevelEffectSpecialLighting] = true;
                //m_effects.PutData( strdup(_in->GetNextToken()) );
            }
        }
        else if( stricmp( "GameTypes", word ) == 0 )
        {
            char *type = _in->GetNextToken();
            for( int i = 0; i < g_app->m_multiwinia->s_gameBlueprints.Size(); ++i )
            {
                if( strcmp( g_app->m_multiwinia->s_gameBlueprints[i]->m_name, type ) == 0 )
                {
                    m_gameTypes = i;
                    break;
                }
            }
        }
        else if( stricmp( "NumPlayers", word ) == 0 )
        {
            m_numPlayers = atoi( _in->GetNextToken() );
        }
        else if( stricmp( "LevelOptions", word ) == 0 )
        {
            while( _in->TokenAvailable() )
            {
                char *option = _in->GetNextToken();
                if( stricmp( "nohandicap", option ) == 0 )
                {
                    m_levelOptions[LevelOptionNoHandicap] = 1;
                }
                else if( stricmp( "instantspawnpointcapture", option ) == 0 )
                {
                    m_levelOptions[LevelOptionInstantSpawnPointCapture] = 1;
                }
                else if( stricmp( "trunkportreinforcements", option ) == 0)
                {
                    m_levelOptions[LevelOptionTrunkPortReinforcements] = atoi( _in->GetNextToken() );
                }
                else if ( stricmp( "trunkportarmour", option ) == 0 )
                {
                    m_levelOptions[LevelOptionTrunkPortArmour] = atoi( _in->GetNextToken() );
                }
                else if ( stricmp( "nogunturrets", option ) == 0 )
                {
                    m_levelOptions[LevelOptionNoGunTurrets] = 1;
                }
                else if( stricmp( "attackerreinforcements", option ) == 0 )
                {
                    m_levelOptions[LevelOptionAttackerReinforcements] = atoi( _in->GetNextToken() );
                }
                else if( stricmp( "defenderreinforcements", option ) == 0 )
                {
                    m_levelOptions[LevelOptionDefenderReinforcements] = atoi( _in->GetNextToken() );
                }
                else if( stricmp( "defenderreinforcementtype", option ) == 0 )
                {
                    m_levelOptions[LevelOptionDefenderReinforcementType] = GlobalResearch::GetType( _in->GetNextToken() );
                }
                else if( stricmp( "attackerreinforcementtype", option ) == 0 )
                {
                    m_levelOptions[LevelOptionAttackerReinforcementType] = GlobalResearch::GetType( _in->GetNextToken() );
                }
                else if( stricmp( "forceradarteammatch", option ) ==  0 )
                {
                    m_levelOptions[LevelOptionForceRadarTeamMatch] = 1;
                }
                else if( stricmp( "noradarcontrol", option ) == 0 )
                {
                    m_levelOptions[LevelOptionNoRadarControl] = 1;
                }
                else if( stricmp( "airstrikecapturezones", option ) == 0 )
                {
                    m_levelOptions[LevelOptionAirstrikeCaptureZones] = atoi( _in->GetNextToken() );
                }
                else if( stricmp( "futurewinianteam", option ) == 0 )
                {
                    m_levelOptions[LevelOptionFuturewinianTeam] = atoi( _in->GetNextToken() );
                }
                else if( stricmp( "attackreinforcementoption", option ) == 0 )
                {
                    m_levelOptions[LevelOptionAttackReinforcementMode] = atoi( _in->GetNextToken() );
                }
                else if( stricmp( "defendreinforcementoption", option ) == 0 )
                {
                    m_levelOptions[LevelOptionDefendReinforcementMode] = atoi( _in->GetNextToken() );
                }
            }
        }
        else if( stricmp( "Name", word ) == 0 )
        {
            strcpy( m_levelName, _in->GetRestOfLine() );
        }
        else if( stricmp( "InvalidCrates", word ) == 0 )
        {
            while( _in->TokenAvailable() )
            {
                char *crateType = _in->GetNextToken();
                for( int i = 0; i < Crate::NumCrateRewards; ++i )
                {
                    if( stricmp( crateType, Crate::GetName(i) ) == 0 )
                    {
                        m_invalidCrates[i] = true;
                        break;
                    }
                }
            }
        }
        else if( stricmp( "Author", word ) == 0 )
        {
            strcpy(m_author,_in->GetRestOfLine() );
        }
        else if( stricmp( "Description", word ) == 0 )
        {
            strcpy( m_description, _in->GetRestOfLine() );
        }
        else if( stricmp( "PlayTime", word ) == 0 )
        {
            m_defaultPlayTime = atoi( _in->GetNextToken() );
        }
        else if( stricmp( "Difficulty", word ) == 0 )
        {
            m_difficultyRating = strdup( _in->GetNextToken() );
        }
        else if( stricmp( "AttackingTeams", word ) == 0 )
        {
            while( _in->TokenAvailable() )
            {
                m_attacking[atoi(_in->GetNextToken())] = true;
            }
        }
        else if( stricmp( "CoopMode", word ) == 0 )
        {
            m_coop = true;
        }
        else if( stricmp( "ForceCoop", word) == 0 )
        {
            m_forceCoop = true;
        }
        else if( stricmp( "CoopGroup1Positions", word ) == 0 )
        {
            while( _in->TokenAvailable() )
            {
                m_coopGroup1Positions.PutData( atoi(_in->GetNextToken()) );
            }
        }
        else if( stricmp( "CoopGroup2Positions", word ) == 0 )
        {
            while( _in->TokenAvailable() )
            {
                m_coopGroup2Positions.PutData( atoi(_in->GetNextToken()) );
            }
        }
        else if( stricmp( "OfficialLevel", word ) == 0 )
        {
            m_official = true;
			m_mapId = 0; // ### CRC based method here
        }
    }
}

void LevelFile::ParseBuildings(TextReader *_in, bool _dynamic)
{
	double loadDifficultyFactor = 1.0;
	if (m_levelDifficulty < 0)
		loadDifficultyFactor = 1.0 + (double) g_app->m_difficultyLevel / 5.0;

	while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;
		char *word = _in->GetNextToken();

        if (stricmp("Buildings_EndDefinition", word) == 0)
        {
            return;
        }		

		Building *building = Building::CreateBuilding( word );
        if( building )
        {
            building->Read( _in, _dynamic );

			// Make sure that this building's ID isn't already in use
			int uniqueId = building->m_id.GetUniqueId();
			Building *existingBuilding = GetBuilding(uniqueId);
			if (existingBuilding)
			{
				AppReleaseAssert(0,
					"%s UniqueId was not unique in %s", 
					Building::GetTypeName(existingBuilding->m_type),
					_in->GetFilename());
			}

			// Make sure that it's global if it needs to be
			if (building->m_type == Building::TypeTrunkPort ||
                building->m_type == Building::TypeControlTower ||
                building->m_type == Building::TypeRadarDish ||
                building->m_type == Building::TypeIncubator ||
                building->m_type == Building::TypeFenceSwitch )
			{
                AppReleaseAssert(building->m_isGlobal, "Non-global %s found in %s", 
                                Building::GetTypeName(building->m_type), _in->GetFilename());
			}
            
			// Increase the difficulty by raising the population limits for the opposing forces
			if (building->m_id.GetTeamId() == 1) {
				switch (building->m_type) {
					case Building::TypeSpawnPopulationLock: {
						SpawnPopulationLock *spl = (SpawnPopulationLock *) building;
						spl->m_maxPopulation = int(spl->m_maxPopulation * loadDifficultyFactor);
					}
					break;
					
					case Building::TypeAntHill: {
						AntHill *ah = (AntHill *) building;
						ah->m_numAntsInside = int(ah->m_numAntsInside * loadDifficultyFactor);
					}
					break;
					
					case Building::TypeAISpawnPoint: {
						AISpawnPoint *aisp = (AISpawnPoint *) building;
						aisp->m_period = int(aisp->m_period / loadDifficultyFactor);
					}
					break;
					
					case Building::TypeTriffid: {
						Triffid *t = (Triffid *) building;
						t->m_reloadTime = int(t->m_reloadTime / loadDifficultyFactor);
					}
					break;
				}
			}
			
			m_buildings.PutData(building);
        }
	}
}


void LevelFile::ParseInstantUnits(TextReader *_in)
{
	double loadDifficultyFactor = 1.0;
	if (m_levelDifficulty < 0)
		loadDifficultyFactor = 1.0 + (double) g_app->m_difficultyLevel / 5.0;
		
	while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;
		char *word = _in->GetNextToken();
		
        if (stricmp("InstantUnits_EndDefinition", word) == 0)
        {
            return;
        }		

        int entityType = Entity::GetTypeId( word );
        if (entityType == -1 || Entity::EntityIsBlocked(entityType) )
        {
            continue;
		}

		InstantUnit *iu = new InstantUnit();
		int numCopies = 0;
		iu->m_type = entityType;		

        iu->m_teamId    = atoi(_in->GetNextToken());
		iu->m_posX      = iv_atof(_in->GetNextToken());
		iu->m_posZ      = iv_atof(_in->GetNextToken());
		iu->m_number    = atoi(_in->GetNextToken());
        iu->m_inAUnit   = atoi(_in->GetNextToken()) ? true : false;        
    	iu->m_state     = atoi(_in->GetNextToken());	   
        iu->m_spread    = iv_atof(_in->GetNextToken());
        
        if( _in->TokenAvailable() )
        {
            iu->m_waypointX = iv_atof(_in->GetNextToken());
            iu->m_waypointZ = iv_atof(_in->GetNextToken());
        }

        if( _in->TokenAvailable() )
        {
	        iu->m_routeId = atoi( _in->GetNextToken() );
            if( _in->TokenAvailable() )
            {
                iu->m_routeWaypointId = atoi( _in->GetNextToken() );
                if( _in->TokenAvailable() )
                {
                    iu->m_runAsTask = atoi(_in->GetNextToken()) ? true : false;
                }
            }
        }
		
		// Stop InstantSquaddies bug from crashing Darwinia (for existing bad save files)
		if (iu->m_type == Entity::TypeInsertionSquadie && iu->m_inAUnit == 0)
			continue;
			
		// If the unit is on the red team then make it more of them
		if (iu->m_teamId == 1) {
			iu->m_number = int(iu->m_number * loadDifficultyFactor);
			
			// It doesn't make sense to make overly 
			if (iu->m_type == Entity::TypeCentipede && loadDifficultyFactor > 1) {
				int maxCentipedeLength = 10 + int(2.0 * loadDifficultyFactor);
				int segmentUtilisation = maxCentipedeLength + 7;
				if (iu->m_number > maxCentipedeLength) {
					numCopies = iu->m_number / segmentUtilisation - 1;
					iu->m_number = maxCentipedeLength;
				}
			}
			else 
			{
				if( loadDifficultyFactor > 1.0 )
					iu->m_spread *= iv_pow(1.2, g_app->m_difficultyLevel / 5.0);
			}

		}
			
		m_instantUnits.PutData(iu);
		
		// Create some additional centipedes if necessary
		for (int i = 0; i < numCopies; i++) {
			InstantUnit *copy = new InstantUnit;
			*copy = *iu;
			// Spread them out a bit
			copy->m_posX = iu->m_posX + sfrand(60);
			copy->m_posZ = iu->m_posZ + sfrand(60);
			m_instantUnits.PutData(copy);
		}
	}
}


void LevelFile::ParseLandscapeData(TextReader *_in)
{
	while(_in->ReadLine())
	{
		char *word = _in->GetNextToken();
		char *secondWord = NULL;
		
		if (_in->TokenAvailable()) secondWord = _in->GetNextToken();

        if (stricmp("cellSize", word) == 0 )
        {
            m_landscape.m_cellSize = iv_atof(secondWord);
        }
        else if (stricmp("worldSizeX", word) == 0 )
        {
            m_landscape.m_worldSizeX = atoi(secondWord);
        }
        else if (stricmp("worldSizeZ", word) == 0 )
        {
            m_landscape.m_worldSizeZ = atoi(secondWord);
        }
        else if (stricmp("outsideHeight", word) == 0)
        {
            m_landscape.m_outsideHeight = iv_atof(secondWord);
        }
        else if( stricmp( "maxHeight", word ) == 0 )
        {
            m_landscape.m_maxHeight = atof(secondWord);
        }
        else if (stricmp("landColourFile", word) == 0)
        {
            strcpy( m_landscapeColourFilename, secondWord );
        }
        else if (stricmp("wavesColourFile", word) == 0)
        {
            strcpy( m_wavesColourFilename, secondWord );
        }
        else if (stricmp("waterColourFile", word) == 0)
        {
            strcpy( m_waterColourFilename, secondWord );
        }
        else if (stricmp("landscape_endDefinition", word) == 0)
        {
            return;
        }		
	}
}


void LevelFile::ParseLandscapeTiles(TextReader *_in)
{
    while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;
		char *word = _in->GetNextToken();

		if (stricmp(word, "landscapeTiles_endDefinition") == 0)
		{
			return;
		}

		LandscapeTile *def = new LandscapeTile();
		m_landscape.m_tiles.PutDataAtEnd(def);

		def->m_posX = atoi(word);
		
		word = _in->GetNextToken();
		def->m_posY = (double)iv_atof(word);
		
		word = _in->GetNextToken();
		def->m_posZ = atoi(word);

		word = _in->GetNextToken();
		def->m_size = atoi(word);
		
		word = _in->GetNextToken();
        def->m_fractalDimension = (double)iv_atof(word);

		word = _in->GetNextToken();
        def->m_heightScale = (double)iv_atof(word);

		word = _in->GetNextToken();
        def->m_desiredHeight = (double)iv_atof(word);

		word = _in->GetNextToken();
        def->m_generationMethod = atoi(word);

		word = _in->GetNextToken();
        def->m_randomSeed = atoi(word);
		
		word = _in->GetNextToken();
		def->m_lowlandSmoothingFactor = (double)iv_atof(word);

        word = _in->GetNextToken();
        int guidePower = atoi(word);
        def->GuideGridSetPower( guidePower );

        if( def->m_guideGridPower > 0 )
        {
            word = _in->GetNextToken();
            def->GuideGridFromString( word );
        }
            
	}
}


void LevelFile::ParseLandFlattenAreas(TextReader *_in)
{
	while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;
		char *word = _in->GetNextToken();

        if (stricmp("landFlattenAreas_endDefinition", word) == 0)
        {
            return;
        }		
		
		LandscapeFlattenArea *def = new LandscapeFlattenArea();
		m_landscape.m_flattenAreas.PutDataAtEnd(def);
		
        def->m_centre.x = (double)iv_atof(word);

        word = _in->GetNextToken();
		def->m_centre.y = (double)iv_atof(word);

        word = _in->GetNextToken();
        def->m_centre.z = (double)iv_atof(word);
        
		word = _in->GetNextToken();
        def->m_size = (double)iv_atof(word);

        if( _in->TokenAvailable() )
        {
            def->m_flattenType = atoi(_in->GetNextToken());

            word = _in->GetNextToken();
            def->m_heightThreshold = (double)iv_atof(word);
        }
        else
        {
            def->m_flattenType = LandscapeFlattenArea::FlattenTypeAbsolute;
        }
	}
}


void LevelFile::ParseLights(TextReader *_in)
{
	bool ignoreLights = false;

	if (m_lights.Size() > 0)
	{
		// This function is called first when parsing the level file and 
		// secondly when parsing the map file. We only get here if the
		// level file specified some lights. In which case, these lights
		// are to be used in preference to the map lights. So we need
		// to ignore all the lights we encounter.
		ignoreLights = true;
	}

	while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;
		char *word = _in->GetNextToken();

        if (stricmp("Lights_EndDefinition", word) == 0)
        {
            return;
        }		
		
		if (ignoreLights) continue;

		Light *light = new Light;
		m_lights.PutData(light);

		light->m_front[0] = iv_atof(word);

		word = _in->GetNextToken();
		light->m_front[1] = iv_atof(word);

		word = _in->GetNextToken();
		light->m_front[2] = iv_atof(word);
		light->m_front[3] = 0.0;	// Set to be an infinitely distant light
		light->Normalise();

		word = _in->GetNextToken();
		light->m_colour[0] = iv_atof(word);

		word = _in->GetNextToken();
		light->m_colour[1] = iv_atof(word);

		word = _in->GetNextToken();
		light->m_colour[2] = iv_atof(word);
		light->m_colour[3] = 0.0;
 	}
}


void LevelFile::ParseRoute(TextReader *_in, int _id)
{
	Route *r = new Route(_id);

	while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;
		char *word = _in->GetNextToken(); 

		if (stricmp("end", word) == 0) break;
		AppDebugAssert(isdigit(word[0]));

		int type = atoi(word);
		Vector3 pos;
		int buildingId = -1;

		switch (type)
		{
			case WayPoint::TypeGroundPos:
				pos.x = iv_atof(_in->GetNextToken());
				pos.z = iv_atof(_in->GetNextToken());
				break;
			case WayPoint::Type3DPos:
				pos.x = iv_atof(_in->GetNextToken());
				pos.y = iv_atof(_in->GetNextToken());
				pos.z = iv_atof(_in->GetNextToken());
				break;
			case WayPoint::TypeBuilding:
				buildingId = atoi(_in->GetNextToken());
				break;
		}

		WayPoint *wp = new WayPoint(type, pos);
		if (buildingId != -1)
		{
			wp->m_buildingId = buildingId;
		}
		r->m_wayPoints.PutDataAtEnd(wp);
	}

	m_routes.PutDataAtEnd(r);
}


void LevelFile::ParseRoutes(TextReader *_in)
{
	while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;
		char *word = _in->GetNextToken();

		if (stricmp("Routes_EndDefinition", word) == 0)
		{
			return;
		}

		if (stricmp("Route", word) == 0)
		{
			word = _in->GetNextToken(); 
			int id = atoi(word);
			AppDebugAssert(id >= 0 && id < 10000);
			ParseRoute(_in, id);
		}
	}
}


void LevelFile::ParsePrimaryObjectives(TextReader *_in)
{
	while (_in->ReadLine())
	{
        char *word = _in->GetNextToken();

        if( stricmp("PrimaryObjectives_EndDefinition", word) == 0)
        {
            return;
        }

        GlobalEventCondition *condition = new GlobalEventCondition;
		condition->m_type = condition->GetType(word);
        AppDebugAssert( condition->m_type != -1 );

		switch (condition->m_type)
		{
            case GlobalEventCondition::AlwaysTrue:
            case GlobalEventCondition::NotInLocation:
				AppDebugAssert(false);
                break;
            
			case GlobalEventCondition::BuildingOffline:
			case GlobalEventCondition::BuildingOnline:
			{
				//condition->m_locationId = g_app->m_globalWorld->GetLocationIdFromMapFilename( m_mapFilename );
                condition->m_locationId = g_app->m_globalWorld->GetLocationId( _in->GetNextToken() );
				condition->m_id = atoi( _in->GetNextToken() );
                AppDebugAssert( condition->m_locationId != -1 );
				break;
			}

            case GlobalEventCondition::ResearchOwned:
                condition->m_id = GlobalResearch::GetType( _in->GetNextToken() );
                AppDebugAssert( condition->m_id != -1 );
                break;
                
            case GlobalEventCondition::DebugKey:
                condition->m_id = atoi( _in->GetNextToken() );
				break;
		}

        if( _in->TokenAvailable() )
        {
            char *stringId = _in->GetNextToken();
            condition->SetStringId( stringId );
        }

        if( _in->TokenAvailable() )
        {
            char *cutScene = _in->GetNextToken();
            condition->SetCutScene( cutScene );
        }

        m_primaryObjectives.PutData( condition );
	}
}


void LevelFile::GenerateAutomaticObjectives()
{
	// 
	// Create a NeverTrue objective for cutscene mission files

	if (m_primaryObjectives.Size() == 0)
	{
		GlobalEventCondition *objective = new GlobalEventCondition;
		objective->m_type = GlobalEventCondition::NeverTrue;
		m_primaryObjectives.PutData(objective);
	}

	//
	// Add secondary objectives for trunk ports and research items

	for (int i = 0; i < m_buildings.Size(); ++i)
	{
		Building *building = m_buildings.GetData(i);

		if (building->m_type != Building::TypeResearchItem &&
			building->m_type != Building::TypeTrunkPort)
		{
			continue;	
		}

		// Make sure this building isn't already in the primary objectives list
		bool found = false;
		for (int k = 0; k < m_primaryObjectives.Size(); ++k)
		{
			GlobalEventCondition *primaryObjective = m_primaryObjectives.GetData(k);
			if (primaryObjective->m_id == building->m_id.GetUniqueId())
			{
				found = true;
				break;
			}

            if (primaryObjective->m_type == GlobalEventCondition::ResearchOwned &&
                building->m_type == Building::TypeResearchItem &&
                ((ResearchItem *)building)->m_researchType == primaryObjective->m_id )
            {
                found = true;
                break;
            }
		}
			
		if (!found)
		{
            int locationId = g_app->m_globalWorld->GetLocationIdFromMapFilename(m_mapFilename);

			if (building->m_type == Building::TypeResearchItem)
			{
				ResearchItem *item = (ResearchItem*) GetBuilding(building->m_id.GetUniqueId());
                int currentLevel = g_app->m_globalWorld->m_research->CurrentLevel( item->m_researchType );
                if( currentLevel == 0 /*|| currentLevel < item->m_level*/ )
                {
                    // NOTE : We SHOULD really allow objectives to be created when the current research level
                    // is below that of this ResearchItem - eg we have level 1 and this item is level 2.
                    // However GlobalEventCondition isn't currently able to store the level data, so it
                    // ends up being an auto-completed objective.
				    GlobalEventCondition *condition = new GlobalEventCondition();
				    condition->m_locationId = locationId;
                    condition->m_type = GlobalEventCondition::ResearchOwned;
				    condition->m_id = item->m_researchType;
                    condition->SetStringId( "objective_research" );                                                            
				    m_secondaryObjectives.PutData(condition);
                }
			}
			else if (building->m_type == Building::TypeTrunkPort )
			{
                GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( building->m_id.GetUniqueId(), locationId );
                if( gb && !gb->m_online )
                {
                    //
                    // Is there a Control Tower that can enable this trunk port?
                    bool towerFound = false;
                    for( int c = 0; c < m_buildings.Size(); ++c )
                    {
                        Building *thisBuilding = m_buildings[c];
                        if( thisBuilding->m_type == Building::TypeControlTower &&
                            thisBuilding->GetBuildingLink() == building->m_id.GetUniqueId() )
                        {
                            towerFound = true;
                            break;
                        }
                    }

                    if( towerFound )
                    {
				        GlobalEventCondition *condition = new GlobalEventCondition;
				        condition->m_locationId = locationId;
				        condition->m_type = GlobalEventCondition::BuildingOnline;
				        condition->m_id = building->m_id.GetUniqueId();
                        condition->SetStringId( "objective_capture_trunk" );
				        m_secondaryObjectives.PutData(condition);
                    }
                }
			}
		}
	}
}


void LevelFile::WriteInstantUnits(TextWriter *_out)
{
	_out->printf( "InstantUnits_StartDefinition\n");
	_out->printf( "\t# Type         team    x       z   count  inUnit state   spread  waypointX waypointZ  routeId runAsTask\n");
	_out->printf( "\t# ==================================================================================\n");

	for (int i = 0; i < m_instantUnits.Size(); i++)
	{
		InstantUnit *iu = m_instantUnits.GetData(i);
		_out->printf( "\t%-15s %2d %7.1f %7.1f %6d %4d %7d %7.1f %7.1f %7.1f %4d %4d %4d\n",
			Entity::GetTypeName(iu->m_type), iu->m_teamId, iu->m_posX, iu->m_posZ, 
            iu->m_number, iu->m_inAUnit, iu->m_state, iu->m_spread, iu->m_waypointX, iu->m_waypointZ, iu->m_routeId, iu->m_routeWaypointId, iu->m_runAsTask );        
	}
	_out->printf( "InstantUnits_EndDefinition\n\n");
}


void LevelFile::WriteLights(TextWriter *_out)
{
	_out->printf( "Lights_StartDefinition\n");
	_out->printf( "\t# x      y      z        r      g      b\n");
	_out->printf( "\t# =========================================\n");

    if( g_app->m_location )
    {
	    for (int i = 0; i < g_app->m_location->m_lights.Size(); ++i)
	    {
		    Light *light = g_app->m_location->m_lights.GetData(i);
		    _out->printf( "\t%6.2f %6.2f %6.2f   %6.2f %6.2f %6.2f\n",
				    light->m_front[0], light->m_front[1], light->m_front[2],
				    light->m_colour[0], light->m_colour[1], light->m_colour[2]);
        }
    }
    
	_out->printf( "Lights_EndDefinition\n\n");
}


void LevelFile::WriteCameraMounts(TextWriter *_out)
{
	_out->printf( "CameraMounts_StartDefinition\n");
	_out->printf( "\t# Name	          Pos                   Front          Up\n");
	_out->printf( "\t# =========================================================================\n");

	for (int i = 0; i < m_cameraMounts.Size(); i++)
	{
		CameraMount *cmnt = m_cameraMounts.GetData(i);
		_out->printf( "\t%-15s %7.2f %7.2f %7.2f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f\n",
			cmnt->m_name,
			cmnt->m_pos.x, cmnt->m_pos.y, cmnt->m_pos.z,
			cmnt->m_front.x, cmnt->m_front.y, cmnt->m_front.z,
			cmnt->m_up.x, cmnt->m_up.y, cmnt->m_up.z);
	}

	_out->printf( "CameraMounts_EndDefinition\n\n");
}


void LevelFile::WriteCameraAnims(TextWriter *_out)
{
	_out->printf( "CameraAnimations_StartDefinition\n");

	for (int i = 0; i < m_cameraAnimations.Size(); i++)
	{
		CameraAnimation *anim = m_cameraAnimations.GetData(i);
		_out->printf( "\t%s\n", anim->m_name);
		
		for (int j = 0; j < anim->m_nodes.Size(); ++j)
		{
			CamAnimNode *node = anim->m_nodes[j];
			char const *camModeName = CamAnimNode::GetTransitModeName(node->m_transitionMode);
			_out->printf( "\t\t%-8s %-15s %.2f\n", camModeName, node->m_mountName, node->m_duration);
		}
		_out->printf( "\t\tEnd\n");
	}

	_out->printf( "CameraAnimations_EndDefinition\n\n");
}


void LevelFile::WriteBuildings(TextWriter *_out, bool _dynamic)
{
	_out->printf( "Buildings_StartDefinition\n");
	_out->printf( "\t# Type              id      x       z       tm      rx      rz      isGlobal\n");
	_out->printf( "\t# ==========================================================================\n");

	for (int i = 0; i < m_buildings.Size(); i++)
	{
		Building *building = m_buildings.GetData(i);
		if (building->m_dynamic == _dynamic)
		{
			building->Write( _out );
			_out->printf( "\n" );
		}
	}
	_out->printf( "Buildings_EndDefinition\n\n");
}


void LevelFile::WriteLandscapeData(TextWriter *_out)
{
	_out->printf( "Landscape_StartDefinition\n");
	_out->printf( "\tworldSizeX %d\n", m_landscape.m_worldSizeX);
	_out->printf( "\tworldSizeZ %d\n", m_landscape.m_worldSizeZ);
	_out->printf( "\tcellSize %.2f\n", m_landscape.m_cellSize);
	_out->printf( "\toutsideHeight %.2f\n", m_landscape.m_outsideHeight);
    _out->printf( "\tmaxHeight %.2f\n", m_landscape.m_maxHeight );
    _out->printf( "\tlandColourFile %s\n", m_landscapeColourFilename );
    _out->printf( "\twavesColourFile %s\n", m_wavesColourFilename );
    _out->printf( "\twaterColourFile %s\n", m_waterColourFilename );
	_out->printf( "Landscape_EndDefinition\n\n");
}


void LevelFile::WriteLandscapeTiles(TextWriter *_out)
{
	_out->printf( "LandscapeTiles_StartDefinition\n");
	_out->printf( "\t#                            frac  height desired gen         lowland\n");
	_out->printf( "\t# x       y       z    size   dim  scale  height  method seed smooth  guideGrid\n");
	_out->printf( "\t# =============================================================================\n");
	    
    for (int i = 0; i < m_landscape.m_tiles.Size(); ++i)
	{
		LandscapeTile *_def = m_landscape.m_tiles.GetData(i);
		_out->printf( "\t%6d %6.2f %6d ", _def->m_posX, _def->m_posY, _def->m_posZ);
		_out->printf( "%6d ", _def->m_size);
		_out->printf( "%5.2f ", _def->m_fractalDimension);
		_out->printf( "%6.2f ", _def->m_heightScale);
		_out->printf( "%6.0f ", _def->m_desiredHeight);
		_out->printf( "%6d ", _def->m_generationMethod);
		_out->printf( "%6d ", _def->m_randomSeed);
		_out->printf( "%6.2f", _def->m_lowlandSmoothingFactor);
        _out->printf( "%6d", _def->m_guideGridPower);

        if( _def->m_guideGridPower > 0 ) _out->printf( "   %s", _def->GuideGridToString() );        
        
        _out->printf( "\n");
	}
	_out->printf( "LandscapeTiles_EndDefinition\n\n");
}


void LevelFile::WriteLandFlattenAreas(TextWriter *_out)
{
	_out->printf( "LandFlattenAreas_StartDefinition\n");
	_out->printf( "\t# x      y       z      size\n");
	_out->printf( "\t# ==========================\n");
	for (int i = 0; i < m_landscape.m_flattenAreas.Size(); ++i)
	{
		LandscapeFlattenArea *area = m_landscape.m_flattenAreas.GetData(i);
		_out->printf( "\t%6.1f %6.1f %6.1f %6.1f %6d %6.1f\n", area->m_centre.x, 
            area->m_centre.y, area->m_centre.z, area->m_size, area->m_flattenType, area->m_heightThreshold);
	}
	_out->printf( "LandFlattenAreas_EndDefinition\n\n");
}


void LevelFile::WriteRoutes(TextWriter *_out)
{

	_out->printf("Routes_StartDefinition\n");
	for (int i = 0; i < m_routes.Size(); ++i)
	{
		if (!m_routes.ValidIndex(i)) continue;
		
		Route *r = m_routes.GetData(i);

		_out->printf("\tRoute %d\n", r->m_id);
		
		for (int j = 0; j < r->m_wayPoints.Size(); ++j)
		{
			WayPoint *wp = r->m_wayPoints.GetData(j);
			Vector3 pos = wp->GetPos();
			if (wp->m_type == WayPoint::Type3DPos)
			{
				_out->printf( "\t\t%-3d %6.2f %6.2f %6.2f\n", wp->m_type, pos.x, pos.y, pos.z);
			}
			else if (wp->m_type == WayPoint::TypeGroundPos)
			{
				_out->printf( "\t\t%-3d %6.2f %6.2f\n", wp->m_type, pos.x, pos.z);
			}
			else if (wp->m_type == WayPoint::TypeBuilding)
			{
				_out->printf( "\t\t%-3d %6d\n", wp->m_type, wp->m_buildingId);
			}
		}

		_out->printf( "\t\tEnd\n");
	}
	_out->printf( "Routes_EndDefinition\n\n");
}


void LevelFile::WritePrimaryObjectives(TextWriter *_out)
{
	_out->printf( "PrimaryObjectives_StartDefinition\n");

	for (int i = 0; i < m_primaryObjectives.Size(); ++i)
	{
		GlobalEventCondition *gec = m_primaryObjectives[i];
		//_out->printf( "\t%s:%d", gec->GetTypeName(gec->m_type), gec->m_id);
        _out->printf( "\t" );
        gec->Save( _out );

        if( gec->m_stringId ) _out->printf( "\t%s", gec->m_stringId );
        if( gec->m_cutScene ) _out->printf( "\t%s", gec->m_cutScene );

        _out->printf( "\n" );
	}

	_out->printf( "PrimaryObjectives_EndDefinition\n");
}



// **************
// Public Methods
// **************

LevelFile::LevelFile()
{
	sprintf(m_landscapeColourFilename,  "landscape_default.bmp" );
    sprintf(m_wavesColourFilename,      "waves_default.bmp" );
    sprintf(m_waterColourFilename,      "water_default.bmp" );
	m_levelDifficulty = -1;
    m_populationCap = -1;
    m_defenderPopulationCap = -1;
    m_numPlayers = -1;
    m_defaultPlayTime = -1;
    m_coop = false;
    m_forceCoop = false;
    m_official = false;
	m_mapId = -1;
	m_leaderboardLevelTime = 0.0;
	m_lastLeaderboardLevelTime = 9999999.0;
    m_gameTypes = -1;

   // m_author = NULL;
    //m_description = NULL;
    m_difficultyRating = NULL;

    memset( m_levelOptions, -1, sizeof(int) * NumLevelOptions );
    memset( m_invalidCrates, false, sizeof(bool) * Crate::NumCrateRewards );
    memset( m_attacking, false, sizeof(bool) * NUM_TEAMS );
    memset( m_effects, false, sizeof(m_effects) );
    strcpy( m_levelName, "NewLevel" );
    strcpy( m_description, "LevelDescription" );
    strcpy( m_author, "Author" );
    strcpy( m_thumbnailFilename, "default.bmp" );

    m_levelOptions[LevelFile::LevelOptionNoHandicap] = 0;
    m_levelOptions[LevelFile::LevelOptionInstantSpawnPointCapture] = 0;
    m_levelOptions[LevelFile::LevelOptionNoGunTurrets] = 0;
    m_levelOptions[LevelFile::LevelOptionForceRadarTeamMatch] = 0;
    m_levelOptions[LevelFile::LevelOptionNoRadarControl] = 0;
}

LevelFile::LevelFile(char const *_missionFilename, char const *_mapFilename, bool _textFileLevel )
{
	sprintf(m_mapFilename,              "%s", _mapFilename);
    sprintf(m_missionFilename,          "%s", _missionFilename );
	sprintf(m_landscapeColourFilename,  "landscape_default.bmp" );
    sprintf(m_wavesColourFilename,      "waves_default.bmp" );
    sprintf(m_waterColourFilename,      "water_default.bmp" );
	m_levelDifficulty = -1;
    m_populationCap = -1;
    m_defenderPopulationCap = -1;
    m_numPlayers = -1;
    m_defaultPlayTime = -1;
    m_coop = false;
    m_forceCoop = false;
    m_official = false;
	m_mapId = -1;
	m_leaderboardLevelTime = 0.0;
	m_lastLeaderboardLevelTime = 9999999.0;
    m_gameTypes = -1;

    //m_author = NULL;
    //m_description = NULL;
    m_difficultyRating = NULL;

    memset( m_levelOptions, -1, sizeof(int) * NumLevelOptions );
    memset( m_invalidCrates, false, sizeof(bool) * Crate::NumCrateRewards );
    memset( m_attacking, false, sizeof(bool) * NUM_TEAMS );
    memset( m_effects, false, sizeof(m_effects) );
    strcpy( m_levelName, "NewLevel" );
    strcpy( m_description, "LevelDescription" );
    strcpy( m_author, "Author" );
    strcpy( m_thumbnailFilename, "default.bmp" );

    m_levelOptions[LevelFile::LevelOptionNoHandicap] = 0;
    m_levelOptions[LevelFile::LevelOptionInstantSpawnPointCapture] = 0;
    m_levelOptions[LevelFile::LevelOptionNoGunTurrets] = 0;
    m_levelOptions[LevelFile::LevelOptionForceRadarTeamMatch] = 0;
    m_levelOptions[LevelFile::LevelOptionNoRadarControl] = 0;
	
	// Make sure that the current game difficulty setting
	// is consistent with the preferences (it can become inconsistent
	// when a level is loaded that was saved with a different difficulty
	// level to what the preferences say).	
	g_app->UpdateDifficultyFromPreferences();
	
    if( _textFileLevel )
    {
	    if (stricmp(_missionFilename, "null") != 0)
        {
            ParseMissionFile(m_missionFilename);
        }
   	    ParseMapFile(m_mapFilename);
    }
    else
    {
        ParseMWMFile(m_mapFilename);
    }

	GenerateAutomaticObjectives();
}


LevelFile::~LevelFile()
{
	m_cameraMounts.EmptyAndDelete();
	m_cameraAnimations.EmptyAndDelete();
    m_buildings.EmptyAndDelete();
    m_instantUnits.EmptyAndDelete();
    m_lights.EmptyAndDelete();
    m_routes.EmptyAndDelete();
	m_runningPrograms.EmptyAndDelete();
	m_primaryObjectives.EmptyAndDelete();
	m_secondaryObjectives.EmptyAndDelete();
}

void LevelFile::Save()
{
    TextWriter *out = NULL;
    char fullFilename[256];

    if (strstr(m_missionFilename, "null") == NULL)
	{
        SaveMissionFile(m_missionFilename);
    }

	// Write the map file
	if (strstr(m_mapFilename, "null") == NULL)
	{
#ifdef TARGET_DEBUG
        sprintf( fullFilename, "levels/%s", m_mapFilename );     
	    out = g_app->m_resource->GetTextFileWriter(fullFilename, false);
#else
        sprintf( fullFilename, "%s%s", g_app->GetMapDirectory(), m_mapFilename );
        out = new TextFileWriter( fullFilename, false, false );
#endif
        SaveMapFile(out);
        delete out;
    }
}

void LevelFile::Save(TextWriter *_out)
{
	// Write the map file
	if (strstr(m_mapFilename, "null") == NULL)
	{
        SaveMapFile(_out);
    }
}


void LevelFile::SaveMapFile(TextWriter *_out)
{
    if( g_app->m_multiwinia )
    {
        // options should be at the top to reduce the amount of time taken for the menu to compile the level list
        WriteMultiwiniaOptions(_out);
    }

	WriteLandscapeData(_out);
	WriteLandscapeTiles(_out);
	WriteLandFlattenAreas(_out);
	WriteLights(_out);
	WriteBuildings(_out, false);

    if( g_app->m_multiwinia )
    {
	    WriteCameraMounts(_out);
	    //WriteCameraAnims(_out);
	    //WriteBuildings(_out, true);
	    WriteInstantUnits(_out);    	
	    //WritePrimaryObjectives(_out);
    }
}


void LevelFile::SaveMissionFile(char const *_filename)
{
    TextFileWriter *out = NULL;
    char fullFilename[256];

    if( !g_app->m_editing )
    {
        sprintf( fullFilename, "%susers/%s/%s", g_app->GetProfileDirectory(), g_app->m_userProfileName, _filename );        

#if defined(TARGET_DEBUG) 
        out = new TextFileWriter( fullFilename, false );
#else
        out = new TextFileWriter( fullFilename, true );
#endif
    }

    if( !out )
    {
        sprintf( fullFilename, "levels/%s", _filename );
        out = g_app->m_resource->GetTextFileWriter( fullFilename, false );
    }

	WriteDifficulty(out);
	WriteCameraMounts(out);
	WriteCameraAnims(out);
	WriteBuildings(out, true);
	WriteInstantUnits(out);
	WriteRoutes(out);
	WritePrimaryObjectives(out);
    WriteRunningPrograms(out);
	WriteLeaderboardStats(out);
	delete out;
}


Building *LevelFile::GetBuilding( int _id )
{
    for( int i = 0; i < m_buildings.Size(); ++i )
    {
        Building *building = m_buildings.GetData(i);
        if( building->m_id.GetUniqueId() == _id )
        {
            return building;
        }
    }
    return NULL;
}


CameraMount *LevelFile::GetCameraMount(char const *_name)
{
    for( int i = 0; i < m_cameraMounts.Size(); ++i )
    {
        CameraMount *mount = m_cameraMounts.GetData(i);
        if( stricmp(mount->m_name, _name) == 0)
        {
            return mount;
        }
    }
    return NULL;
}


int LevelFile::GetCameraAnimId(char const *_name)
{
    for( int i = 0; i < m_cameraAnimations.Size(); ++i )
    {
        CameraAnimation *anim = m_cameraAnimations.GetData(i);
        if( stricmp(anim->m_name, _name) == 0)
        {
            return i;
        }
    }
    return -1;
}


void LevelFile::RemoveBuilding( int _id )
{
    for( int i = 0; i < m_buildings.Size(); ++i )
    {
        if( m_buildings.ValidIndex(i) )
        {
            Building *building = m_buildings.GetData(i);
            if( building->m_id.GetUniqueId() == _id )
            {
                m_buildings.RemoveData(i);
                delete building;
                break;
            }
        }
    }
}


int LevelFile::GenerateNewRouteId()
{
	for (int i = 0; i < m_routes.Size(); ++i)
	{
		bool idNotUsed = true;
		for (int j = 0; j < m_routes.Size(); ++j)
		{
			Route *r = m_routes.GetData(j);
			if (i == r->m_id)
			{
				idNotUsed = false;
				break;
			}
		}

		if (idNotUsed)
		{
			return i;
		}
	}

	return m_routes.Size();
}


Route *LevelFile::GetRoute(int _id)
{
	int size = m_routes.Size();
	for (int i = 0; i < size; ++i)
	{
		Route *route = m_routes.GetData(i);
		if (route->m_id == _id)
		{
			return route;
		}
	}

	return NULL;
}


void LevelFile::GenerateInstantUnits()
{
    m_instantUnits.EmptyAndDelete();

    //
    // Record all the full size UNITS that exist

    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        Team *team = g_app->m_location->m_teams[t];
        if( team->m_teamType == TeamTypeCPU )
        {
            for( int u = 0; u < team->m_units.Size(); ++u )
            {
                if( team->m_units.ValidIndex(u) )
                {
                    Unit *unit = team->m_units[u];

                    Vector3 centrePos;
                    double roamRange = 0;
                    int numFound = 0;
                    for( int i = 0; i < unit->m_entities.Size(); ++i )
                    {
                        if( unit->m_entities.ValidIndex(i) )
                        {
                            Entity *entity = unit->m_entities[i];
                            centrePos += entity->m_spawnPoint;
                            roamRange += entity->m_roamRange;
                            numFound++;
                        }
                    }

                    centrePos /= (double) numFound;
                    roamRange /= (double) numFound;

                    InstantUnit *instant = new InstantUnit();                    
                    instant->m_type = unit->m_troopType;
                    instant->m_teamId = unit->m_teamId;
                    instant->m_posX = centrePos.x;
                    instant->m_posZ = centrePos.z;
                    instant->m_number = numFound;
                    instant->m_inAUnit = true;     
                    instant->m_spread = roamRange;
                    instant->m_routeId = unit->m_routeId;
                    instant->m_routeWaypointId = unit->m_routeWayPointId;
                    m_instantUnits.PutData( instant );
                }
            }
        }
    }

    
    //
    // Record all other entities that exist
    // If they are in transit in a teleport, ignore them
    // (dealt with later)

    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        Team *team = g_app->m_location->m_teams[t];
        if( team->m_teamType == TeamTypeCPU )
        {
            for( int i = 0; i < team->m_others.Size(); ++i )
            {
                if( team->m_others.ValidIndex(i) )
                {
                    Entity *entity = team->m_others[i];
                    if( entity->m_enabled )
                    {
                        bool insideSpawnArea = ( entity->m_pos - entity->m_spawnPoint ).Mag() < entity->m_roamRange;

                        InstantUnit *unit = new InstantUnit();
                        unit->m_type = entity->m_type;
                        unit->m_teamId = t;
                        unit->m_posX = insideSpawnArea ? entity->m_spawnPoint.x : entity->m_pos.x;
                        unit->m_posZ = insideSpawnArea ? entity->m_spawnPoint.z : entity->m_pos.z;
                        unit->m_spread = insideSpawnArea ? entity->m_roamRange : 0;
                        unit->m_number = 1;
                        unit->m_inAUnit = false;
                        unit->m_routeId = entity->m_routeId;
                        unit->m_routeWaypointId = entity->m_routeWayPointId;

                        if( entity->m_type == Entity::TypeDarwinian )
                        {
                            Darwinian *darwinian = (Darwinian *) entity;
                            unit->m_posX = darwinian->m_pos.x;
                            unit->m_posZ = darwinian->m_pos.z;
                            unit->m_waypointX = darwinian->m_wayPoint.x;
                            unit->m_waypointZ = darwinian->m_wayPoint.z; 
                            unit->m_spread = 0.0; // Darwinians should be placed exactly where they were when the game was saved
                            if( darwinian->m_state == Darwinian::StateFollowingOrders )
                            {
                                unit->m_state = Darwinian::StateFollowingOrders;
                            }
                        }

                        m_instantUnits.PutData( unit );
                    }
                }
            }
        }
        if( team->m_teamType == TeamTypeLocalPlayer )
        {
            for( int i = 0; i < team->m_others.Size(); ++i )
            {
                if( team->m_others.ValidIndex(i) )
                {
                    Entity *entity = team->m_others[i];
                    if( entity->m_type == Entity::TypeOfficer &&
                        entity->m_enabled )
                    {                      
                        Officer *officer = (Officer *) entity;
                        InstantUnit *unit = new InstantUnit();
                        unit->m_type = entity->m_type;
                        unit->m_teamId = t;
                        unit->m_posX = entity->m_pos.x;
                        unit->m_posZ = entity->m_pos.z;
                        unit->m_spread = 0;
                        unit->m_number = 1;
                        unit->m_inAUnit = false;
                        unit->m_state = officer->m_orders;
                        unit->m_waypointX = officer->m_orderPosition.x;
                        unit->m_waypointZ = officer->m_orderPosition.z;
                        unit->m_routeId = officer->m_routeId;
                        unit->m_routeWaypointId = officer->m_routeWayPointId;
                        m_instantUnits.PutData( unit );
                    }
                    else if( entity->m_type == Entity::TypeArmour )
                    {
                        bool taskControlled = false;
                        for( int i = 0; i < g_app->m_location->m_teams[ t ]->m_taskManager->m_tasks.Size(); ++i )
                        {
                            Task *task = g_app->m_location->m_teams[ t ]->m_taskManager->m_tasks[i];
                            if( task->m_type == GlobalResearch::TypeArmour &&
                                task->m_objId == entity->m_id )
                            {
                                taskControlled = true;
                                break;
                            }
                        }
                        if( !taskControlled )
                        {
                            Armour *armour = (Armour *) entity;
                            InstantUnit *unit = new InstantUnit();
                            unit->m_type = Entity::TypeArmour;
                            unit->m_teamId = t;
                            unit->m_posX = armour->m_pos.x;
                            unit->m_posZ = armour->m_pos.z;
                            unit->m_spread = 0;
                            unit->m_number = 1;
                            unit->m_inAUnit = false;
                            unit->m_state = armour->m_state;
                            unit->m_waypointX = armour->m_wayPoint.x;
                            unit->m_waypointZ = armour->m_wayPoint.z;
                            unit->m_routeId = armour->m_routeId;
                            unit->m_routeWaypointId = armour->m_routeWayPointId;
                            m_instantUnits.PutData( unit );
                        }
                    }
                }
            }
        }
    }


    //
    // Record all entities in transit in a Radar Dish beam

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building && building->m_type == Building::TypeRadarDish )
            {
                RadarDish *dish = (RadarDish *) building;
                Vector3 exitPos, exitFront;
                dish->GetExit( exitPos, exitFront );

                for( int e = 0; e < dish->m_inTransit.Size(); ++e )
                {
                    WorldObjectId id = *dish->m_inTransit.GetPointer(e);
                    Entity *entity = g_app->m_location->GetEntity( id );
					
					if( entity == NULL )
						continue;
						
					if( entity->m_type == Entity::TypeInsertionSquadie ) 
					{
						// InsertionSquaddies are running programs and will be saved there, so no
						// need to create an InstantUnit for them. However, we do need to adjust the
						// position of the squaddies so that they are not dunked in the water when revived
						entity->m_pos.x = exitPos.x;
						entity->m_pos.z = exitPos.z;
						entity->m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(entity->m_pos.x, entity->m_pos.z) + 0.1;
					}
                    else 
                    {
                        InstantUnit *unit = new InstantUnit();
                        unit->m_type = entity->m_type;
                        unit->m_teamId = id.GetTeamId();
                        unit->m_posX = exitPos.x;
                        unit->m_posZ = exitPos.z;
                        unit->m_spread = 50;
                        unit->m_number = 1;
                        unit->m_inAUnit = false;
                        unit->m_state = 0;
                        unit->m_routeId = entity->m_routeId;
                        unit->m_routeWaypointId = entity->m_routeWayPointId;
                        //unit->m_waypointX = officer->m_orderPosition.x;
                        //unit->m_waypointZ = officer->m_orderPosition.z;
                        m_instantUnits.PutData( unit );
                    }
                }
            }
        }
    }
        
}


void LevelFile::GenerateDynamicBuildings()
{
    //
    // Update buildings if they are dynamic
    // Remove all dynamic buildings from the list
    // that aren't on the level anymore

    for( int i = 0; i < m_buildings.Size(); ++i )
    {
        Building *building = m_buildings[i];
        if( building && building->m_dynamic )
        {
            Building *locBuilding = g_app->m_location->GetBuilding( building->m_id.GetUniqueId() );
            if( !locBuilding )
            {                
                m_buildings.RemoveData(i);
                delete building;
                --i;
            }
            else
            {
                if( building->m_type == Building::TypeAntHill )
                {
                    ((AntHill *) building)->m_numAntsInside = ((AntHill *)locBuilding)->m_numAntsInside;
                }
                else if( building->m_type == Building::TypeIncubator )
                {
                    ((Incubator *) building)->m_numStartingSpirits = ((Incubator *)locBuilding)->NumSpiritsInside();
                }
                else if( building->m_type == Building::TypeEscapeRocket )
                {
                    ((EscapeRocket *) building)->m_fuel = ((EscapeRocket *)locBuilding)->m_fuel;
                    ((EscapeRocket *) building)->m_passengers = ((EscapeRocket *)locBuilding)->m_passengers;
                    ((EscapeRocket *) building)->m_spawnCompleted = ((EscapeRocket *)locBuilding)->m_spawnCompleted;
                }
                else if( building->m_type == Building::TypeFenceSwitch )
                {
                    ((FenceSwitch *) building)->m_locked = ((FenceSwitch *) locBuilding)->m_locked;
                    ((FenceSwitch *) building)->m_switchValue = ((FenceSwitch *) locBuilding)->m_switchValue;
                }
                else if( building->m_type == Building::TypeLaserFence )
                {
                    ((LaserFence *) building)->m_mode = ((LaserFence *) locBuilding)->m_mode;
                }
                else if( building->m_type == Building::TypeDynamicHub )
                {
                    ((DynamicHub *) building)->m_currentScore = ((DynamicHub *) locBuilding)->m_currentScore;
                }
                else if( building->m_type == Building::TypeDynamicNode )
                {
                    ((DynamicNode *) building)->m_scoreSupplied = ((DynamicNode *) locBuilding)->m_scoreSupplied;
                }
                else if( building->m_type == Building::TypeAISpawnPoint )
                {
                    ((AISpawnPoint *) building)->m_spawnLimit = ((AISpawnPoint *) locBuilding)->m_spawnLimit;
                }
            }
        }
    }


    //
    // Search for new dynamic buildings on the level

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building && building->m_dynamic )
            {
                Building *levelFileBuilding = GetBuilding( building->m_id.GetUniqueId() );
                if( !levelFileBuilding )
                {
                    Building *newBuilding = Building::CreateBuilding( building->m_type );
                    newBuilding->m_id = building->m_id;
                    newBuilding->m_pos = building->m_pos;
                    newBuilding->m_front = building->m_front;
                    newBuilding->m_type = building->m_type;
                    newBuilding->m_dynamic = building->m_dynamic;
                    newBuilding->m_isGlobal = building->m_isGlobal;
                    m_buildings.PutData( newBuilding );
                }
            }
        }
    }
}


void LevelFile::ParseRunningPrograms(TextReader *_in)
{
	while (_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;
        char *word = _in->GetNextToken();

        if( stricmp("RunningPrograms_EndDefinition", word) == 0)
        {
            return;
        }

        RunningProgram *program = new RunningProgram;
        program->m_type         = Entity::GetTypeId( word );
        program->m_count        = atoi( _in->GetNextToken() );
        program->m_state        = atoi( _in->GetNextToken() );
        program->m_data         = atoi( _in->GetNextToken() );
        program->m_waypointX    = iv_atof( _in->GetNextToken() );
        program->m_waypointZ    = iv_atof( _in->GetNextToken() );

        for( int i = 0; i < program->m_count; ++i )
        {
            program->m_positionX[i] = iv_atof( _in->GetNextToken() );
            program->m_positionZ[i] = iv_atof( _in->GetNextToken() );
            program->m_health[i]    = atoi( _in->GetNextToken() );
        }

        m_runningPrograms.PutData( program );
    }
}


void LevelFile::WriteRunningPrograms(TextWriter *_out)
{
    if( !g_app->m_editing )
    {
        Team *team = g_app->m_location->GetMyTeam();

	    _out->printf( "\nRunningPrograms_StartDefinition\n");
	    //_out->printf( "\t# x      y       z      size\n");
	    _out->printf( "\t# ==========================\n");


        //
        // Engineer     count   state   numSpirits  waypointX waypointZ    (positionX positionZ health)
        // Squaddie     count   state   weaponType  waypointX waypointZ    (positionX positionZ health)
        
        for( int t = 0; t < team->m_taskManager->m_tasks.Size(); ++t )
        {
            Task *task = team->m_taskManager->m_tasks[t];
            if( task->m_state == Task::StateRunning )
            {
                if( task->m_type == GlobalResearch::TypeEngineer )
                {
                    Engineer *engineer = (Engineer *) g_app->m_location->GetEntitySafe( task->m_objId, Entity::TypeEngineer );
                    if( engineer )
                    {
                        _out->printf( "\t%-15s %6d %6d %6d %8.2f %8.2f %8.2f %8.2f %d\n", 
                                            Entity::GetTypeName(Entity::TypeEngineer),
                                            1,
                                            engineer->m_state,
                                            engineer->GetNumSpirits(),
                                            engineer->m_wayPoint.x, engineer->m_wayPoint.z,
                                            engineer->m_pos.x, engineer->m_pos.z, 
                                            engineer->m_stats[Entity::StatHealth] );
                    }       
                }
                                
                if( task->m_type == GlobalResearch::TypeSquad )
                {
                    InsertionSquad *squad = (InsertionSquad *) g_app->m_location->GetUnit( task->m_objId );
                    if( squad && squad->m_troopType == Entity::TypeInsertionSquadie )
                    {
                        _out->printf( "\t%-15s %6d %6d %6d %8.2 %8.2",
                                            Entity::GetTypeName(Entity::TypeInsertionSquadie),
                                            squad->m_entities.NumUsed(),
                                            0, 
                                            squad->m_weaponType,
                                            squad->GetWayPoint().x, squad->GetWayPoint().z );

                        for( int e = 0; e < squad->m_entities.Size(); ++e )
                        {
                            if( squad->m_entities.ValidIndex(e) )
                            {
                                Entity *entity = squad->m_entities[e];
								
                                _out->printf( " %8.2 %8.2 %6d", 
                                                    entity->m_pos.x, 
                                                    entity->m_pos.z, 
                                                    entity->m_stats[Entity::StatHealth] );
                            }
                        }

                        _out->printf( "\n" );
                    }
                }
            }
        }

	    _out->printf( "RunningPrograms_EndDefinition\n");
    }
}


void LevelFile::ParseDifficulty(TextReader *_in)
{
	m_levelDifficulty = -1;
	while (_in->ReadLine()) {
		if (!_in->TokenAvailable()) continue;
		char *word = _in->GetNextToken();

		if (stricmp("Difficulty_EndDefinition", word) == 0) return;
		else if (stricmp(word, "CreatedAsDifficulty") == 0) 
		{
			// The difficulty setting is 1-based in the file, but 0-based internally
			m_levelDifficulty = atoi( _in->GetNextToken() ) - 1;
			if( m_levelDifficulty < 0 )	m_levelDifficulty = 0;
		}
	}
}

void LevelFile::WriteDifficulty(TextWriter *_out)
{
	// When we write the difficulty setting to a file it should be 1-based 
	// (internally it is 0 based).
	_out->printf("Difficulty_StartDefinition\n");
	_out->printf("\tCreatedAsDifficulty %d\n", m_levelDifficulty + 1);	
	_out->printf("Difficulty_EndDefinition\n\n");
}

void LevelFile::WriteLeaderboardStats(TextWriter *_out )
{
	if( !g_app->IsSinglePlayer() )
	{
		return;
	}

	int index = g_app->m_requestedLocationId;
	if( index == 12 )
	{
		index = 0;
	}
	else
	{
		index--;
	}

	bool completedAllPrimaryObjectives = true;
	for( int i = 0; i < m_primaryObjectives.Size(); i++ )
	{
		GlobalEventCondition* gec = g_app->m_location->m_levelFile->m_primaryObjectives.GetData(i);
		if( gec )
		{
			completedAllPrimaryObjectives &= gec->Evaluate();
		}
	}

	unsigned int maxTime = 3599999;

	AppDebugOut("%sCompleted all primary objective (time %u)\n", 
		completedAllPrimaryObjectives ? "" : "!",
		completedAllPrimaryObjectives ? 
			(unsigned int)g_app->m_location->m_levelFile->m_leaderboardLevelTime :
			maxTime);

	unsigned int time = completedAllPrimaryObjectives ? 
			g_app->m_location->m_levelFile->m_leaderboardLevelTime :
			maxTime;

	g_app->m_globalWorld->m_spLevelTimes[index] = min(time,g_app->m_globalWorld->m_spLevelTimes[index]) ;

	AppDebugOut("LeaderboardStats_StartDefinition\n");
	AppDebugOut("\tTime %u\n", g_app->m_globalWorld->m_spLevelTimes[index] );
	AppDebugOut("\tKills %d\n", g_app->m_location->m_teams[2]->m_teamKills);
	AppDebugOut("\tSurvivors %d\n", g_app->m_location->m_teams[0]->m_others.NumUsed());

	_out->printf("LeaderboardStats_StartDefinition\n");
	_out->printf("\tTime %f\n", g_app->m_globalWorld->m_spLevelTimes[index] );
	_out->printf("\tKills %d\n", g_app->m_location->m_teams[2]->m_teamKills);
	_out->printf("\tSurvivors %d\n", g_app->m_location->m_teams[0]->m_others.NumUsed());
	_out->printf("LeaderboardStats_EndDefinition\n");

	AppDebugOut("LeaderboardStats_EndDefinition\n");
}

void LevelFile::WriteMultiwiniaOptions(TextWriter *_out )
{
    _out->printf("MultiwiniaOptions_StartDefinition\n");
    
    if( strlen( m_levelName ) > 0 )
    {
        _out->printf( "\tName\t\t%s\n", m_levelName );
    }

    if( m_gameTypes != -1 )
    {
        _out->printf( "\tGameTypes\t");
        _out->printf("%s ", g_app->m_multiwinia->s_gameBlueprints[m_gameTypes]->m_name );
        _out->printf("\n");
    }
  /*  else
    {
        _out->printf("\tGameTypes\tKingOfTheHill\tSkirmish\n");
    }*/

    _out->printf("\tPopulationCap\t%d\n", m_populationCap );
    _out->printf("\tDefenderPopulationCap\t%d\n", m_defenderPopulationCap );
    _out->printf("\tNumPlayers\t%d\n", m_numPlayers );

    bool writeHeader = false;
    for( int i = 0; i < NumLevelEffects; ++i )
    {
        if( m_effects[i] )
        {
            if( !writeHeader )
            {
                _out->printf("\tEffects\t");
                writeHeader = true;
            }
            switch(i)
            {
                case LevelEffectSnow:               _out->printf( "snow\t");                break;
                case LevelEffectDustStorm:          _out->printf( "dustwind\t" );           break;
                case LevelEffectSoulRain:           _out->printf( "soulrain\t") ;           break;
                case LevelEffectLightning:          _out->printf( "lightning\t" );          break;
                case LevelEffectSpecialLighting:    _out->printf( "SpecialLighting\t" );    break;
            }
        }
    }
    if( writeHeader )
    {
        _out->printf("\n");
    }

    writeHeader = false;
    for( int i = 0; i < NumLevelOptions; ++i )
    {
        if( m_levelOptions[i] > 0 )
        {
            if( !writeHeader )
            {
                _out->printf( "\tLevelOptions\t");
                writeHeader = true;
            }
            switch( i )
            {
                case LevelOptionNoHandicap:                 _out->printf( "nohandicap\t" );                                         break;
                case LevelOptionInstantSpawnPointCapture:   _out->printf( "instantspawnpointcapture\t" );                           break;
                case LevelOptionTrunkPortReinforcements:    _out->printf( "trunkportreinforcements %d\t", m_levelOptions[i] );      break;
                case LevelOptionTrunkPortArmour:            _out->printf( "trunkportarmour %d\t", m_levelOptions[i] );              break;
                case LevelOptionNoGunTurrets:               _out->printf( "nogunturrets\t" );                                       break;
                case LevelOptionAttackerReinforcements:     _out->printf( "attackerreinforcements %d\t", m_levelOptions[i] );       break;
                case LevelOptionDefenderReinforcements:     _out->printf( "defenderreinforcements %d\t", m_levelOptions[i] );       break;
                case LevelOptionDefenderReinforcementType:  _out->printf( "defenderreinforcementtype %s\t", GlobalResearch::GetTypeName(m_levelOptions[i]) );   break;
                case LevelOptionAttackerReinforcementType:  _out->printf( "attackerreinforcementtype %s\t", GlobalResearch::GetTypeName(m_levelOptions[i]) );   break;
                case LevelOptionForceRadarTeamMatch:        _out->printf( "forceradarteammatch\t" );                                break;
                case LevelOptionNoRadarControl:             _out->printf( "noradarcontrol\t" );                                     break;
                case LevelOptionAirstrikeCaptureZones:      _out->printf( "airstrikecapturezones %d\t", m_levelOptions[i] );        break;
                case LevelOptionFuturewinianTeam:           _out->printf( "futurewinianteam %d\t", m_levelOptions[i] );
                case LevelOptionAttackReinforcementMode:    _out->printf( "attackreinforcementoption %d\t", m_levelOptions[i] );    break;
                case LevelOptionDefendReinforcementMode:    _out->printf( "defendreinforcementoption %d\t", m_levelOptions[i] );    break;

            }
        }
    }
    if( writeHeader ) _out->printf( "\n" );

    writeHeader = false;
    for( int i = 0; i < Crate::NumCrateRewards; ++i )
    {
        if( m_invalidCrates[i] )
        {
            if( !writeHeader )
            {
                _out->printf( "\tInvalidCrates\t");
                writeHeader = true;
            }
            _out->printf( "%s\t", Crate::GetName(i) );
        }
    }
    if( writeHeader ) _out->printf( "\n" );

    if( m_defaultPlayTime != -1 ) _out->printf( "\tPlayTime\t%d\n", m_defaultPlayTime );

    if( strcmp(m_author, "Author") != 0 && strlen( m_author ) > 0 ) _out->printf( "\tAuthor\t\t%s\n", m_author );
    if( strcmp(m_description, "LevelDescription") != 0 && strlen( m_description ) > 0 ) _out->printf( "\tDescription\t%s\n", m_description );
    if( m_difficultyRating ) _out->printf( "\tDifficulty\t%s\n", m_difficultyRating );

    writeHeader = false;
    for( int i = 0; i < NUM_TEAMS-2; ++i )
    {
        if( m_attacking[i] )
        {
            if( !writeHeader )
            {
                _out->printf( "\tAttackingTeams\t");
                writeHeader = true;
            }
            _out->printf("%d ", i );
        }
    }
    if( writeHeader ) _out->printf("\n");

    if( m_coop ) _out->printf( "\tCoopMode\n" );
    if( m_forceCoop ) _out->printf( "\tForceCoop\n" );

    if( m_coopGroup1Positions.Size() > 0 )
    {
        _out->printf( "\tCoopGroup1Positions\t");
        for( int i = 0; i < m_coopGroup1Positions.Size(); ++i )
        {
            _out->printf( "%d ", m_coopGroup1Positions[i] );
        }
        _out->printf( "\n" );
    }

    if( m_coopGroup2Positions.Size() > 0 )
    {
        _out->printf( "\tCoopGroup2Positions\t");
        for( int i = 0; i < m_coopGroup2Positions.Size(); ++i )
        {
            _out->printf( "%d ", m_coopGroup2Positions[i] );
        }
        _out->printf( "\n" );
    }

    if( m_official )
    {
        _out->printf( "\tOfficialLevel\n" );
    }

    _out->printf("MultiwiniaOptions_EndDefinition\n\n");
}

bool LevelFile::HasEffect(char *_effect)
{
    /*for( int i = 0; i < m_effects.Size(); ++i )
    {
        if( strcmp( m_effects[i], _effect ) == 0 )
        {
            return true;
        }
    }*/

    return false;
}

int LevelFile::CalculeCRC( char const *_mapFilename )
{
	int mapCRC = -1;

	if( !_mapFilename )
	{
		return mapCRC;
	}

	char filename[512];
	snprintf( filename, sizeof(filename), "levels/%s", _mapFilename );
	filename[ sizeof(filename) - 1 ] = '\0';

	// Determine CRC identifier of map
	BinaryReader *reader = g_app->m_resource->GetBinaryReader( filename );
	if( reader && reader->IsOpen() )
	{
		mapCRC = FileCRC( reader );
	}
	delete reader;

	return mapCRC;
}
