 
/*
 * ===============
 * RESOURCE SYSTEM
 * ===============
 *
 * Manages a cache of all game resources
 * Will load on request
 *
 */

#ifndef _included_resource_h
#define _included_resource_h

#include "lib/tosser/btree.h"

class Image;
class BitmapFont;



class Resource
{
protected:
    BTree   <Image *>           m_imageCache;
    BTree   <BitmapFont *>      m_bitmapFontCache;
    BTree   <unsigned int>      m_displayLists;

public:
    Resource();

    void            Restart();
    void            Shutdown();

    Image           *GetImage           ( const char *_filename );
    BitmapFont      *GetBitmapFont      ( const char *_filename );
    
    bool            GetDisplayList      ( const char *_name, unsigned int &_listId );                    // returns true if list was created
    void            DeleteDisplayList   ( const char *_name );

	void            Inject              ( const char *_filename, BitmapFont *_font );                    // insert a resource
};


extern Resource *g_resource;

#endif

