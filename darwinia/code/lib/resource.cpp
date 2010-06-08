#include "lib/universal_include.h"

#include <stdio.h>
#include <unrar.h>

#include "lib/binary_stream_readers.h"
#include "lib/bitmap.h"
#include "lib/debug_utils.h"
#include "lib/filesys_utils.h"
#include "lib/file_writer.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/text_stream_readers.h"
#include "lib/preferences.h"
#include "lib/hi_res_time.h"

#include "sound/sound_stream_decoder.h"
#include "sound/soundsystem.h"

#include "app.h"
#include "gesture_demo.h"
#include "landscape_renderer.h"
#include "location.h"
#include "renderer.h"
#include "water.h"


Resource::Resource()
:	m_nameSeed(1),
    m_modName(NULL)
{
}


Resource::~Resource()
{
	FlushOpenGlState();
	m_bitmaps.EmptyAndDelete();
	m_shapes.EmptyAndDelete();
	m_gestureDemos.EmptyAndDelete();
}


void Resource::ParseArchive( char const *_dataFile, char const *_password )
{
	if (!DoesFileExist( _dataFile))
		return;

	UncompressedArchive	*mainData = NULL;

	try
	{
		mainData = new UncompressedArchive(_dataFile, _password);
	}
	catch( ... )
	{
		return;
	}

	for (int i = 0; i < mainData->m_numFiles; ++i)
	{
		MemMappedFile *file = mainData->m_files[i];
		if (file->m_size > 0)
		{
			strlwr(file->m_filename);
			
			// Subsequent archives may override existing resources
			
			MemMappedFile *oldFile = m_resourceFiles.GetData(file->m_filename);
			if (oldFile) {
				m_resourceFiles.RemoveData(file->m_filename);
				delete oldFile;
			}
			
			m_resourceFiles.PutData(file->m_filename, file);
		}
	}

	delete mainData;
}


void Resource::AddBitmap( char const *_name, BitmapRGBA const &_bmp, bool _mipMapping )
{
	// Only insert if a bitmap with no other bitmap is already using that name
	if (m_bitmaps.GetIndex(_name) == -1)
	{
		BitmapRGBA *bmpCopy = new BitmapRGBA(_bmp);
		m_bitmaps.PutData(_name, bmpCopy);
	}
}


void Resource::DeleteBitmap(char const *_name)
{
	int index = m_bitmaps.GetIndex(_name);
	if (index >= 0)
	{
		BitmapRGBA *bmp = m_bitmaps.GetData(index);
		delete bmp;
		m_bitmaps.RemoveData(index);
	}
}


BitmapRGBA const *Resource::GetBitmap(char const *_name)
{
	return m_bitmaps.GetData(_name);
}


MemMappedFile *Resource::GetUncompressedFile(char const *_filename)
{
	return m_resourceFiles.GetData(_filename);
}


TextReader *Resource::GetTextReader(std::string const &_filename)
{
	return GetTextReader( _filename.c_str() );
}

TextReader *Resource::GetTextReader(char const *_filename)
{
	TextReader *reader = NULL;
    char fullFilename[256];

    if( m_modName )
    {
        sprintf( fullFilename, "%smods/%s/%s", g_app->GetProfileDirectory(), m_modName, _filename );
        if( DoesFileExist(fullFilename) ) 
			reader = new TextFileReader(fullFilename);

#ifdef TARGET_OS_VISTA
		// The Oberon build bundles the Perdition mod
		if( !reader )
		{
			sprintf( fullFilename, "mods/%s/%s", m_modName, _filename );
			if( DoesFileExist(fullFilename) ) 
				reader = new TextFileReader(fullFilename);
		}
#endif
	}

    if( !reader )
    {
        sprintf( fullFilename, "data/%s", _filename );
	    if (DoesFileExist(fullFilename)) 
			reader = new TextFileReader(fullFilename);	    
    }

	if( !reader )
	{
        sprintf( fullFilename, "data/%s", _filename );
		MemMappedFile *mmfile = GetUncompressedFile(fullFilename);
		if (mmfile) 
			reader = new TextDataReader((char*)mmfile->m_data, mmfile->m_size, fullFilename);		
	}

	return reader;
}


