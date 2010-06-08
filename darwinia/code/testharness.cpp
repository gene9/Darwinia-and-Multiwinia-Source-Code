#include "lib/universal_include.h"

#include <stdarg.h>
#include <string.h>

#include "worldobject/researchitem.h"
#include "worldobject/trunkport.h"

#include "app.h"
#include "global_world.h"
#include "level_file.h"
#include "testharness.h"

#ifdef TEST_HARNESS_ENABLED


TestHarness::TestHarness()
:	m_numErrors(0)
{
	m_indent = new char[256];
	for (int i = 0; i < 255; ++i)
	{
		m_indent[i] = ' ';
	}
	m_indent[255] = '\0';
	m_indent = &m_indent[255];

	m_out = fopen("test_log.txt", "w");
	setvbuf(m_out, NULL, _IONBF, 0 );
}


TestHarness::~TestHarness()
{
	delete [] m_indent;
	fclose(m_out);
}


void TestHarness::PrintError(char *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);
    fprintf(m_out, "%sERROR: %s\n", m_indent, buf);

	m_numErrors++;
}


void TestHarness::PrintGlobalState(GlobalWorld *_world)
{
	fprintf(m_out, "\n");


	//
	// Print maps and missions

	for (int i = 0; i < _world->m_locations.Size(); ++i)
	{
		GlobalLocation *loc = _world->m_locations[i];
		fprintf(m_out, "%s%-16s %-37s %-8s\n",
			m_indent,
			loc->m_name,
			loc->m_missionFilename,
			loc->m_available ? "available" : "unavailable");
	}


	//
	// Print global buildings

//	for (int i = 0; i < _world->m_buildings.Size(); ++i)
//	{
//		GlobalBuilding *building = _world->m_buildings[i];
//		fprintf(m_out, m_indent); 
//		fprintf(m_out, "%-14s ", Building::GetTypeName(building->m_type));
//		fprintf(m_out, "%-16s ", _world->GetLocationName(building->m_locationId));
//		fprintf(m_out, "%4d ", building->m_id);
//		fprintf(m_out, "%7s ", building->m_online ? "Online" : "Offline");
//		fprintf(m_out, "\n");
//	}


	fprintf(m_out, "\n");
}


LList <int> *TestHarness::FindAvailableMissions(GlobalWorld *_world)
{
	LList <int> *missions = new LList<int>;

	for (int i = 0; i < _world->m_locations.Size(); ++i)
	{
		GlobalLocation *loc = _world->m_locations[i];
		if (loc->m_available && 
			stricmp(loc->m_missionFilename, "null") != 0)
		{
			if (strnicmp(loc->m_missionFilename, "cutscene", strlen("cutscene")) == 0)
			{
				PrintError("Cutscene mission file left hanging around in %s",
						   _world->GetLocationName(loc->m_id));
				continue;
			}
			missions->PutDataAtEnd(loc->m_id);
		}
	}

	return missions;
}


void TestHarness::AchieveObjective(GlobalWorld *_world, GlobalEventCondition *_objective, 
								   int _locId, LevelFile *_levelFile)
{
	switch (_objective->m_type)
	{
		case GlobalEventCondition::AlwaysTrue:
		case GlobalEventCondition::DebugKey:
		case GlobalEventCondition::NotInLocation:
			break;

		case GlobalEventCondition::BuildingOffline:
		{
			GlobalBuilding *building = _world->GetBuilding(_objective->m_id, _locId);
			if (!building)
			{
				PrintError("Building online objective has bad ID of %s:%d",
						   _world->GetLocationName(_locId), _objective->m_id);
				break;
			}
			
			building->m_online = false;
			
			fprintf(m_out, "%sPlayer sets building OFFLINE: %s id:%d in location %s\n",
					m_indent,
					Building::GetTypeName(building->m_type),
					_objective->m_id, 
					_world->GetLocationName(_locId));
			break;
		}

		case GlobalEventCondition::BuildingOnline:
		{
			GlobalBuilding *building = _world->GetBuilding(_objective->m_id, _locId);
			if (!building)
			{
				PrintError("Building online objective specifies a none existant building (%s:%d)",
						   _world->GetLocationName(_locId), _objective->m_id);
				break;
			}
			
			building->m_online = true;
			
			fprintf(m_out, "%sPlayer sets building ONLINE: %s id:%d in location %s\n",
					m_indent,
					Building::GetTypeName(building->m_type),
					_objective->m_id, 
					_world->GetLocationName(_locId));

			if (building->m_type == Building::TypeTrunkPort)
			{
				TrunkPort *port = (TrunkPort*)_levelFile->GetBuilding(_objective->m_id);
				GlobalLocation *targetLoc = _world->GetLocation(port->m_targetLocationId);
				if (targetLoc)
				{
					targetLoc->m_available = true;
				}
				else
				{
					PrintError("Trunk Port (%s:%d) has an invalid target location ID",
							   _world->GetLocationName(_locId), _objective->m_id);
				}
			}

			break;
		}
		
		case GlobalEventCondition::ResearchOwned:
			fprintf(m_out, "%sPlayer collects research %d\n", m_indent, _objective->m_id);
			_world->m_research->AddResearch(_objective->m_id);
			break;
	}
}


