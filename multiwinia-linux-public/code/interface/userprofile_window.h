
#ifndef _included_userprofilewindow_h
#define _included_userprofilewindow_h

#include "interface/darwinia_window.h"
#include "interface/mainmenus.h"


class UserProfileWindow : public GameOptionsWindow
{
public:
    UserProfileWindow();

    void Render ( bool hasFocus );
    void Create();    
};

class ResetProfileButton : public GameMenuButton
{
public:
    ResetProfileButton();

    void MouseUp();
};


class NewUserProfileWindow : public DarwiniaWindow
{
public:
    static char s_profileName[256];

public:
    NewUserProfileWindow();
    void Create();
};

#endif