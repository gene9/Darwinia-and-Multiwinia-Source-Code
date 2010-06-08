#include "lib/universal_include.h"

#include <fstream>
#include <sstream>
#include <string>

#include <zlib.h>

#include "lib/tosser/directory.h"

#include "lib/filesys/text_file_writer.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/filesys_utils.h"

#include "lib/resource.h"
#include "lib/bitmap.h"
#include "lib/network_stream.h"

#include "mapfile.h"

#include "app.h"
#include "location.h"
#include "level_file.h"

MapFile::MapFile()
:   m_filename(NULL),
    m_levelData(NULL),
    m_levelDataLength(0),
    m_landscapeTextureFilename(NULL),
    m_wavesTextureFilename(NULL),
    m_waterTextureFilename(NULL),
    m_thumbnailTextureFilename(NULL)
{
}

MapFile::MapFile( char *_filename, bool _loadTextures )
:   m_filename(NULL),
    m_levelData(NULL),
    m_levelDataLength(0),
    m_landscapeTextureFilename(NULL),
    m_wavesTextureFilename(NULL),
    m_waterTextureFilename(NULL),
    m_thumbnailTextureFilename(NULL)
{

    m_filename = strdup( _filename );

    std::ifstream input( m_filename, std::ios::in | std::ios::binary );

    int length = 0;

    input.seekg (0, std::ios::end);
    length = input.tellg();
    input.seekg (0, std::ios::beg);

    int ucLen = 0;
    ReadNetworkValue<int>( input, ucLen );

    char *data = new char[length];
    input.read( data, length );

    char *uncomp = new char[ucLen];
    uncompress( (Bytef *)uncomp, (uLongf *)&ucLen, (Bytef *)data, length );

    Directory *directory = new Directory();
    directory->Read(uncomp, ucLen);

    input.close();

    char *dirData = (char *)directory->GetDataVoid( MAPFILE_MAPDATA, &m_levelDataLength );
    m_levelData = new char[m_levelDataLength];
    memcpy( m_levelData, dirData, m_levelDataLength );

    if( _loadTextures )
    {
        LoadTextures( directory );
    }

    delete data;
    delete uncomp;
    delete directory;
}

MapFile::~MapFile()
{
    if( m_filename )                        SAFE_DELETE( m_filename );
    if( m_levelData)                        SAFE_DELETE( m_levelData );
    if( m_landscapeTextureFilename )        SAFE_DELETE( m_landscapeTextureFilename );
    if( m_wavesTextureFilename )            SAFE_DELETE( m_wavesTextureFilename );
    if( m_waterTextureFilename )            SAFE_DELETE( m_waterTextureFilename );
    if( m_thumbnailTextureFilename )        SAFE_DELETE( m_thumbnailTextureFilename );
}

TextReader *MapFile::GetTextReader()
{
    return new TextDataReader( m_levelData, m_levelDataLength, m_filename );
}

void MapFile::Save( char *_filename )
{
    m_filename = strdup( _filename );
    m_filename[ strlen( m_filename ) - 4 ] = '\0';
    sprintf( m_filename, "%s.mwm", m_filename );

    TextMemoryWriter *writer = new TextMemoryWriter();
    g_app->m_location->m_levelFile->Save(writer);

    Directory *directory = new Directory();
    directory->SetName("NewMap");

    m_levelDataLength = writer->GetStream()->str().size();
    m_levelData = new char[m_levelDataLength];
    memcpy(m_levelData, writer->GetStream()->str().data(), m_levelDataLength);
    directory->CreateData( MAPFILE_MAPDATA, m_levelData, m_levelDataLength );

    SaveTextures( directory );

    if( !IsDirectory(g_app->GetMapDirectory()) ) 
    {
        CreateDirectory(g_app->GetMapDirectory());
    }

    int length = 0;
    char *directoryOutput = directory->Write( length );

    char *compressedDir = new char[length];
    memset( compressedDir, 0, length );
    uLongf compLength = length;
    compress2( (Bytef*)compressedDir, &compLength, (Bytef*)directoryOutput, length, 9 );

    std::ofstream output( m_filename, std::ios::binary | std::ios::out );
    //directory->Write( output );
    WriteNetworkValue( output, length );
    output.write( compressedDir, compLength );
    output.close();

    delete directoryOutput;
    delete compressedDir;
    delete directory;
    delete writer;
}

