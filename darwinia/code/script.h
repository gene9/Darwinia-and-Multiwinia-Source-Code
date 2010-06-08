#ifndef _included_script_h
#define _included_script_h

class TextReader;
class LevelFile;


class Script
{
public:
	// If you modify this remember to update g_opCodeNames in script.cpp
	enum
	{
		OpCamCut,
		OpCamMove,
		OpCamAnim,
        OpCamFov,
        OpCamBuildingFocus,
        OpCamBuildingApproach,
        OpCamLocationFocus,                     // global world only please
        OpCamGlobalWorldFocus,                  // global world only please
        OpCamReset,
		OpEnterLocation,
		OpExitLocation,
		OpSay,
        OpShutUp,
		OpWait,
		OpWaitSay,
		OpWaitCam,
		OpWaitFade,
        OpWaitRocket,
        OpWaitPlayerNotBusy,
        OpHighlight,
        OpClearHighlights,
        OpTriggerSound,
        OpStopSound,
        OpDemoGesture,
        OpGiveResearch,
        OpSetMission,
        OpGameOver,
        OpResetResearch,                        // Currently only affects darwinians
        OpRestoreResearch,                      // Currently only affects darwinians
        OpRunCredits,
        OpSetCutsceneMode,
        OpGodDishActivate,
        OpGodDishDeactivate,
        OpGodDishSpawnSpam,
        OpGodDishSpawnResearch,
        OpTriggerSpam,
        OpPurityControl,
        OpShowDarwinLogo,
        OpShowDemoEndSequence,
        OpPermitEscape,
		OpDestroyBuilding,
		OpActivateTrunkPort,
		OpActivateTrunkPortFull,
		OpNumOps
	};

    TextReader  *m_in;
    double      m_waitUntil;
    bool        m_waitForSpeech;
	bool		m_waitForCamera;
	bool		m_waitForFade;
    bool        m_waitForPlayerNotBusy;

    int         m_requestedLocationId;
    int         m_darwinianResearchLevel;

    int         m_rocketId;
    int         m_rocketState;
    int         m_rocketData;
    bool        m_waitForRocket;
    bool        m_permitEscape;

protected:
	void ReportError(LevelFile const *_levelFile, char *_fmt, ...);

	void RunCommand_CamCut			    (char const *_mountName);
	void RunCommand_CamMove			    (char const *_mountName, float _duration);
	void RunCommand_CamAnim			    (char const *_animName);
    void RunCommand_CamFov              (float _fov, bool _immediate);
    void RunCommand_CamBuildingFocus    (int _buildingId, float _range, float _height);
    void RunCommand_CamBuildingApproach (int _buildingId, float _range, float _height, float _duration );
    void RunCommand_CamGlobalWorldFocus ();
    void RunCommand_LocationFocus       (char const *_locationName, float _fov );
    void RunCommand_CamReset            ();

    void RunCommand_EnterLocation   (char *_name);
    void RunCommand_ExitLocation    ();
    void RunCommand_SetMission      (char *_locName, char *_missionName );
    void RunCommand_Say             (char *_stringId);
    void RunCommand_ShutUp          ();

    void RunCommand_Wait            (double _time);
    void RunCommand_WaitSay		    ();
	void RunCommand_WaitCam			();
	void RunCommand_WaitFade		();
    void RunCommand_WaitRocket      (int _buildingId, char *_state, int _data);
    void RunCommand_WaitPlayerNotBusy();

    void RunCommand_Highlight       (int _buildingId);
    void RunCommand_ClearHighlights ();

    void RunCommand_TriggerSound    (char const *_event);
    void RunCommand_StopSound       (char const *_event);

    void RunCommand_DemoGesture     (char const *_name);
    void RunCommand_GiveResearch    (char const *_name);

    void RunCommand_ResetResearch   ();
    void RunCommand_RestoreResearch ();

    void RunCommand_GameOver        ();
    void RunCommand_RunCredits      ();

    void RunCommand_GodDishActivate     ();
    void RunCommand_GodDishDeactivate   ();
    void RunCommand_GodDishSpawnSpam    ();
    void RunCommand_GodDishSpawnResearch();
    void RunCommand_SpamTrigger();

    void RunCommand_PurityControl();

    void RunCommand_ShowDarwinLogo();
    void RunCommand_ShowDemoEndSequence();

    void RunCommand_PermitEscape();

	void RunCommand_DestroyBuilding( int _buildingId, float _intensity );
	void RunCommand_ActivateTrunkPort( int _buildingId, bool _fullActivation );

public:
    Script();

    void Advance            ();
    void AdvanceScript      ();
    void RunScript          (char *_filename);
	void TestScript			(char *_filename);
    bool IsRunningScript    ();
    bool Skip               ();

	int	 GetOpCode					(char const *_word);
};



#endif
