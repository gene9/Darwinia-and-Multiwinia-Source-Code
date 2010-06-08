#ifndef _included_soundeditorwindow_h
#define _included_soundeditorwindow_h

#ifdef SOUND_EDITOR

#include "interface/darwinia_window.h"

#include "sound/soundsystem.h"


#define SOUND_EDITOR_WINDOW_NAME "Sound Editor"


class ScrollBar;


// ============================================================================
// class SoundEditorWindow

class SoundEditorWindow : public DarwiniaWindow
{
public:
    int             m_eventIndex;
	bool			m_objectSelectorEnabled;

public:
    SoundEditorWindow( char *_name );
    ~SoundEditorWindow();

    void SelectEvent( int _eventIndex );

    void Render( bool hasFocus );
    void Update();
    void Create();

    void CreateInstanceEditor();
    void RemoveInstanceEditor();

    void StartPlayback( bool looped );
    void StopPlayback();

    static void CreateSoundParameterButton( DarwiniaWindow *_window,
                                             char *name, SoundParameter *parameter, int y,
							                 float _minOutput, float _maxOutput,
                                             DarwiniaButton *callback=NULL, int x=-1, int w=-1 );
    static void RemoveSoundParameterButton( DarwiniaWindow *_window, char *name );

    SoundSourceBlueprint *GetSoundSourceBlueprint();
    SoundEventBlueprint *GetSoundEventBlueprint();

	void GetSelectionName(char *_buf);
};


// ============================================================================
// class DspEffectEditor

class DspEffectEditor : public DarwiniaWindow
{
public:
    DspHandle *m_effect;

public:
    DspEffectEditor( char *_name );

    void Create();
};


// ============================================================================
// class SampleGroupEditor

class SampleGroupEditor : public DarwiniaWindow
{
public:
    SoundEventBlueprint *m_seb;
    ScrollBar *m_scrollbar;

public:
    SampleGroupEditor( char *_name );

    void SelectSampleGroup( char *_group );

    void Create();
    void Render( bool _hasFocus );
    void Remove();
};


// ============================================================================
// class RenameSampleGroupWindow

class RenameSampleGroupWindow : public DarwiniaWindow
{
public:
    char m_oldName[256];
    char m_newName[256];

public:
    RenameSampleGroupWindow( char *_name );
    void Create();
};



// ============================================================================
// class PurgeSoundsWindow

class PurgeSoundsWindow : public DarwiniaWindow
{
public:
    LList <char *> m_fileList;

    ScrollBar *m_scrollBar;

public:
    PurgeSoundsWindow( char *_name );

    void Create();
    void Remove();
    void Render( bool _hasFocus );
    void RefreshFileList();
};



#endif // SOUND_EDITOR

#endif
