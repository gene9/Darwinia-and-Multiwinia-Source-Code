#ifndef INCLUDED_INPUT_FIELD_H
#define INCLUDED_INPUT_FIELD_H

#include "lib/rgb_colour.h"

#include "darwinia_window.h"
#include "network/networkvalue.h"


// ****************************************************************************
// Class InputField
// ****************************************************************************

// This widget provides an editable text box where the user can type in numbers
// or short strings

class InputField: public BorderlessButton
{
public:
	UnicodeString	m_buf;

	enum
	{
		TypeNowt,
		TypeChar,
		TypeBool,
		TypeInt,
		TypeFloat,
		TypeDouble,
		TypeString,
		TypeUnicodeString
	};

	int				m_type;

	NetworkValue	*m_value;
	bool			m_ownsValue;

	int				m_inputBoxWidth;
	
    DarwiniaButton *m_callback;
    
public:
	InputField();
	virtual ~InputField();
	
	virtual void Render		( int realX, int realY, bool highlighted, bool clicked );
	virtual void MouseUp    ();
	virtual void Keypress	( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii );

	// These functions allow you to register a piece of storage with the button, such that
	// the button's value is written back to the storage whenever the user changes the
	// button's value
	void RegisterBool		(bool *, double _lowBound = 0.0, double _highBound = 1.0e4 );
	void RegisterChar		(unsigned char *, double _lowBound = 0.0, double _highBound = 1.0e4 );
	void RegisterInt		(int *, double _lowBound = 0.0, double _highBound = 1.0e4 );
	void RegisterFloat		(float *, double _lowBound = 0.0, double _highBound = 1.0e4 );
	void RegisterDouble		(double *_float, double _lowBound = 0.0, double _highBound = 1.0e4 );
	void RegisterString		(char *, int _bufSize);
	void RegisterUnicodeString( UnicodeString &_string );
	void RegisterNetworkValue( int _type, NetworkValue * value );

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
    double		m_mouseDownStartTime;
    
public:
    
    InputScroller();

	virtual void Render     ( int realX, int realY, bool highlighted, bool clicked );
    void MouseDown  ();
    void MouseUp    ();

protected:
	void CheckInput( int realX, int realY );
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

