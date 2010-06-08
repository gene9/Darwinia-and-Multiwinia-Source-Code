#ifndef _included_taskmanager_h
#define _included_taskmanager_h

#include "lib/llist.h"
#include "worldobject/entity.h"
#include "worldobject/worldobject.h"

class Route;
class GlobalEventCondition;

// ============================================================================
// class Task


class Task 
{
public:
    enum
    {
        StateIdle,                              // Not yet run
        StateStarted,                           // Running, not yet targetted
        StateRunning,                           // Targetted, running
        StateStopping                           // Stopping
    };

    int             m_id;
    int             m_type;
    int             m_state;
    WorldObjectId   m_objId;
    
    Route           *m_route;                   // Only used when this is a Controller task
    
public:
    Task();
    ~Task();

    void Start      ();
    bool Advance    ();
    void SwitchTo   ();
    void Stop       ();

    void    Target              ( Vector3 const &_pos );
    void    TargetSquad         ( Vector3 const &_pos );
    void    TargetEngineer      ( Vector3 const &_pos );
    void    TargetOfficer       ( Vector3 const &_pos );
    void    TargetArmour        ( Vector3 const &_pos );
   
    WorldObjectId        Promote         ( WorldObjectId _id );
    static WorldObjectId Demote          ( WorldObjectId _id );
    static WorldObjectId FindDarwinian   ( Vector3 const &_pos );

    static char *GetTaskName            ( int _type );
    static char *GetTaskNameTranslated  ( int _type );
};


// ============================================================================
// class TaskTargetArea


class TaskTargetArea
{
public:
    Vector3 m_centre;
    float   m_radius;
    bool    m_stationary;
};


// ============================================================================
// class TaskManager


class TaskManager
{
public:   
    LList   <Task *> m_tasks;

    int     m_nextTaskId;
    int     m_currentTaskId;
    bool    m_verifyTargetting;

protected:
    void    AdvanceTasks                ();
    
public:
    TaskManager();

    int     Capacity();
    int     CapacityUsed();
    
    bool    RunTask             ( Task *_task );                            // Starts the task, registers it
    bool    RunTask             ( int _type );
    bool    RegisterTask        ( Task *_task );                            // Assumes task is already started, just registers it

    int     MapGestureToTask    ( int _gestureId );                         // Maps a gesture to a task type

    Task    *GetCurrentTask     ();
    Task    *GetTask            ( int _id );
    Task    *GetTask            ( WorldObjectId _id );
    void    SelectTask          ( int _id );
    void    SelectTask          ( WorldObjectId _id );                      // Selects the corrisponding task 
	bool	IsValidTargetArea   ( int _id, Vector3 const &_pos );
    bool    TargetTask          ( int _id, Vector3 const &_pos );
    bool    TerminateTask       ( int _id );

    void    StopAllTasks        ();
    
    LList   <TaskTargetArea>    *GetTargetArea( int _id );                  // Returns all valid placement areas

    void    Advance             ();        
};


#endif