BinaryReader *Resource::GetBinaryReader(char const *_filename)
{
	BinaryReader *reader = NULL;
    char fullFilename[256];

    if( m_modName )
    {
        sprintf( fullFilename, "%smods/%s/%s", g_app->GetProfileDirectory(), m_modName, _filename );
        if( DoesFileExist( fullFilename) ) reader = new BinaryFileReader(fullFilename);
    }

#ifdef TARGET_OS_VISTA

    if( !reader )
    {
        sprintf( fullFilename, "mods/%s/%s", m_modName, _filename );
        if( DoesFileExist( fullFilename) ) reader = new BinaryFileReader(fullFilename);
    }

#endif

    if( !reader )
    {
		sprintf( fullFilename, "data/%s", _filename );
		if (DoesFileExist(fullFilename)) reader = new BinaryFileReader(fullFilename);
    }

    if( !reader )
	{
        sprintf( fullFilename, "data/%s", _filename );
		MemMappedFile *mmfile = GetUncompressedFile(fullFilename);
		if (mmfile) reader = new BinaryDataReader(mmfile->m_data, mmfile->m_size, fullFilename);		
	}

	return reader;
}


int Resource::GetTexture( char const *_name, bool _mipMapping, bool _masked )
{
    // First lookup this name in the BTree of existing textures
	int theTexture = m_textures.GetData( _name, -1 );

	// If the texture wasn't there, then look in our bitmap store
	if (theTexture == -1)
	{
		BitmapRGBA *bmp = m_bitmaps.GetData(_name);
		if (bmp)
		{
		    if (_masked) bmp->ConvertPinkToTransparent();
			theTexture = bmp->ConvertToTexture(_mipMapping);
		    m_textures.PutData(_name, theTexture);
		}
	}

	// If we still didn't find it, try to load it from a file on the disk
    if( theTexture == -1 )
    {
	    char fullPath[512];
        sprintf( fullPath, "%s", _name );
		strlwr(fullPath);
        BinaryReader *reader = GetBinaryReader(fullPath);
            		
        if( reader )
        {
		    char const *extension = GetExtensionPart(fullPath);
            BitmapRGBA bmp(reader, extension);
		    delete reader;

		    if( _masked ) bmp.ConvertPinkToTransparent();
		    theTexture = bmp.ConvertToTexture(_mipMapping);
		    m_textures.PutData(_name, theTexture);
        }
	}

    if( theTexture == -1 )
    {
        char errorString[512];
        sprintf( errorString, "Failed to load texture %s", _name );
        DarwiniaReleaseAssert( false, errorString );
    }
    
    return theTexture;
}


bool Resource::DoesTextureExist(char const *_name)
{
    // First lookup this name in the BTree of existing textures
	int theTexture = m_textures.GetData( _name, -1 );
    if( theTexture != -1 ) return true;


	// If the texture wasn't there, then look in our bitmap store
	BitmapRGBA *bmp = m_bitmaps.GetData(_name);
	if (bmp) return true;


	// If we still didn't find it, try to load it from a file on the disk
	char fullPath[512];
    sprintf( fullPath, "%s", _name );
	strlwr(fullPath);
    BinaryReader *reader = GetBinaryReader(fullPath);
    if( reader ) return true;

    

    return false;
}


void Resource::DeleteTexture(char const *_name)
{
	int id = m_textures.GetData(_name);
	if (id > 0)
	{
		unsigned int id2 = id;
		glDeleteTextures(1, &id2);
		m_textures.RemoveData(_name);
	}
}


