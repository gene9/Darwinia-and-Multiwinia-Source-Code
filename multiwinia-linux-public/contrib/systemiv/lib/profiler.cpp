#include "lib/universal_include.h"

#include <math.h>
#include <float.h>

#include "debug_utils.h"
#include "profiler.h"
#include "hi_res_time.h"

#pragma warning( disable : 4189 )

#define PROFILE_HISTORY_LENGTH  10

Profiler *g_profiler = NULL;


// ****************************************************************************
//  Class ProfiledElement
// ****************************************************************************

// *** Constructor
ProfiledElement::ProfiledElement(char const *_name, ProfiledElement *_parent)
:   m_currentTotalTime(0.0),
	m_currentNumCalls(0),
	m_lastTotalTime(0.0),
	m_lastNumCalls(0),
	m_longest(DBL_MIN),
	m_shortest(DBL_MAX),
	m_callStartTime(0.0),
	m_historyTotalTime(0.0),
	m_historyNumSeconds(0.0),
	m_historyNumCalls(0),
	m_parent(_parent),
	m_isExpanded(false)
{
    m_name = strdup(_name);
}


// *** Destructor
ProfiledElement::~ProfiledElement()
{
}


// *** Start
void ProfiledElement::Start()
{
    m_callStartTime = GetHighResTime();
}


// *** End
void ProfiledElement::End()
{
	double const timeNow = GetHighResTime();

	m_currentNumCalls++;
	double const duration = timeNow - m_callStartTime;
	m_currentTotalTime += duration;
	
	if (duration > m_longest)
	{
		m_longest = duration;
	}
	if (duration < m_shortest)
	{
		m_shortest = duration;
	}
}


// *** Advance
void ProfiledElement::Advance()
{
	m_lastTotalTime = m_currentTotalTime;
	m_lastNumCalls = m_currentNumCalls;
	m_currentTotalTime = 0.0;
	m_currentNumCalls = 0;
	m_historyTotalTime += m_lastTotalTime;
	m_historyNumSeconds += 1.0;
	m_historyNumCalls += m_lastNumCalls;

//    float thisMax = m_lastTotalTime;
//    if( thisMax > g_profiler->m_maxFound ) g_profiler->m_maxFound = thisMax;

	for (int i = 0; i < m_children.Size(); ++i)
	{
		if (m_children.ValidIndex(i))
		{
			m_children[i]->Advance();
		}
	}
}


// *** ResetHistory
void ProfiledElement::ResetHistory()
{
	m_historyTotalTime = 0.0;
	m_historyNumSeconds = 0.0;
	m_historyNumCalls = 0;
	m_longest = DBL_MIN;
	m_shortest = DBL_MAX;

	for (unsigned int i = 0; i < m_children.Size(); ++i)
	{
		if (m_children.ValidIndex(i))
		{
			m_children[i]->ResetHistory();
		}
	}
}


double ProfiledElement::GetMaxChildTime()
{    
	double rv = 0.0;

	short first = m_children.StartOrderedWalk();
	if (first == -1)
	{
		return 0.0;
	}

	short i = first;
	while (i != -1)
	{
		float val = m_children[i]->m_historyTotalTime;
		ProfiledElement *child = m_children[i];
		if (val > rv)
		{
			rv = val;
		}

		i = m_children.GetNextOrderedIndex();
	}

	return rv / m_children[first]->m_historyNumSeconds;
}



// ****************************************************************************
//  Class Profiler
// ****************************************************************************

#ifdef SINGLE_THREADED_PROFILER
#ifdef TARGET_OS_LINUX
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#else
#include <SDL.h>
#include <SDL_thread.h>
#endif
static Uint32 s_profileThread;
#define MAIN_THREAD_ONLY { if (SDL_ThreadID() != s_profileThread) return; }
#else
#define MAIN_THREAD_ONLY {}
#endif 

// *** Constructor
Profiler::Profiler()
:   m_doGlFinish(false),
	m_currentElement(NULL),
	m_insideRenderSection(false),
    m_maxFound(0.0f),
    m_lastFrameStart(-1.0)
{
	m_rootElement = new ProfiledElement("Root", NULL);
	m_rootElement->m_isExpanded = true;
	m_currentElement = m_rootElement;
	m_endOfSecond = GetHighResTime() + 1.0f;
#ifdef SINGLE_THREADED_PROFILER
	s_profileThread = SDL_ThreadID();
#endif
}


