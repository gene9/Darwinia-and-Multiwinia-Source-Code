
#ifndef _included_mainmenus_h
#define _included_mainmenus_h

#include "interface/darwinia_window.h"


class MainMenuWindow : public DarwiniaWindow
{
public:
    MainMenuWindow();

    void Create();
	void Render( bool _hasFocus );
};


class OptionsMenuWindow : public DarwiniaWindow
{
public:
    OptionsMenuWindow();

    void Create();
};


class LocationWindow : public DarwiniaWindow
{
public:
    LocationWindow();

    void Create();
};


class ResetLocationWindow : public DarwiniaWindow
{
public:
    ResetLocationWindow();

    void Create();
    void Render( bool _hasFocus );
};


class AboutDarwiniaWindow : public DarwiniaWindow
{
public:
    AboutDarwiniaWindow();

    void Create();
    void Render( bool _hasFocus );
};

class SkipPrologueWindow : public DarwiniaWindow
{
public:
	SkipPrologueWindow();

	void Create();
	void Render( bool _hasFocus );
};

class PlayPrologueWindow : public DarwiniaWindow
{
public:
	PlayPrologueWindow();

	void Create();
	void Render ( bool _hasFocus );
};

#endif