Shape *Resource::GetShape( char const *_name )
{
	Shape *theShape = m_shapes.GetData( _name );

	// If we haven't loaded the shape before, or _makeNew is true, then
	// try to load it from the disk
    if( !theShape )
    {
		theShape = GetShapeCopy(_name, false);
		m_shapes.PutData( _name, theShape );
	}

    return theShape;
}


Shape *Resource::GetShapeCopy( char const *_name, bool _animating )
{
	char fullPath[512];
	Shape *theShape = NULL;

    if( m_modName )
    {
        sprintf( fullPath, "%smods/%s/shapes/%s", g_app->GetProfileDirectory(), m_modName, _name );
        strlwr(fullPath);
        if (DoesFileExist(fullPath)) theShape = new Shape( fullPath, _animating );		
    }

#ifdef TARGET_OS_VISTA

    if( !theShape )
    {
        sprintf( fullPath, "mods/%s/shapes/%s", m_modName, _name );
        strlwr(fullPath);
        if (DoesFileExist(fullPath)) theShape = new Shape( fullPath, _animating );		
    }

#endif

    if( !theShape )
    {
        sprintf( fullPath, "data/shapes/%s", _name );
		strlwr(fullPath);
        if (DoesFileExist(fullPath)) theShape = new Shape( fullPath, _animating );
    }

	// If it wasn't present on the disk, look in the data archive
	if( !theShape )
	{
		MemMappedFile *file = m_resourceFiles.GetData(fullPath);
		DarwiniaReleaseAssert(file, "Couldn't find shape file %s", fullPath);
		TextDataReader reader((char*)file->m_data, file->m_size, file->m_filename);
		theShape = new Shape(&reader, _animating);
	}

    DarwiniaReleaseAssert( theShape, "Couldn't create shape file %s", _name );
    return theShape;
}


GestureDemo *Resource::GetGestureDemo(char const *_name)
{
	GestureDemo *demo = m_gestureDemos.GetData(_name);

	if (demo == NULL)
	{
	    char fullPath[512];
        sprintf( fullPath, "gesture_demos/%s", _name );
		TextReader *in = GetTextReader(fullPath);        
        
		DarwiniaReleaseAssert(in && in->IsOpen(), "Couldn't find mouse gesture demo %s", _name);
		demo = new GestureDemo(in);
		delete in;

		m_gestureDemos.PutData(_name, demo);
	}

	return demo;
}

	
SoundStreamDecoder *Resource::GetSoundStreamDecoder(char const *_filename)
{
	char buf[256];
	sprintf( buf, "%s.ogg", _filename );
	BinaryReader *binReader = GetBinaryReader(buf);
		
	if(!binReader || !binReader->IsOpen() )
    {
        return NULL;
    }
	
	SoundStreamDecoder *ssd = new SoundStreamDecoder(binReader);
	if(!ssd)
    {
        return NULL;
    }

	return ssd;
}


int Resource::WildCmp(char const *wild, char const *string) 
{
	char const *cp;
	char const *mp;
	
	while ((*string) && (*wild != '*')) 
	{
		if ((*wild != *string) && (*wild != '?'))
		{
			return 0;
		}
		wild++;
		string++;
	}
	
	while (*string) 
	{
		if (*wild == '*')
		{
			if (!*++wild)
			{
				return 1;
			}
			mp = wild;
			cp = string + 1;
		}
		else if ((*wild == *string) || (*wild == '?'))
		{
			wild++;
			string++;
		}
		else 
		{
			wild = mp;
			string = cp++;
		}
	}
	
	while (*wild == '*') 
	{
		wild++;
	}
	return !*wild;
}


int Resource::CreateDisplayList(char const *_name)
{
	// Make sure name isn't NULL and isn't too long
	DarwiniaDebugAssert(_name && strlen(_name) < 20);

	unsigned int id = glGenLists(1);
	m_displayLists.PutData(_name, id);

	return id;
}


int Resource::GetDisplayList(char const *_name)
{
	// Make sure name isn't NULL and isn't too long
	DarwiniaDebugAssert(_name && strlen(_name) < 20);

	return m_displayLists.GetData(_name, -1);
}


