#ifndef _included_oberonloader_h
#define _included_oberonloader_h

#include "loaders/loader.h"


class OberonLoader : public Loader
{        
public:
    OberonLoader();
    ~OberonLoader();
    
    void Run();
};


#endif