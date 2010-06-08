
#ifndef _included_soulloader_h
#define _included_soulloader_h

#include "lib/universal_include.h"

#ifdef USE_LOADERS

#include "loaders/loader.h"

#include "lib/tosser/llist.h"

class SoulLoaderSpirit;


class SoulLoader : public Loader
{
public:
    float   m_startTime;
    float   m_time;
    float   m_spawnTimer;
    LList   <SoulLoaderSpirit *> m_spirits;

protected:
    void AdvanceAllSpirits( float _time );
    void RenderAllSpirits();
    void RenderMessage( float _time );
    
public:
    SoulLoader();

    void Run();
};



class SoulLoaderSpirit
{
public:
    Vector3     m_pos;
    float       m_positionOffset;                       // Used to make them float around a bit
    float       m_xaxisRate;
    float       m_yaxisRate;
    float       m_zaxisRate;
    RGBAColour  m_colour;
    float       m_life;

public:
    SoulLoaderSpirit();

    void Advance( float _time );
    void Render();
};

#endif // USE_LOADERS

#endif
