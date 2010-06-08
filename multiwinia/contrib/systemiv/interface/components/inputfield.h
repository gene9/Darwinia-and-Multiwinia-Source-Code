#ifndef INCLUDED_INPUT_FIELD_H
#define INCLUDED_INPUT_FIELD_H

#include "core.h"


// ****************************************************************************
// Class InputField
//
// This widget provides an editable text box where the user can type in numbers
// or short strings
// ****************************************************************************

class InputField: public EclButton
{
public:
	enum
	{
		TypeNowt,
		TypeChar,
		TypeInt,
		TypeFloat,
		TypeString
	};

	int		        m_type;
	char	        m_buf[256];
	unsigned char	*m_char;
	int		        *m_int;
	float	        *m_float;
	char	        *m_string;
	
	float			m_lowBound;
	float			m_highBound;

    int             m_align;                // -1=left, 0=centre, 1=right

    float           m_lastKeyPressTimer;

    InterfaceButton *m_callback;
    
public:
	InputField();
	
	virtual void Render		( int realX, int realY, bool highlighted, bool clicked );
	virtual void MouseUp    ();
	virtual void Keypress	( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii );

    void RegisterChar		(unsigned char *);
	void RegisterInt		(int *);
	void RegisterFloat		(float *);
	void RegisterString		(char *);

	void ClampToBounds		();

    void SetCallback        (InterfaceButton *button);              // This button will be clicked on Refresh

	void Refresh			();	                                    // Updates the display if the registered variable has changed
};



// ****************************************************************************
// Class InputScroller
// ****************************************************************************

class InputScroller : public InterfaceButton
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



#endif

