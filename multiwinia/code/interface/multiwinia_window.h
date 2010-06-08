#ifndef _included_multiwiniawindow_h
#define _included_multiwiniawindow_h

#include "interface/darwinia_window.h"
#include "mainmenus.h"

#include "multiwinia.h"

class LocalString;

class MultiwiniaWindow : public GameOptionsWindow
{
public:
    int     m_gameType;
    int     m_params[MULTIWINIA_MAXPARAMS];
    int     m_researchLevel[GlobalResearch::NumResearchItems];
    
    LocalString m_mapFilename;
    char        m_fileString[256];

public:
    MultiwiniaWindow();
    
    void Create();
    void Update();
};

class NewMapWindow : public GameOptionsWindow
{
public:
    LocalString m_mapFilename;
    char        m_fileString[256];

public:
    NewMapWindow();

    void Create();
};

#endif