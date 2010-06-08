#ifndef INCLUDED_SEPULVEDA_H
#define INCLUDED_SEPULVEDA_H

#ifdef USE_SEPULVEDA_HELP_TUTORIAL

#include "lib/tosser/llist.h"
#include "lib/tosser/btree.h"
#include "lib/vector3.h"


class GestureDemo;
class TextReader;
class MouseCursor;
class SepulvedaHighlight;
class SepulvedaCaption;


#define SEPULVEDA_MAX_PHRASE_LENGTH	1024


// ****************************************************************************
// Class Sepulveda
// ****************************************************************************

class Sepulveda
{
protected:   
    UnicodeString m_caption;

    LList       <char *> m_msgQueue;                
        
    float       m_timeSync;
    float       m_fade;
    int         m_previousNumChars;      
    bool        m_cutsceneMode;
    float       m_scrollbarOffset;

	GestureDemo	    *m_gestureDemo;
	float		    m_gestureStartTime;
	MouseCursor     *m_mouseCursor;

    LList           <SepulvedaHighlight *> m_highlights;
    LList           <SepulvedaCaption *> m_history;
    BTree			<const char *> m_langDefines;
	
protected:
    float       CalculateTextTime();            // Time for Dr Sepulveda to speak
    float       CalculatePauseTime();           // Pause time between Sepulveda's lines

	void		RenderGestureDemo();
    void        RenderHighlights();
    void        RenderTextBox();
    void        RenderTextBoxCutsceneMode();
    void        RenderTextBoxTaskManagerMode();
    void        RenderScrollBar();

    float       GetCaptionAlpha( float _timeSync );
	void		BuildCaption(UnicodeString const &_baseString, UnicodeString &_dest);	// Caller must pass in a string of size SEPULVEDA_MAX_PHRASE_LENGTH as _dest

    void        RenderFace( float _x, float _y, float _w, float _h, float _alpha );

public:
    Sepulveda();
	~Sepulveda();

    void Advance    ();
    void Render     ();

    void Say                ( char *_stringId );
    void DemoGesture        ( char const *_gestureDemoName, float _startDelay );
    void ShutUp             ();     

    void SetCutsceneMode    ( bool _cutsceneMode );
    bool IsInCutsceneMode   ();
    void HighlightBuilding  ( int _buildingId, char *_highlightName );
    void HighlightPosition  ( Vector3 const &_pos, float _radius, char *_highlightName );
    bool IsHighlighted      ( Vector3 const &_pos, float _radius, char *_highlightName=NULL );
    void ClearHighlights    ( char *_highlightName=NULL );    

    bool PlayerSkipsMessage();      // Returns true if a message was skipped

    bool IsTalking  ();
    bool IsVisible  ();             // Is the text box visible at all
};


// ****************************************************************************
// Class SepulvedaCaption
// ****************************************************************************

class SepulvedaCaption
{
public:
    char        *m_stringId;
    float       m_timeSync;
};


// ****************************************************************************
// Class SepulvedaHighlight
// ****************************************************************************

class SepulvedaHighlight
{
public:
    Vector3     m_pos;
    float       m_radius;
    float       m_alpha;
    bool        m_ended;
    char        *m_name;

public:
    SepulvedaHighlight( char *_name );
    ~SepulvedaHighlight();

    void        Start   ( Vector3 const &_pos, float _radius );
    void        Stop    ();
    bool        Advance ();
    void        Render  ();
};

#endif // USE_SEPULVEDA_HELP_TUTORIAL

#endif
