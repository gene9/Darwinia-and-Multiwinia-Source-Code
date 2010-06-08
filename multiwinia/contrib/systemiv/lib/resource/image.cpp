#include "lib/universal_include.h"
  
#include <stdio.h>

#include "lib/render/colour.h"
#include "lib/filesys/file_system.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/filesys/binary_stream_readers.h"

#include "bitmap.h"
#include "image.h"


Image::Image( char *filename )
:   m_textureID(-1),
    m_mipmapping(false)
{
    BinaryReader *in = g_fileSystem->GetBinaryReader(filename);

    if( in && in->IsOpen() )
    {
        char *extension = (char *)GetExtensionPart(filename);
        AppAssert(stricmp(extension, "bmp") == 0);
        m_bitmap = new Bitmap(in, extension);
        delete in;
    }
    else
    {
        // Failed to load the bitmap, so create an obvious ERROR bitmap
        m_bitmap = new Bitmap(32,32);        
        m_bitmap->Clear( Colour(255,0,0) );

        for( int x = 1; x < 30; ++x )
        {
            m_bitmap->PutPixel( x, x, Colour(0,255,0) );
            m_bitmap->PutPixel( x-1, x, Colour(0,255,0) );
            m_bitmap->PutPixel( x+1, x, Colour(0,255,0) );

            m_bitmap->PutPixel( 31-x, x, Colour(0,255,0) );
            m_bitmap->PutPixel( 31-x-1, x, Colour(0,255,0) );
            m_bitmap->PutPixel( 31-x+1, x, Colour(0,255,0) );
        }
    }
}


Image::Image( Bitmap *_bitmap )
:   m_textureID(-1),
    m_mipmapping(false),
    m_bitmap(_bitmap)
{
}

Image::~Image()
{
    if( m_textureID > -1 )
    {
        glDeleteTextures( 1, (GLuint *) &m_textureID );
    }
}

int Image::Width()
{
    return m_bitmap->m_width;
}

int Image::Height()
{
    return m_bitmap->m_height;
}


void Image::MakeTexture( bool mipmapping, bool masked )
{
    if( m_textureID == -1 )
    {
        m_mipmapping = mipmapping;
        if( masked ) m_bitmap->ConvertPinkToTransparent();
        m_textureID = m_bitmap->ConvertToTexture(mipmapping);
    }
}

Colour Image::GetColour( int pixelX, int pixelY )
{
    return m_bitmap->GetPixelClipped( pixelX, pixelY );
}
