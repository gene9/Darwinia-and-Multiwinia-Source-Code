
#ifndef _included_clonelab_h
#define _included_clonelab_h

#include "worldobject/building.h"
#include "worldobject/incubator.h"
#include "worldobject/ai.h"

class CloneLab: public Incubator
{
public:
    AITarget    *m_aiTarget;
    int         m_startingSpirits;
    
public:
    CloneLab();
    ~CloneLab();

    void Initialise     ( Building *_template );
    void InitialiseAITarget();
    
    bool Advance        ();
    
    void RenderPorts    ();

    void Read           ( TextReader *_in, bool _dynamic );     
    void Write          ( TextWriter *_out );

    void RecalculateOwnership();
};


#endif
