#define _CRT_SECURE_NO_DEPRECATE
#include "lib/universal_include.h"

#include <stdio.h>
#include <fstream>

#include "lib/resource/bitmap.h"
#include "lib/resource/resource.h"
#include "lib/resource/bitmapfont.h"
#include "lib/resource/image.h"



Resource *g_resource = NULL;



Resource::Resource()
{
}

void Resource::Restart()
{
    Shutdown();
}

void Resource::Shutdown()
{
    //
    // Image Cache

    DArray<Image *> *images = m_imageCache.ConvertToDArray();
    for( int i = 0; i < images->Size(); ++i )
    {
        Image *image = images->GetData(i);
        delete image;
    }
    delete images;
    m_imageCache.Empty();


    //
    // BitmapFont cache

    DArray<BitmapFont *> *fonts = m_bitmapFontCache.ConvertToDArray();
    for( int i = 0; i < fonts->Size(); ++i )
    {
        BitmapFont *font = fonts->GetData(i);
        delete font;
    }
    delete fonts;
    m_bitmapFontCache.Empty();


    //
    // Display list cache

    DArray<unsigned int> *displayLists = m_displayLists.ConvertToDArray();
    for( int i = 0; i < displayLists->Size(); ++i )
    {
        unsigned int displayList = displayLists->GetData(i);
        glDeleteLists(displayList, 1);
    }
    delete displayLists;
    m_displayLists.Empty();
}


Image *Resource::GetImage( const char *filename )
{   
    if( !filename ) return NULL;
    
    char fullFilename[512];
    sprintf( fullFilename, "data/%s", filename );

    Image *image = m_imageCache.GetData( fullFilename );
    if( image )
    {
        return image;
    }
    else
    {
        image = new Image( fullFilename );
        m_imageCache.PutData( fullFilename, image );
        image->MakeTexture( true, true );
        return image;
    }
}


BitmapFont *Resource::GetBitmapFont ( const char *filename )
{
    if( !filename ) return NULL;

    char fullFilename[512];
    sprintf( fullFilename, "data/%s", filename );

    BitmapFont *font = m_bitmapFontCache.GetData( fullFilename );
    if( font )
    {
        return font;
    }
    else
    {
        font = new BitmapFont( fullFilename );
        m_bitmapFontCache.PutData( fullFilename, font );
        return font;
    }
}


bool Resource::GetDisplayList( const char *_name, unsigned int &_listId )
{
    BTree<unsigned int> *data = m_displayLists.LookupTree(_name);

    if( data )
    {
        _listId = data->data;
        return false;
    }

    _listId = glGenLists(1);
    m_displayLists.PutData( _name, _listId );

    return true;
}


void Resource::DeleteDisplayList( const char *_name )
{
    BTree<unsigned int> *data = m_displayLists.LookupTree(_name);

    if( data )
    {
        m_displayLists.RemoveData( _name );
    }
}


void Resource::Inject( const char *_filename, BitmapFont *_font )
{
    if( !_filename ) return;

    char fullFilename[512];
    sprintf( fullFilename, "data/%s", _filename );

    m_bitmapFontCache.PutData( fullFilename, _font );
}