void Resource::DeleteDisplayList(char const *_name)
{
	if (!_name) return;

	// Make sure name isn't too long
	DarwiniaDebugAssert(strlen(_name) < 20);

	int id = m_displayLists.GetData(_name,-1);
    if (id >= 0)
	{
		glDeleteLists(id, 1);
		m_displayLists.RemoveData(_name);
	}
}


void Resource::FlushOpenGlState()
{
#if 1 // Try to catch crash on shutdown bug
		// Tell OpenGL to delete the display lists
		for (int i = 0; i < m_displayLists.Size(); ++i)
		{
			if (m_displayLists.ValidIndex(i)) 
			{
				glDeleteLists(m_displayLists[i], 1);
			}
		}

		// Tell OpenGL to delete the textures
		for (int i = 0; i < m_textures.Size(); ++i)
		{
			if (m_textures.ValidIndex(i))
			{
				unsigned int id = i;
				glDeleteTextures(1, &id);
			}
		}
#endif

	// Forget all the display lists
	m_displayLists.Empty();

	// Forget all the texture handles
	m_textures.Empty();

	if (g_app->m_location)
	{
		g_app->m_location->FlushOpenGlState();
	}
}


void Resource::RegenerateOpenGlState()
{
	// Tell the text renderers to reload their font
	g_editorFont.BuildOpenGlState();
	g_gameFont.BuildOpenGlState();
	
	// Tell all the shapes to generate a new display list
	for (int i = 0; i < m_shapes.Size(); ++i)
	{
		if (!m_shapes.ValidIndex(i)) continue;

		m_shapes[i]->BuildDisplayList();
	}

	// Tell the renderer (for the pixel effect texture)
	g_app->m_renderer->BuildOpenGlState();

	// Tell the location
	if (g_app->m_location)
	{
		g_app->m_location->RegenerateOpenGlState();
	}
}


char *Resource::GenerateName()
{
	int digits = log10f(m_nameSeed) + 1;
	char *name = new char [digits + 1];
	itoa(m_nameSeed, name, 10);
	m_nameSeed++;

	return name;
}


void Resource::LoadMod( char const *_modName )
{
#ifndef DEMOBUILD
#ifndef PURITY_CONTROL
    bool modsEnabled = g_prefsManager->GetInt( "ModSystemEnabled", 0 ) != 0;
    if( modsEnabled )
    {
        if( m_modName )
        {
            delete m_modName;
            m_modName = NULL;
        }

        if( stricmp( _modName, "none" ) != 0 )
        {
            m_modName = strdup( _modName );
    
            FlushOpenGlState();
			RegenerateOpenGlState();
        }

        EntityBlueprint::Initialise();
        g_app->SetLanguage( g_prefsManager->GetString( "TextLanguage", "english" ), false );
        g_prefsManager->SetString( "Mod", _modName );
        g_prefsManager->Save();

        /*if( strcmp( g_app->m_userProfileName, "none" ) == 0 )
        {
            // new mod is being loaded - create a user profile to go with it
            g_app->SetProfileName( m_modName );
            g_app->LoadProfile();
            g_app->SaveProfile( true, false );
        }*/
    }
#endif
#endif
}


char *Resource::GetBaseDirectory()
{
    static char result[256];

    if( m_modName )
    {
        sprintf( result, "%smods/%s/", g_app->GetProfileDirectory(), m_modName );
    }
    else
    {      
        sprintf( result, "data/" );
    }

    return result;
}

const char *Resource::GetModName()
{
	return m_modName;
}

bool Resource::IsModLoaded()
{
    return( m_modName != NULL );
}


