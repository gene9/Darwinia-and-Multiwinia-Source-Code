#ifndef INCLUDED_KEYBINDINGS_WINDOW_H
#define INCLUDED_KEYBINDINGS_WINDOW_H


#include "lib/auto_vector.h"
#include "lib/input/input.h"
#include "interface/darwinia_window.h"


typedef auto_vector<InputDescription> InputDescList;


class PrefsKeybindingsWindow : public DarwiniaWindow
{
public:
	InputDescList m_bindings;
	int m_numMouseButtons;
    int m_controlMethod;

public:
    PrefsKeybindingsWindow();

    void Create();
    void Remove();

    void Render( bool _hasFocus );
};


#endif
