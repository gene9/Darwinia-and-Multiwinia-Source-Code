#ifndef _included_multiwiniahelp_h
#define _included_multiwiniahelp_h


#include <vector>


class AnimatedPanelRenderer;
class WorldObjectId;


// ============================================================================


class MultiwiniaHelpItem
{
public:
    char m_iconFilename[256];
    bool m_appropriate;
    int  m_accomplished;
    bool m_renderMe;

    float m_screenX;
    float m_screenY;
    float m_size;
    float m_alpha;
    float m_screenLinkX;
    float m_screenLinkY;
    float m_screenLinkSize;

    float m_targetX;
    float m_targetY;
    float m_targetSize;

    float m_worldSize;
    Vector3 m_worldPos;
    float m_worldOffset;

public:
    MultiwiniaHelpItem();    
};


// ============================================================================



class MultiwiniaHelp
{
protected:
    void RenderTab2d( float screenX, float screenY, float size, char *icon, float alpha );
    void RenderTabLink( MultiwiniaHelpItem *item );

    void RenderTab3d( Vector3 const &pos, float size, char *icon, float alpha );
    void RenderTab3d( Vector3 const &pos, float width, UnicodeString &title, UnicodeString &message );

    void RenderControlHelp( int position, char *caption=NULL, char *iconKb=NULL, char *iconXbox=NULL, 
                            float alpha = 1.0f, float highlight = 0.0f, bool tutorialHighlight=false );
    void RenderDeselect();

    void        AttachTo3d              ( int helpId );
    void        AttachTutorialTo3d      ();
    void        AttachTutorial2To3d     ();

    Building    *FindNearestBuilding    ( int type, int teamId = -1, int occupied = 0 );            // occupied: 0=dont care, 1=require yes, -1=require no
                                                                                                    // teamId: -1=dont care, -2=enemy
    int         CountIdleDarwinians         ( Vector3 const &pos, float range, int teamId );
    int         CountDarwiniansGoingHere    ( Vector3 const &pos, int teamId );
    int         CountOfficersWithGotoHere   ( Vector3 const &pos, Vector3 const &target );
    int         CountFormationOfficersGoingHere      ( Vector3 const &pos );
    Vector3     GetFormationOfficerPosition ();
    int         CountActiveArmour();    
    
    void        RunTutorialHelp         ( int helpId, Vector3 const &pos, float offset );

	void LoadIcons();

    int CratesOnLevel();
    int SpawnPointsOwned();
    int SpawnPointsAvailableForCapture();
    bool HighlightingDarwinian();
    bool WallBusterOwned();

    bool PulseBombToAttack();
    bool PulseBombToDefend();

    void InitialiseAnimatedPanel();

private:
    static std::vector<WorldObjectId> s_neighbours;

public:
    enum
    {
        HelpStartHere,
        HelpReinforcements,
        HelpSpawnPoints,
        HelpKoth,
        HelpLiftStatue,
        HelpCaptureStatue,
        HelpSolarPanels,
        HelpRocketRiot,
        HelpSpawnPointsConquer,
        HelpCrates,
        HelpDarwinians,
        HelpWallBuster,
        HelpStationAttack,
        HelpStationDefend,
        HelpPulseBombAttack,
        HelpPulseBombDefend,
        HelpBlitzkrieg1,
        HelpBlitzkrieg2,
        HelpTutorialGroupSelect,
        HelpTutorialMoveToSpawnPoint,
        HelpTutorialMoveToMainZone,
        HelpTutorialPromote,
        HelpTutorialPromoteAdvanced,
        HelpTutorialSendToZone,
        HelpTutorialCaptureZone,
        HelpTutorialScores,
        HelpTutorialCrateFalling,
        HelpTutorialCrateCollect,
        HelpTutorialSelectTask,
        HelpTutorialUseAirstrike,
        HelpTutorial2SelectRadar,
        HelpTutorial2AimRadar,
        HelpTutorial2Deselect,
        HelpTutorial2UseRadar,
        HelpTutorial2RadarTransit,
        HelpTutorial2OrderInRadar,
        HelpTutorial2FormationSetup,
        HelpTutorial2FormationMove,
        HelpTutorial2FormationInfo,
        HelpTutorial2FormationAttack,
        HelpTutorial2SelectArmour,
        HelpTutorial2PlaceArmour,
        HelpTutorial2LoadArmour,
        HelpTutorial2MoveArmour,
        HelpTutorial2UnloadArmour,
        HelpTutorial2CaptureStation,
        HelpTutorial2End,
        HelpTutorial2BreakFormation,
        HelpTutorialDismissInfoBox,
        NumHelpItems
    };

    MultiwiniaHelpItem m_tabs[NumHelpItems];

	int	    m_inputMode;

    int     m_groupSelected;
    int     m_promoted;
    int     m_reinforcements;

    int     m_currentHelpHighlight;
    float   m_currentHelpTimer;

    int     m_currentTutorialHelp;
    double  m_tutorialTimer;

	float	m_showingScoreHelpTime;
	bool	m_showingScoreCheck;
	bool	m_setScoreHelpTime;

    float   m_showSpaceDeselectHelp;
    bool    m_spaceDeselectShowRMB;
    float   m_showShiftToSpeedUpHelp;

    int     m_tutorialCrateId;
    bool    m_tutorial2RadarHelpDone;
    bool    m_tutorial2FormationHelpDone;
    bool    m_tutorial2ArmourHelpDone;
    bool    m_tutorial2AICanAct;
    bool    m_tutorial2AICanSpawn;

    AnimatedPanelRenderer *m_animatedPanel;

public:
    MultiwiniaHelp();

    void Advance();
    void Render();
    void Reset();

	void RenderCurrentTaskHelp( float _iconCentreX, float _iconCentreY, float _iconSize, int _taskId );
	void RenderCurrentUnitHelp( float iconCentreX, float iconCentreY, float iconSize, int  );

    void RenderGracePeriodHelp();
    void RenderBasicControlHelp();
    void RenderControlHelp();
    void RenderTutorialHelp();
    void RenderAnimatedPanel();
    void RenderHelpMessage( float alpha, UnicodeString caption, bool buildingPosition=false, bool oversize=false );

    void NotifyCapturedCrate();
    void NotifyCapturedKothZone( int numZones );
    void NotifyLiftedStatue();
    void NotifyCapturedStatue();
    void NotifyGroupSelectedDarwinians();
    void NotifyMadeOfficer();
    void NotifyFueledUp();
    void NotifyReinforcements();

    void ShowSpaceDeselectHelp( bool immediate=false, bool showRMB=true );
    void ShowShiftSpeedupHelp();

    int GetResearchType( int _helpType );
    int GetHelpType( int _taskId );

    static UnicodeString   GetHelpString( int _helpType );
    static UnicodeString   GetTitleString( int _helpType );   
};



#endif
