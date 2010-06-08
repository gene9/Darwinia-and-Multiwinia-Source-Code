
#ifndef _included_prefsotherwindow_h
#define _included_prefsotherwindow_h

#include "interface/darwinia_window.h"
#include "interface/mainmenus.h"


class PrefsOtherWindow : public GameOptionsWindow
{
public:
    int     m_helpEnabled;
	int		m_controlHelpEnabled;
    int     m_bootLoader;
	int		m_usePortForwarding;
	int		m_serverPort;
	int	    m_clientPort;
    int     m_christmas;
    int     m_language;
	int		m_difficulty;
	int		m_largeMenus;
    int     m_automaticCamera;
    
    LList   <char *> m_languages;

public:
    PrefsOtherWindow();

    void Create();
    void Render( bool _hasFocus );

    void ListAvailableLanguages();
};

// Defines useful to reference preferences from 
// other parts of the program.
#define OTHER_HELPENABLED       "HelpEnabled"
#define OTHER_CONTROLHELPENABLED "ControlHelpEnabled"
#define OTHER_BOOTLOADER        "BootLoader"
#define OTHER_CHRISTMASENABLED  "ChristmasEnabled"
#define OTHER_LANGUAGE          "TextLanguage"
#define OTHER_DIFFICULTY		"Difficulty"
#define OTHER_LARGEMENUS		"LargeMenus"
#define OTHER_AUTOMATICCAM      "AutomaticCamera"
#define OTHER_USEPORTFORWARDING	PREFS_NETWORKUSEPORTFORWARDING
#define OTHER_SERVERPORT        PREFS_NETWORKSERVERPORT
#define OTHER_CLIENTPORT		PREFS_NETWORKCLIENTPORT

#endif