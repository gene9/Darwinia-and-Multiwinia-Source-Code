#ifndef _included_testharness_h
#define _included_testharness_h


#ifdef TEST_HARNESS_ENABLED

#include <stdio.h>

#include "lib/llist.h"

#include "global_world.h"


class CompletedGame
{
public:
	LList <char *> m_missionsDone;
};


class TestHarness
{
public:
	FILE	*m_out;
	char 	*m_indent;
	unsigned int m_numErrors;
	LList	<char *> m_missionsDone;
	LList	<CompletedGame *> m_completedGames;

protected:
    void		PrintGlobalState		(GlobalWorld *_world);
	LList<int> *FindAvailableMissions	(GlobalWorld *_world);
	void		DoMission				(GlobalWorld *_world, int _locId);
	void		AchieveObjective		(GlobalWorld *_world, 
										 GlobalEventCondition *_objective, 
										 int _locId, LevelFile *_levelFile);
    void		ExploreStates			(GlobalWorld *_world);		// Recursive

public:
    TestHarness();
	~TestHarness();

	void		PrintError				(char *_fmt, ...);
	void		RunTest					();
};



#endif // TEST_HARNESS_ENABLED

#endif
