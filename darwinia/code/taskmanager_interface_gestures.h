#ifndef _included_taskmanagerinterfacegestures_h
#define _included_taskmanagerinterfacegestures_h

#include "lib/llist.h"
#include "worldobject/entity.h"
#include "worldobject/worldobject.h"
#include "taskmanager_interface.h"


class TaskManagerInterfaceGestures : public TaskManagerInterface
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
    
    float   m_screenX;
    float   m_desiredScreenX;
    float   m_screenW;
    float   m_screenH;

    float   m_chatLogY;
	
    LList   <ScreenZone *>          m_screenZones;               // All clickable areas on-screen
    LList   <ScreenZone *>          m_newScreenZones;            // New zones generated this frame
	LList   <KeyboardShortcut *>    m_keyboardShortcuts;         // Keyboard shortcuts to screenzones
    int     m_currentScreenZone;
    float   m_screenZoneTimer;

protected:
    void    AdvanceScrolling            ();
    void    AdvanceScreenEdges          ();
    void    AdvanceGestures             ();
    void    AdvanceTerminate            ();
    void    AdvanceScreenZones          ();    
    void    AdvanceKeyboardShortcuts    ();
    
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
    
    void    RenderCompass       ( float _screenX, float _screenY, Vector3 const &_worldPos, bool selected );
        
public:
    TaskManagerInterfaceGestures();

    virtual void Advance();        
    virtual void Render();

	bool ControlEvent( TMControl _type );

	bool AdviseCreateControlHelpBlue();
	bool AdviseCreateControlHelpGreen();
	bool AdviseCloseControlHelp();
	bool AdviseOverSelectableZone();

#ifdef USE_DIRECT3D
	static void ReleaseD3dResources();
#endif
};



#endif