bool MapFile::IsSaveRequired( char *_filename )
{
    // Run under the assumption that if the file exists as an Uncompressed file (ie, has been extracted from a dat file
    // then the file is included with the game and therefore doesn't need to be saved as part of the map
    if( g_app->m_resource->GetUncompressedFile( _filename ) )
    {
        return false;
    }
    else
    {
        return true;
    }
}

void MapFile::SaveTexture( Directory *_directory, char *_texName, char *_filename, int texNum )
{
    const BitmapRGBA *bitmap = g_app->m_resource->GetBitmap( _filename );
    if( bitmap )
    {
        int size = sizeof(RGBAColour) * bitmap->m_height * bitmap->m_width;
        char filenameIdentifier[512];
        sprintf( filenameIdentifier, "%sFilename", _texName );

        _directory->CreateData( filenameIdentifier, _filename );
        _directory->CreateData( _texName, bitmap->m_pixels, size);
        
        char textureinfo[256];
        sprintf( textureinfo, "%s%d", MAPFILE_TEXTUREHEIGHT, texNum );
        _directory->CreateData( textureinfo, bitmap->m_height );

        sprintf( textureinfo, "%s%d", MAPFILE_TEXTUREWIDTH, texNum );
        _directory->CreateData( textureinfo, bitmap->m_width );

    }
}

void MapFile::SaveTextures( Directory *_directory )
{
    int num = 1;
    char filename[512];

    sprintf( filename, "terrain/%s", g_app->m_location->m_levelFile->m_landscapeColourFilename );
    if( IsSaveRequired( filename ) )
    {
        SaveTexture( _directory, MAPFILE_LANDSCAPETEX, filename, num );
        num++;
    }

    sprintf( filename, "terrain/%s", g_app->m_location->m_levelFile->m_wavesColourFilename );
    if( IsSaveRequired( filename ) )
    {
        SaveTexture( _directory, MAPFILE_WAVESTEX, filename, num );
        num++;
    }

    sprintf( filename, "terrain/%s", g_app->m_location->m_levelFile->m_waterColourFilename );
    if( IsSaveRequired( filename ) )
    {
        SaveTexture( _directory, MAPFILE_WATERTEX, filename, num );
        num++;
    }

    if( strcmp( g_app->m_location->m_levelFile->m_thumbnailFilename, "default.bmp" ) != 0 )
    {
        sprintf( filename, "%s%s", g_app->GetMapDirectory(), g_app->m_location->m_levelFile->m_thumbnailFilename ); 
        SaveTexture( _directory, MAPFILE_THUMBNAIL, filename, num );
        num++;
    }
}

void MapFile::LoadTextures(Directory *_directory)
{
    int num = 1;
    if( LoadTexture( _directory, MAPFILE_LANDSCAPETEX, num, &m_landscapeTextureFilename ) ) num++;
    if( LoadTexture( _directory, MAPFILE_WAVESTEX, num, &m_wavesTextureFilename ) ) num++;
    if( LoadTexture( _directory, MAPFILE_WATERTEX, num, &m_waterTextureFilename ) ) num++;    
    if( LoadTexture( _directory, MAPFILE_THUMBNAIL, num, &m_thumbnailTextureFilename ) ) num++;
}

bool MapFile::LoadTexture(Directory *_directory, char *_texName, int _texNum, char **_newFilename )
{
    if( _directory->HasData( _texName ) )
    {
        char textureinfo[256];
        sprintf( textureinfo, "%s%d", MAPFILE_TEXTUREHEIGHT, _texNum );
        int height = _directory->GetDataInt( textureinfo );
        sprintf( textureinfo, "%s%d", MAPFILE_TEXTUREWIDTH, _texNum );
        int width = _directory->GetDataInt( textureinfo );

        if( width < 0 || height < 0 ) return true;

        BitmapRGBA *bitmap = new BitmapRGBA( width, height );
        int size = 0;

        RGBAColour *pixels = (RGBAColour *)_directory->GetDataVoid( _texName, &size );
        memcpy( bitmap->m_pixels, pixels, size );
        //for( int i = 0; i < height * width; ++i )
        {
            //bitmap->m_pixels[i] = pixels[i];
        }

        char filenameIdentifier[512];
        sprintf( filenameIdentifier, "%sFilename", _texName );

        *_newFilename = strdup(_directory->GetDataString( filenameIdentifier ));
        g_app->m_resource->AddBitmap( *_newFilename, *bitmap );

        return true;
    }

    return false;
}