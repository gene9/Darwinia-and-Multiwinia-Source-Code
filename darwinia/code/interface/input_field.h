#ifndef INCLUDED_INPUT_FIELD_H
#define INCLUDED_INPUT_FIELD_H

#include "lib/rgb_colour.h"

#include "darwinia_window.h"


// ****************************************************************************
// Class InputField
// ****************************************************************************

// This widget provides an editable text box where the user can type in numbers
// or short strings

class InputField: public BorderlessButton
{
public:
	char	m_buf[256];

	enum
	{
		TypeNowt,
		TypeChar,
		TypeInt,
		TypeFloat,
		TypeString
	};

	int		m_type;

	unsigned char	*m_char;
	int		        *m_int;
	float	        *m_float;
	char	        *m_string;

	int				m_inputBoxWidth;
	
	float			m_lowBound;
	float			m_highBound;

    DarwiniaButton *m_callback;
    
public:
	InputField();
	
	virtual void Render		( int realX, int realY, bool highlighted, bool clicked );
	virtual void MouseUp    ();
	virtual void Keypress	( int keyCode, bool shift, bool ctrl, bool alt );

	// These functions allow you to register a piece of storage with the button, such that
	// the button's value is written back to the storage whenever the user changes the
	// button's value
	void RegisterChar		(unsigned char *);
	void RegisterInt		(int *);
	void RegisterFloat		(float *);
	void RegisterString		(char *);

	void ClampToBounds		();

    void SetCallback        (DarwiniaButton *button);              // This button will be clicked on Refresh

	void Refresh			();	// Updates the display if the registered variable has changed
};



// ****************************************************************************
// Class InputScroller
// ****************************************************************************

class InputScroller : public DarwiniaButton
{
public:
    InputField *m_inputField;
    float       m_change;
    float		m_mouseDownStartTime;
    
public:
    
    InputScroller();

	void Render     ( int realX, int realY, bool highlighted, bool clicked );
    void MouseDown  ();
    void MouseUp    ();
};



// ****************************************************************************
// Class ColourWidget
// ****************************************************************************

class ColourWidget : public DarwiniaButton
{
public:
    DarwiniaButton *m_callback;
    int *m_value;
    
public:
    ColourWidget();

	void Render     ( int realX, int realY, bool highlighted, bool clicked );
    void MouseUp    ();
    
    void SetValue           (int *value);
    void SetCallback        (DarwiniaButton *button);              // This button will be clicked on Refresh
};



// ****************************************************************************
// Class ColourWindow
// ****************************************************************************

class ColourWindow : public DarwiniaWindow
{
public:
    DarwiniaButton *m_callback;
    int *m_value;
   
public:
    ColourWindow( char *_name );

    void SetValue           (int *value);
    void SetCallback        (DarwiniaButton *button);              // This button will be clicked on Refresh

    void Create();
    void Render( bool hasFocus );
};

#endif