void TestHarness::DoMission(GlobalWorld *_world, int _locId)
{

	GlobalLocation *loc = _world->GetLocation(_locId);

	fprintf(m_out, "%sDoing mission in location %s: %s\n", m_indent, 
			_world->GetLocationName(_locId),
			loc->m_missionFilename);

	m_missionsDone.PutDataAtEnd(strdup(loc->m_missionFilename));

	// Increase indent by three characters
	m_indent -= 3;

	LevelFile levelFile(loc->m_missionFilename, loc->m_mapFilename);

	// Complete primaries
	for (int i = 0; i < levelFile.m_primaryObjectives.Size(); ++i)
	{
		GlobalEventCondition *objective = levelFile.m_primaryObjectives.GetData(i);
		AchieveObjective(_world, objective, _locId, &levelFile);
	}

	// Complete secondaries
	for (int i = 0; i < levelFile.m_secondaryObjectives.Size(); ++i)
	{
		GlobalEventCondition *objective = levelFile.m_secondaryObjectives.GetData(i);
		AchieveObjective(_world, objective, _locId, &levelFile);
	}

	// Set the mission file to "null"
	strcpy(loc->m_missionFilename, "null");
	
	// Re-evaluate global events
	fprintf(m_out, "\n%sEvaluating events\n", m_indent);
	while (_world->EvaluateEvents()) ;

	// Dump events triggered to test result file eg Cutscene
}


void TestHarness::ExploreStates(GlobalWorld *_world)
{
	PrintGlobalState(_world);
	
	LList <int> *missions = FindAvailableMissions(_world);

	if (missions->Size() == 0)
	{
		// We have reached an end state for the game
		fprintf(m_out, "%sGame Over. Missions done: %d\n", m_indent, m_missionsDone.Size());
		m_indent -= 3;

		CompletedGame *completedGame = new CompletedGame;

		for (int i = 0; i < m_missionsDone.Size(); ++i)
		{
			fprintf(m_out, "%s%s\n", m_indent, m_missionsDone[i]);
			completedGame->m_missionsDone.PutDataAtEnd(strdup(m_missionsDone[i] + 8));
		}

		m_completedGames.PutDataAtEnd(completedGame);
		
		m_indent += 3;
	}
	else
	{
		// Do all the missions that are available
		for (int i = 0; i < missions->Size(); ++i)
		{
			// Copy global state and place into stack
			GlobalWorld *copyOfWorld = new GlobalWorld(*_world);
			
			// Big hack
			GlobalWorld *oldGlobalWorld = g_app->m_globalWorld;
			g_app->m_globalWorld = copyOfWorld;

			// Do mission
			DoMission(copyOfWorld, missions->GetData(i));

			// Recurse
			ExploreStates(copyOfWorld);
			
			m_indent += 3;
			char *missionName = m_missionsDone.GetData(m_missionsDone.Size() - 1);
			m_missionsDone.RemoveData(m_missionsDone.Size() - 1);
			free(missionName);

			fprintf(m_out, "%sEnd of mission in %s\n", m_indent, 
					_world->GetLocationName(missions->GetData(i)));

			
			// Remove global state from stack and delete
			delete copyOfWorld;

			// Undo big hack
			g_app->m_globalWorld = oldGlobalWorld;
		}
	}

	delete missions;
}


void TestHarness::RunTest()
{
	ExploreStates(g_app->m_globalWorld);

	fprintf(m_out, "\n\n\n");
	fprintf(m_out, "========================================================\n");
	fprintf(m_out, "                        Summary\n");
	fprintf(m_out, "========================================================\n\n");

	for (int i = 0; i < m_completedGames.Size(); ++i)
	{
		CompletedGame *game = m_completedGames[i];

		fprintf(m_out, "Game %d: Num missions: %d\n", i, game->m_missionsDone.Size());

		for (int j = 0; j < game->m_missionsDone.Size(); ++j)
		{
			fprintf(m_out, "\t%s\n", game->m_missionsDone[j]);
		}
	}
	fprintf(m_out, "\nNumber of errors: %d\n", m_numErrors);
}


#endif // TEST_HARNESS_ENABLED
