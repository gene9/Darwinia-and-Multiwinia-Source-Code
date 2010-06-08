
#ifndef _included_soundparameterwindow_h
#define _included_soundparameterwindow_h

#include "interface/components/core.h"

class SoundParameter;



// ============================================================================
// class SoundParameterButton

class SoundParameterButton : public TextButton
{
public:
    SoundParameter *m_parameter;
    float           m_minOutput;
    float           m_maxOutput;
    
public:
    SoundParameterButton();

    void Render( int realX, int realY, bool highlighted,  bool clicked );
    void MouseUp();
};



// ============================================================================
// class SoundParameterEditor

class SoundParameterEditor : public InterfaceWindow
{
public:
    SoundParameter *m_parameter;
    float           m_minOutput;
    float           m_maxOutput;

public:
    SoundParameterEditor( char *_name );

    void Create();
    void Update();
};



// ============================================================================
// class SoundParameterGraph

class SoundParameterGraph : public InvertedBox
{
public:
    SoundParameter *m_parameter;
    float           m_minOutput;
    float           m_maxOutput;
    float           m_minInput;
    float           m_maxInput;
	bool			m_linearScale;
    
public:
	SoundParameterGraph(): m_linearScale(true) {}

    void SetParameter   ( SoundParameter *_parameter, float _minOutput, float _maxOutput );
    void GetPosition    ( float _output, float _input, float *_x, float *_y );
    void GetValues      ( float _x, float _y, float *_output, float *_input );

    void RescaleAxis();
    void HandleMouseEvents();

    void Render         ( int realX, int realY, bool highlighted, bool clicked );
    void RenderAxis     ( int realX, int realY );
    void RenderValues   ( int realX, int realY );
    void RenderOutput   ( int realX, int realY );
};



#endif
