
#ifndef _included_helpsystem_h
#define _included_helpsystem_h

#ifdef USE_SEPULVEDA_HELP_TUTORIAL

#include "lib/tosser/llist.h"
#include "lib/tosser/darray.h"
#include "worldobject/worldobject.h"

class Shape;
class ShapeMarker;


// ============================================================================
// Class ActionHelp

class ActionHelp
{
public:
    bool    m_actionDone;
    int     m_data;
    
public:
    ActionHelp();
    
    virtual bool IsActionAvailable  ();
    virtual void GiveActionHelp     ();
    virtual void PlayerDoneAction   ();
};


// ============================================================================
// Class HelpSystem

#define HELPSYSTEM_ACTIONHELP_INTERVAL      90

class HelpSystem
{    
protected:
    int             m_nextHelpObjectId;
    float           m_actionHelpTimer;

public:   
    bool            m_helpEnabled;

    DArray          <ActionHelp *> m_actionHelp;
    DArray          <ActionHelp *> m_buildingHelp;
   
    enum
    {
        CameraMovement,                                 // These are PRIORITY ORDERED
        CameraHeight,
        TaskBasics,
        TaskShutdown,
        SquadSummon,
        SquadUse,
        SquadThrowGrenade,
        SquadDeselect,
        TaskReselect,
        EngineerSummon,
        EngineerDeselect,
        OfficerCreate,
        UseArmour,
        ShowObjectives,
        CameraZoom,
        CameraMoveFast,
        ShowChatLog,
        UseRadarDish,
        UseGunTurret,
        ResearchUpgrade,
        NumActionHelps
    };

public:
    HelpSystem();
	~HelpSystem();
    
    void        PlayerDoneAction    ( int _actionId, int _data=-1 );               // Data can be used for anything
    
    bool        RunHighlightedBuildingHelp  ();
    void        RunDefaultHelp              ();                           // Brings up help when all other help has run out
    void        Advance                     ();
    void        Render                      ();
};



#endif // USE_SEPULVEDA_HELP_TUTORIAL

#endif
