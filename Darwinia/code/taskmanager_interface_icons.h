#ifndef _included_taskmanagerinterfaceicons_h
#define _included_taskmanagerinterfaceicons_h

#include "lib/llist.h"
#include "worldobject/entity.h"
#include "worldobject/worldobject.h"
#include "taskmanager_interface.h"

class QuickUnitButton;

class TaskManagerInterfaceIcons : public TaskManagerInterface
{
protected:
    enum
    {
        ScreenOverlay,                  // On all screens
        ScreenTaskManager,
        ScreenObjectives,
        ScreenResearch
    };
    int     m_screenId;

    float   m_screenY;
    float   m_desiredScreenY;
    float   m_screenW;
    float   m_screenH;

    float   m_chatLogY;

    LList   <ScreenZone *>          m_screenZones;               // All clickable areas on-screen
    LList   <ScreenZone *>          m_newScreenZones;            // New zones generated this frame
	LList   <KeyboardShortcut *>    m_keyboardShortcuts;         // Keyboard shortcuts to screenzones
    int     m_currentScreenZone;
    int     m_currentMouseScreenZone;
	int		m_currentScrollZone;
    float   m_screenZoneTimer;
    double  m_taskManagerDownTime;

public:
    int     m_currentQuickUnit;
    int     m_quickUnitDirection;
    LList   <QuickUnitButton *>     m_quickUnitButtons;

protected:
    void    AdvanceScrolling            ();
    void    AdvanceScreenEdges          ();
    void    AdvanceScreenZones          ();
    void    AdvanceKeyboardShortcuts    ();
    void    AdvanceTerminate            ();
    void    AdvanceQuickUnit            ();

    void    HideTaskManager             ();

    bool    ScreenZoneHighlighted   ( ScreenZone *_zone );
    void    RunScreenZone           ( const char *_name, int _data );          // Occurs during click or keypress

    void    SetupRenderMatrices     ( int _screenId );
    void    ConvertMousePosition    ( float &_x, float &_y );
    void    RestoreRenderMatrices   ();

    void    RenderTargetAreas   ();
    void    RenderMessages      ();
    void    RenderGestures      ();
    void    RenderRunningTasks  ();
    void    RenderOverview      ();
    void    RenderTaskManager   ();
    void    RenderObjectives    ();
    void    RenderResearch      ();
    void    RenderTitleBar      ();
    void    RenderScreenZones   ();
    void    RenderTooltip       ();
    void    RenderCreateTaskMenu();
    void    RenderQuickUnit     ();

    void    RenderCompass       ( float _screenX, float _screenY,
                                  Vector3 const &_worldPos, bool selected, float _size );

    int     GetQuickUnitTask    ( int _position = 2);
	void	CreateQuickUnitInterface();
	void	DestroyQuickUnitInterface();

	bool	ButtonHeld();
	bool	ButtonHeldAndReleased();

public:
    TaskManagerInterfaceIcons();

    virtual void Advance();
    virtual void Render();

	bool ControlEvent( TMControl _type );
	bool AdviseCreateControlHelpBlue();
	bool AdviseCreateControlHelpGreen();
	bool AdviseCloseControlHelp();
	bool AdviseOverSelectableZone();
};

class QuickUnitButton
{
public:
    int     m_taskId;
    int     m_positionId;
    float   m_size;

    int     m_x;
    int     m_y;
    float   m_alpha;

    bool    m_movable;

public:

    QuickUnitButton();
    void Advance();
    void Render();
    void ActivateButton();
};


#endif