// *** Destructor
Profiler::~Profiler()
{
}


void Profiler::Start()
{
    if( !g_profiler )
    {
        g_profiler = new Profiler();
    }
}


void Profiler::Stop()
{
    if( g_profiler )
    {
        delete g_profiler;
        g_profiler = NULL;
    }
}


// *** Advance
void Profiler::Advance()
{
	double timeNow = GetHighResTime();
	if (timeNow > m_endOfSecond)
	{
		m_lengthOfLastSecond = timeNow - (m_endOfSecond - 1.0);
		m_endOfSecond = timeNow + 1.0;

        m_maxFound = 0.0f;
		m_rootElement->Advance();  
	}

    if( m_lastFrameStart >= 0 )
    {
        double lastFrameTime = timeNow - m_lastFrameStart;
        m_frameTimes.PutDataAtStart( int( lastFrameTime * 1000 ) );
    }
    
    while( m_frameTimes.ValidIndex(200) )
    {
        m_frameTimes.RemoveData(200);
    }
    m_lastFrameStart = timeNow;
}


// *** RenderStarted
void Profiler::RenderStarted()
{
	m_insideRenderSection = true;
}


// *** RenderEnded
void Profiler::RenderEnded()
{
	m_insideRenderSection = false;
}


// *** ResetHistory
void Profiler::ResetHistory()
{
	m_rootElement->ResetHistory();
}

static bool s_expanded = false;

// *** StartProfile
void Profiler::StartProfile(char const *_name)
{
	MAIN_THREAD_ONLY;

	ProfiledElement *pe = m_currentElement->m_children.GetData(_name);
	if (!pe)
	{
		pe = new ProfiledElement(_name, m_currentElement);
		m_currentElement->m_children.PutData(_name, pe);
	}

	AppReleaseAssert(m_rootElement->m_isExpanded, "Profiler root element has been un-expanded");
	
    bool wasExpanded = m_currentElement->m_isExpanded;

    if (m_currentElement->m_isExpanded)
	{
		if (m_doGlFinish && m_insideRenderSection)
		{
			glFinish();
		}
 		pe->Start();
	}
	m_currentElement = pe;

    m_currentElement->m_wasExpanded = wasExpanded;
}


// *** EndProfile
void Profiler::EndProfile(char const *_name)
{
	((void)sizeof(_name));
	MAIN_THREAD_ONLY;

    //AppDebugAssert( m_currentElement->m_wasExpanded == m_currentElement->m_parent->m_isExpanded );

	if (m_currentElement &&
        m_currentElement->m_parent )
    {
        if( m_currentElement->m_parent->m_isExpanded)
	    {
		    if (m_doGlFinish && m_insideRenderSection)
		    {
			    glFinish();
		    }
        
		    AppDebugAssert(m_currentElement != m_rootElement);
		    AppDebugAssert(stricmp(_name, m_currentElement->m_name) == 0);

		    m_currentElement->End();
	    }

        AppDebugAssert( strcmp( m_currentElement->m_name, m_currentElement->m_parent->m_name ) != 0 );
	    m_currentElement = m_currentElement->m_parent;
    }
}


void ExpandAllRecursively( ProfiledElement *current )
{
    current->m_isExpanded = true;

    for( int i = 0; i < current->m_children.Size(); ++i )
    {
        if( current->m_children.ValidIndex(i) )
        {
            ProfiledElement *child = current->m_children[i];
            ExpandAllRecursively( child );
        }
    }
}


void Profiler::ExpandAll()
{
    ExpandAllRecursively( m_rootElement );
}


ProfiledElement *LookupElementRecursive(char const *_name, ProfiledElement *current )
{
    if( !current )
    {
        return NULL;
    }

    if( stricmp( current->m_name, _name ) == 0 )
    {
        return current;
    }

    for( int i = 0; i < current->m_children.Size(); ++i )
    {
        if( current->m_children.ValidIndex(i) )
        {
            ProfiledElement *child = current->m_children[i];
            ProfiledElement *result = LookupElementRecursive( _name, child );
            if( result ) return result;
        }
    }

    return NULL;
}


ProfiledElement *Profiler::LookupElement(char const *_name)
{
    return LookupElementRecursive( _name, m_rootElement );
}
