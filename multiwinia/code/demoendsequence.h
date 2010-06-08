
#ifndef _included_demoendsequence_h
#define _included_demoendsequence_h

#ifdef USE_SEPULVEDA_HELP_TUTORIAL

#include "lib/vector3.h"
#include "lib/tosser/llist.h"


struct DemoEndDarwinian
{
public:
    Vector3     m_pos;
    Vector3     m_up;
    Vector3     m_vel;
    float       m_turnRate;
    float       m_size;
};



class DemoEndSequence
{
protected:
    float   m_timer;
    float   m_newDarwinianTimer;
    bool    m_endDialogCreated;
    
    LList<DemoEndDarwinian *> m_darwinians;

public:
    DemoEndSequence();

    void Render();
    
};


#endif // USE_SEPULVEDA_HELP_TUTORIAL

#endif
