#ifndef _included_fodderloader_h
#define _included_fodderloader_h

#include "lib/universal_include.h"

#ifdef USE_LOADERS

#include "loaders/loader.h"

class FodderLoader : public Loader
{        
public:
    FodderLoader();
    ~FodderLoader();
    
    void Run();
};

#endif // USE_LOADERS

#endif
