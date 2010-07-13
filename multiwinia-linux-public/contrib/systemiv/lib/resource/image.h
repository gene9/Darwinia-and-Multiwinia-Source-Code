
/*
 * =====
 * IMAGE
 * =====
 *
 * A wrapper class for Bitmap Image objects
 * that have been converted into Textures
 *
 */


#ifndef _included_image_h
#define _included_image_h

class Bitmap;

#include "lib/resource/resource.h"
#include "lib/render/colour.h"


class Image
{
public:
    Bitmap  *m_bitmap;
    int     m_textureID;
    bool    m_mipmapping;
    
public:
    Image( char *filename );
    Image( Bitmap *_bitmap );
    ~Image();

    int Width();
    int Height();    

    void MakeTexture( bool mipmapping, bool masked );
    
    Colour GetColour( int pixelX, int pixelY );
};



#endif