FileWriter *Resource::GetFileWriter( char const *_filename, bool _encrypt )
{
    char fullFilename[256];

    if( m_modName )
    {
        sprintf( fullFilename, "%smods/%s/%s", g_app->GetProfileDirectory(), m_modName, _filename );

        char *nextSlash = fullFilename;
        while( nextSlash = strchr( nextSlash, '/' ) )
        {
            *nextSlash = 0;
            bool result = CreateDirectory( fullFilename );
            DarwiniaReleaseAssert( result, "Failed to write to %s", fullFilename );
            *nextSlash = '/';
            ++nextSlash;
        }

        return new FileWriter( fullFilename, _encrypt );
    }
    
    sprintf( fullFilename, "data/%s", _filename );
    return new FileWriter( fullFilename, _encrypt );
}


// The string is copied and entered into llist
// in correct alphabetical order.
// The string wont be added if it is already present.
void OrderedInsert( LList<char *> *_llist, char *_newString )
{
    bool added = false;

    for( int i = 0; i < _llist->Size()-1; ++i )
    {
        char *thisString = _llist->GetData(i);
        if( strcmp( _newString, thisString ) == 0 )
        {
            // String duplicate
            return;
        }
        if( strcmp( _newString, thisString ) < 0 )
        {
            _llist->PutDataAtIndex( strdup(_newString), i );
            added = true;
            break;
        }
    }

    if( !added )
    {
        _llist->PutDataAtEnd( strdup(_newString) );
    }
}


// Finds all the filenames in the specified directory that match the specified
// filter. Directory should be like "textures" or "textures/".
// Filter can be NULL or "" or "*.bmp" or "map_*" or "map_*.txt"
// Set _longResults to true if you want results like "textures/blah.bmp" 
// or false for "blah.bmp"
LList <char *> *Resource::ListResources(char const *_dir, char const *_filter, bool _longResults /* = true */)
{
    LList<char *> *results = NULL;
    
    //
    // List the base data directory

    char fullDirectory[256];
    sprintf( fullDirectory, "data/%s", _dir );
    results = ListDirectory(fullDirectory, _filter, _longResults);


    //
    // List the mod directory

    if( m_modName )
    {
        sprintf( fullDirectory, "%smods/%s/%s", g_app->GetProfileDirectory(), m_modName, _dir );
        LList<char *> *modResults = ListDirectory( fullDirectory, _filter, _longResults );
        for( int i = 0; i < modResults->Size(); ++i )
        {
            char *thisResult = modResults->GetData(i);
            OrderedInsert( results, thisResult );
        }
        modResults->EmptyAndDelete();

#ifdef TARGET_OS_VISTA
        sprintf( fullDirectory, "mods/%s/%s", m_modName, _dir );
        modResults = ListDirectory( fullDirectory, _filter, _longResults );
        for( int i = 0; i < modResults->Size(); ++i )
        {
            char *thisResult = modResults->GetData(i);
            OrderedInsert( results, thisResult );
        }
        modResults->EmptyAndDelete();
#endif
    }

    //
    // List the pre-loaded resource files

    char desiredDir[256];
    sprintf( desiredDir, "data/%s", _dir );

	if (m_resourceFiles.Size() > 0)
	{
		if(_filter == NULL || _filter[0] == '\0')
		{
			_filter = "*";
		}

		DArray <char *> *unfilteredResults = m_resourceFiles.ConvertIndexToDArray();

		for (unsigned int i = 0; i < unfilteredResults->Size(); ++i)
		{
			if (!unfilteredResults->ValidIndex(i)) continue;

			char *fullname = unfilteredResults->GetData(i);
            char *dirPart = (char *) GetDirectoryPart( fullname );
            if( stricmp( dirPart, desiredDir ) == 0 )
            {
			    char *filePart = (char *) GetFilenamePart( fullname );
			    int result = WildCmp(_filter, filePart);
			    if (result != 0)
			    {
				    if( _longResults ) OrderedInsert(results, fullname);				
				    else   			   OrderedInsert(results, filePart);				
			    }
            }
		}

		delete unfilteredResults;
	}

    
    return results;
}
