#ifndef _included_debugmenu_h
#define _included_debugmenu_h

#include "interface/darwinia_window.h"
#include "app.h"


class DebugMenu : public DarwiniaWindow
{
public:
    DebugMenu( char *name );

	void Advance();
    void Create();
	void Render(bool hasFocus);
};


class DebugKeyBindings
{
public:
	static void DebugMenu();
	static void NetworkButton();
	static void DebugCameraButton();
	static void FollowCameraButton();
	static void FPSButton();
	static void InputLogButton();
#ifdef PROFILER_ENABLED
	static void ProfileButton();
#endif
#ifdef LOCATION_EDITOR
	static void EditorButton();
#endif
#ifdef AVI_GENERATOR
	static void GrabberButton();
#endif
#ifdef GESTURE_EDITOR
    static void GestureButton();
#endif
#ifdef CHEATMENU_ENABLED
    static void CheatButton();
#endif
#ifdef SOUND_EDITOR
    static void PokeyButton();
    static void SoundStatsButton();
    static void SoundVisualiserButton();
    static void SoundEditorButton();
	static void SoundProfileButton();
#endif // SOUND_EDITOR
	static void ReallyQuitButton();
	static void ToggleFullscreenButton();
};


#endif
