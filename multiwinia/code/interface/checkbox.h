#ifndef _included_checkbox_h
#define _included_checkbox_h

#include "interface/darwinia_window.h"

class NetworkInt;
class NetworkBool;

class CheckBox : public DarwiniaButton
{
public:

    NetworkBool *m_networkBool;
	bool m_allocated;

public:
    CheckBox();
	~CheckBox();

    void Render         ( int realX, int realY, bool highlighted, bool clicked );
    void MouseUp        ();

    void RegisterBool   ( bool *_bool );
    void RegisterInt    ( int *_int );
    void RegisterNetworkInt ( NetworkInt *_int );
	void RegisterNetworkBool( NetworkBool *_bool );

    void Toggle();
    void Set( int value );
    void Set( bool value );

    bool IsChecked      ();
};

#endif