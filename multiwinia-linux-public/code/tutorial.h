
#ifndef _included_tutorial_h
#define _included_tutorial_h

#ifdef USE_SEPULVEDA_HELP_TUTORIAL

#include <vector>


class WorldObjectId;


class Tutorial
{
protected:
    int     m_chapter;
    float   m_nextChapterTimer;
    float   m_repeatMessageTimer;
    float   m_repeatPeriod;
    char    *m_repeatMessage;
    char    *m_repeatGesture;

    std::vector<WorldObjectId> m_neighbours;

	void RepeatMessage( char *_stringId, float _repeatPeriod=20.0, char *_gestureDemo=NULL );

    virtual void TriggerChapter( int _chapter );
    virtual bool AdvanceCurrentChapter();

public:
    Tutorial();

    bool IsRunning();
    int  GetCurrentChapter();
    void SetChapter( int _chapter );
    void Restart();

    void Advance();
    void Render();
};


// ============================================================================


class Demo1Tutorial : public Tutorial
{
protected:
    void HandleSquadDeath();                // Before first incubator
    void HandleSquadDeath2();               // After first incubator
    
    void HandleEngineerDeath();             // first incubator
    void HandleEngineerDeath2();            // second incubator
    
    void TriggerChapter( int _chapter );
    bool AdvanceCurrentChapter();

public:
    Demo1Tutorial();
    
};


// ============================================================================


class Demo2Tutorial : public Tutorial
{
protected:
    void TriggerChapter( int _chapter );
    bool AdvanceCurrentChapter();

    void HandleSquadDeath();
    void HandleDishMisalignment();

protected:
    static float s_playerBusyTimer;
	double	m_timer;
    
public:
    Demo2Tutorial();
        
    static void BeginPlayerBusyCheck();
    static bool IsPlayerBusy();   
};


#endif // USE_SEPULVEDA_HELP_TUTORIAL

#endif
