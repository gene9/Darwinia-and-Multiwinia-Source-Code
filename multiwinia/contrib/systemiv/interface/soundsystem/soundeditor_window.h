#ifndef _included_soundeditorwindow_h
#define _included_soundeditorwindow_h

#include "interface/components/core.h"


class ScrollBar;
class SoundParameter;
class SoundEventBlueprint;
class SoundInstanceBlueprint;
class DspHandle;


// ============================================================================
// class SoundEditorWindow

class SoundEditorWindow : public InterfaceWindow
{
public:
    int             m_eventIndex;
        
public:
    SoundEditorWindow();
    ~SoundEditorWindow();

    void SelectEvent( int _eventIndex );

    void Render( bool hasFocus );
    void Update();
    void Create();

    void CreateInstanceEditor();
    void RemoveInstanceEditor();

    void StartPlayback( bool looped );
    void StopPlayback();

    static void CreateSoundParameterButton( InterfaceWindow *_window,
                                             char *name, SoundParameter *parameter, int y, 
							                 float _minOutput, float _maxOutput,
                                             InterfaceButton *callback=NULL, int x=-1, int w=-1 );
    static void RemoveSoundParameterButton( InterfaceWindow *_window, char *name );

    SoundEventBlueprint *GetSoundEventBlueprint();
    SoundInstanceBlueprint *GetSoundInstanceBlueprint();

	void GetSelectionName(char *_buf);
};


// ============================================================================
// class DspEffectEditor

class DspEffectEditor : public InterfaceWindow
{
public:
    DspHandle *m_effect;

public:
    DspEffectEditor( char *_name );

    void Create();
};


// ============================================================================
// class SampleGroupEditor

class SampleGroupEditor : public InterfaceWindow
{
public:
    SoundInstanceBlueprint *m_seb;
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

class RenameSampleGroupWindow : public InterfaceWindow
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

class PurgeSoundsWindow : public InterfaceWindow
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


#endif
