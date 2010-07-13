#ifndef _included_egamesloader_h
#define _included_egamesloader_h

#ifdef EGAMESBUILD

#include "loaders/loader.h"


class AviPlayer;



class EgamesLoader : public Loader
{
public:
    AviPlayer *m_aviPlayer;
    
public:
    EgamesLoader();

    void RenderDarwinians( Vector3 const &_screenPos,
                           Vector3 const &_screenRight,
                           Vector3 const &_screenUp );

    void Run();
};


#endif // EGAMESBUILD

#endif
