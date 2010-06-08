
#ifndef _included_demoendsequence_h
#define _included_demoendsequence_h

#include "lib/vector3.h"
#include "lib/llist.h"


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
    ~DemoEndSequence();

    void Render();
    
};


#endif
