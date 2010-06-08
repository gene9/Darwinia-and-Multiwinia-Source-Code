
#ifndef _included_userprofilewindow_h
#define _included_userprofilewindow_h

#include "interface/darwinia_window.h"


class UserProfileWindow : public DarwiniaWindow
{
public:
    UserProfileWindow();

    void Render ( bool hasFocus );
    void Create();
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