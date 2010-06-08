#ifndef _included_fodderloader_h
#define _included_fodderloader_h

#include "loaders/loader.h"


class FodderLoader : public Loader
{        
public:
    FodderLoader();
    ~FodderLoader();
    
    void Run();
};


#endif