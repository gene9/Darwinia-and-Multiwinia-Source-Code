#ifndef INCLUDED_PROFILER_H
#define INCLUDED_PROFILER_H

#include "lib/tosser/sorting_hash_table.h"
#include "lib/tosser/llist.h"

class ProfiledElement;


//*****************************************************************************
// Class Profiler
//*****************************************************************************

class Profiler
{
private:
	bool				m_insideRenderSection;	                    // Used to decide whether to do a glFinish for each call to EndProfile

public:
    ProfiledElement		*m_currentElement;		                    // Stores the currently active profiled element
	ProfiledElement		*m_rootElement;
	bool				m_doGlFinish;
	double				m_endOfSecond;
	double				m_lengthOfLastSecond;	                    // Will be somewhere between 1.0 and (1.0 + g_advanceTime)
    double              m_lastFrameStart;
    float               m_maxFound;
    
    LList<int>          m_frameTimes;

public:
    Profiler();
    ~Profiler();

    static void         Start           ();
    static void         Stop            ();
	void				Advance			();

	void				RenderStarted	();
	void				RenderEnded		();

    void				StartProfile	(char const *_name);
    void				EndProfile		(char const *_name);

    void                ExpandAll       ();
	void				ResetHistory	();

    ProfiledElement		*LookupElement  (char const *_name);
};


extern Profiler *g_profiler;


#ifdef NO_PROFILER
#define START_PROFILE(itemName)
#define END_PROFILE(itemName)
#else
#define START_PROFILE(itemName)             if(g_profiler) g_profiler->StartProfile(itemName)
#define END_PROFILE(itemName)               if(g_profiler) g_profiler->EndProfile(itemName)
#endif


//*****************************************************************************
// Class ProfiledElement
//*****************************************************************************

class ProfiledElement
{
public:	
	double				m_currentTotalTime;		                    // Values used for accumulating the profile for the current second
	int					m_currentNumCalls;                          // In seconds
    double				m_lastTotalTime;		                    // Values used for storing the profile for the previous second
    int					m_lastNumCalls;                             // In seconds		
	double				m_historyTotalTime;		                    // Values used for storing history data (accumulation of all m_lastTotalTime
	double				m_historyNumSeconds;                        // and m_lastNumCalls values since last reset)
	int					m_historyNumCalls;                          // In seconds
	double				m_shortest;                                 // Values used for storing the longest and shortest duration spent inside
	double				m_longest;                                  // this elements profile. These values are reset when the history is reset
	double				m_callStartTime;
    bool				m_isExpanded;			                    // Bit of data that a tree view display can use
    bool                m_wasExpanded;
    char				*m_name;

	SortingHashTable	<ProfiledElement *> m_children;
	ProfiledElement		*m_parent;

public:
    ProfiledElement	    (char const *_name, ProfiledElement *_parent);
    ~ProfiledElement    ();
	
	void				Start			();
	void				End				();
	void				Advance			();
	void				ResetHistory	();
    void                ResetTotalTime  ();

	double				GetMaxChildTime	();
};


#endif
