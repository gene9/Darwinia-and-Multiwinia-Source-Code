#ifndef INCLUDED_PROFILER_H
#define INCLUDED_PROFILER_H

#include "lib/sorting_hash_table.h"


#ifdef PROFILER_ENABLED


//*****************************************************************************
// Class ProfiledElement
//*****************************************************************************

class ProfiledElement
{
public:
	// Values used for accumulating the profile for the current second
	double				m_currentTotalTime;		// In seconds
	int					m_currentNumCalls;

	// Values used for storing the profile for the previous second
    double				m_lastTotalTime;		// In seconds
    int					m_lastNumCalls;

	// Values used for storing history data (accumulation of all m_lastTotalTime
	// and m_lastNumCalls values since last reset)
	double				m_historyTotalTime;		// In seconds
	double				m_historyNumSeconds;
	int					m_historyNumCalls;

	// Values used for storing the longest and shortest duration spent inside
	// this elements profile. These values are reset when the history is reset
	double				m_shortest;
	double				m_longest;

	double				m_callStartTime;
    char				*m_name;

	SortingHashTable	<ProfiledElement *> m_children;
	ProfiledElement		*m_parent;

	bool				m_isExpanded;			// Bit of data that a tree view display can use
    bool                m_wasExpanded;

public:
    ProfiledElement	 (char const *_name, ProfiledElement *_parent);
    ~ProfiledElement ();

	void				Start			();
	void				End				();
	void				Advance			();
	void				ResetHistory	();

	double				GetMaxChildTime	();
};


//*****************************************************************************
// Class Profiler
//*****************************************************************************

class Profiler
{
private:
	bool				m_insideRenderSection;	// Used to decide whether to do a glFinish for each call to EndProfile

public:
    ProfiledElement		*m_currentElement;		// Stores the currently active profiled element
	ProfiledElement		*m_rootElement;
	bool				m_doGlFinish;
	double				m_endOfSecond;
	double				m_lengthOfLastSecond;	// Will be somewhere between 1.0 and (1.0 + g_advanceTime)

    Profiler();
    ~Profiler();

	void				Advance			();

	void				RenderStarted	();
	void				RenderEnded		();

    void				StartProfile	(char const *_name);
    void				EndProfile		(char const *_name);

	void				ResetHistory	();
};

	#define SET_PROFILE(profiler, itemName, value) profiler->SetProfile(itemName, value)
	#define START_PROFILE(profiler, itemName) profiler->StartProfile(itemName)
	#define END_PROFILE(profiler, itemName) profiler->EndProfile(itemName)

#else // PROFILER_ENABLED
	#define SET_PROFILE(profiler, name, value)
	#define START_PROFILE(profiler, itemName)
	#define END_PROFILE(profiler, itemName)
#endif // PROFILER_ENABLED


#endif

