#ifndef _included_chatinput_window_h
#define _included_chatinput_window_h

#include "interface/darwinia_window.h"
#include "interface/input_field.h"


class ChatInputWindow : public DarwiniaWindow
{
public:
    char    m_currentChatMessage[256];

public:
    ChatInputWindow();

    void Create();
    void Render(bool _hasFocus );
    void Update();

    static bool IsChatVisible();
    static void CloseChat();
};

class ChatInputField : public InputField
{
public:
    ChatInputField()
    :   InputField()
    {
    }

    void MouseUp();
    void Render( int _realX, int _realY, bool highlighted, bool clicked );
    void Keypress( int _keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii );
};

#endif
