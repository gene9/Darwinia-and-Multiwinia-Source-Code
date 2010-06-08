#ifndef _included_mapfile_h
#define _included_mapfile_h

#define MAPFILE_MAPDATA                 "MapData"
#define MAPFILE_LANDSCAPETEX            "MapLandscapeTexture"
#define MAPFILE_WAVESTEX                "MapWavesTexture"
#define MAPFILE_WATERTEX                "MapWaterTexture"
#define MAPFILE_TEXTUREWIDTH            "TextureWidth"
#define MAPFILE_TEXTUREHEIGHT           "TextureHeight"
#define MAPFILE_THUMBNAIL               "MapThumbnail"

class TextReader;

class MapFile
{
public:
    int     m_levelDataLength;
    char    *m_levelData;
    char    *m_filename;
    char    *m_landscapeTextureFilename;
    char    *m_thumbnailTextureFilename;
    char    *m_wavesTextureFilename;
    char    *m_waterTextureFilename;

public:
    MapFile();
    MapFile( char *filename, bool _loadTextures = false ); /* Filename must include the full path to the file */
    ~MapFile();

    TextReader  *GetTextReader();

    void        Save( char *_filename ); /* Filename must include the full path to the file */
    void        SaveTextures( Directory *directory );   /* Determine if any of the textures used on the map need to be saved and if they do, write them to the directory */
    void        SaveTexture( Directory *_directory, char *_texName, char *_filename, int texNum );

    void        LoadTextures( Directory *directory );   /* Pull out any textures from the Directory and stick them in the resource system */
    bool        LoadTexture( Directory *_directory, char *_texName, int _texNum, char **_newFilename );

    bool        IsSaveRequired( char *_filename );
};

#endif