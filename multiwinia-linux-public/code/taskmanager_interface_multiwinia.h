#ifndef _included_taskmanagerinterfacemultiwinia_h
#define _included_taskmanagerinterfacemultiwinia_h

#include "lib/tosser/llist.h"
#include "worldobject/entity.h"
#include "worldobject/worldobject.h"
#include "taskmanager_interface.h"

class QuickUnitButton;


class TaskManagerInterfaceMultiwinia : public TaskManagerInterface
{
protected:   
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

protected:
    void    AdvanceScreenZones          ();    
    void    AdvanceKeyboardShortcuts    ();
    void    AdvanceTerminate            ();
    
    void    HideTaskManager             ();
    
    bool    ScreenZoneHighlighted   ( ScreenZone *_zone );
    void    RunScreenZone           ( const char *_name, int _data );          // Occurs during click or keypress

    void    ConvertMousePosition    ( float &_x, float &_y );

    void    RenderTargetAreas   ();
    void    RenderMessages      ();
    void    RenderRunningTasks  ();
    void    RenderTaskManager   ();
    void    RenderScreenZones   ();
    void    RenderTooltip       ();
    void    RenderRecentRunTask ();
    void    RenderCurrentCrateHelp();
    
    void    RenderScreenZoneHighlight( float x, float y, float w, float h );

    void    RenderCompass       ( float _screenX, float _screenY, 
                                  Vector3 const &_worldPos, bool selected, float _size );

	bool	ButtonHeld();
	bool	ButtonHeldAndReleased();
        
public:
    TaskManagerInterfaceMultiwinia();
	~TaskManagerInterfaceMultiwinia();

    virtual void Advance();        
    virtual void Render();

    void    SetupRenderMatrices     ();
    void    RestoreRenderMatrices   ();

	bool ControlEvent( TMControl _type );
	bool AdviseCreateControlHelpBlue();
	bool AdviseCreateControlHelpGreen();
	bool AdviseCloseControlHelp();
	bool AdviseOverSelectableZone();

    void    RenderExtraTaskInfo ( Task *task, Vector2 iconCentre, float iconSize, float alpha, 
                                   bool targetOrders=false, bool taskManager=false );

};

#endif
