#include "lib/universal_include.h"

#include <stdio.h>
#include <unrar.h>

#include "lib/filesys/binary_stream_readers.h"
#include "lib/bitmap.h"
#include "lib/debug_utils.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/preferences.h"
#include "lib/hi_res_time.h"
#include "lib/runnable.h"
#include "lib/sphere_renderer.h"

#include "lib/unicode/unicode_text_stream_reader.h"

#include "sound/sound_stream_decoder.h"
#include "sound/soundsystem.h"

#include "app.h"
#include "gesture_demo.h"
#include "landscape_renderer.h"
#include "location.h"
#include "renderer.h"
#include "water.h"
#include "loading_screen.h"
#include "multiwiniahelp.h"
#include "animatedpanel_renderer.h"


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
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
	m_gestureDemos.EmptyAndDelete();
#endif
	m_resourceFiles.EmptyAndDelete();
}


void Resource::ParseArchive( char const *_dataFile, char const *_password )
{
	if (!DoesFileExist( _dataFile))
		return;

	double now = GetHighResTime();
	AppDebugOut("Loading archive: %s\n", _dataFile);

	const char *dataFile = _dataFile;

	UncompressedArchive	*mainData = NULL;

	try
	{
		mainData = new UncompressedArchive(dataFile, _password);
	}
	catch( ... )
	{
		return;
	}

	NetLockMutex lock( m_mutex );
	int numFiles = mainData->m_numFiles;

	for (int i = 0; i < numFiles; ++i)
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

	AppDebugOut("File %s, %d files (%f seconds)\n", 
		_dataFile,
		numFiles,
		GetHighResTime() - now );
}


void Resource::AddBitmap( char const *_name, BitmapRGBA const &_bmp )
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
	BitmapRGBA *bmp = m_bitmaps.GetData(_name);
	if (bmp) return bmp;

	// If we still didn't find it, try to load it from a file on the disk
	char fullPath[512];
	sprintf( fullPath, "%s", _name );
	strlwr(fullPath);
	BinaryReader *reader = GetBinaryReader(fullPath);

	if( reader )
	{
		char const *extension = GetExtensionPart(fullPath);
		BitmapRGBA *bmp = new BitmapRGBA(reader, extension);
		m_bitmaps.PutData(_name, bmp );
		delete reader;
		return bmp;	
	}	

	return NULL;
}


MemMappedFile *Resource::GetUncompressedFile(char const *_filename)
{
	NetLockMutex lock(m_mutex);
	MemMappedFile *result = m_resourceFiles.GetData(_filename);

	return result;
}


TextReader *Resource::GetTextReader(std::string const &_filename)
{
	return GetTextReader( _filename.c_str() );
}

TextReader *Resource::GetTextReader(char const *_filename)
{
	//AppDebugOut("Load resource: %s\n", _filename);

	TextReader *reader = NULL;
    char fullFilename[256];

#ifdef USE_DARWINIA_MOD_SYSTEM

    if( m_modName )
    {
        sprintf( fullFilename, "%smods/%s/%s", g_app->GetProfileDirectory(), m_modName, _filename );
        if( DoesFileExist(fullFilename) )
		{
			reader = new UnicodeTextFileReader( fullFilename ); 

			if (reader && !reader->IsUnicode())
			{
				delete reader;
				reader = new TextFileReader( fullFilename ); 
			}
		}
    }

#ifdef TARGET_OS_VISTA

    if( !reader )
    {
        sprintf( fullFilename, "mods/%s/%s", m_modName, _filename );
        if( DoesFileExist(fullFilename) )
		{
			reader = new UnicodeTextFileReader( fullFilename ); 

			if (reader && !reader->IsUnicode())
			{
				delete reader;
				reader = new TextFileReader( fullFilename ); 
			}
		}
    }

#endif // TARGET_OS_VISTA

#endif // USE_DARWINIA_MOD_SYSTEM

    if( !reader )
    {
        sprintf( fullFilename, "data/%s", _filename );
		if (DoesFileExist(fullFilename))
		{
			reader = new UnicodeTextFileReader(fullFilename);	

			if (reader && !reader->IsUnicode())
			{
				delete reader;
				reader = new TextFileReader(fullFilename);
			}
		}
    }

	if( !reader )
	{
        sprintf( fullFilename, "data/%s", _filename );
		MemMappedFile *mmfile = GetUncompressedFile(fullFilename);
		if (mmfile) 
		{
			reader = new UnicodeTextDataReader((char*)mmfile->m_data, mmfile->m_size, _filename);	
			if (reader && !reader->IsUnicode())
			{
				delete reader;
				reader = new TextDataReader((char*)mmfile->m_data, mmfile->m_size, _filename);
			}
		}	
	}	

	if ( !reader )
	{
		// LEANDER : Uncomment this. Well, not this line, the next line. Then remove this line.
		AppDebugOut("Failed to find text resource: %s\n", fullFilename);
	}

	return reader;
}

BinaryReader *Resource::GetBinaryReader(char const *_filename)
{
	//AppDebugOut("Load resource: %s\n", _filename);
	BinaryReader *reader = NULL;
    char fullFilename[256];

#ifdef USE_DARWINIA_MOD_SYSTEM

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

#endif // TARGET_OS_VISTA

#endif // USE_DARWINIA_MOD_SYSTEM

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

	if ( !reader )
	{
		//AppDebugOut("Failed to find binary resource: %s\n", fullFilename);
	}

	return reader;
}

Resource::TextureInfo::TextureInfo( int _id, int _width, int _height )
:	m_id( _id ),
	m_width( _width ),
	m_height( _height )
{
}

Resource::TextureInfo *Resource::ProcessBmp( BitmapRGBA &_bmp, int _flags )
{
	if (_flags & WithMask) 
		_bmp.ConvertPinkToTransparent();

	if( _flags & WithAlpha )
		_bmp.ConvertColourToAlpha();

	bool withMipmaps = (_flags & WithMipmaps) != 0;
	bool withCompression = (_flags & WithCompression) != 0;

	return new TextureInfo( 
		_bmp.ConvertToTextureAsync( withMipmaps, withCompression ),
		_bmp.m_width, 
		_bmp.m_height );
}


int Resource::GetTextureWithFlags( const char *_name, int _flags )
{
	bool loadingScreenRendering = g_loadingScreen->IsRendering();
	TextureInfo *texInfo = NULL;

	if( loadingScreenRendering )
		m_mutex.Lock();

    // First lookup this name in the BTree of existing textures
	TextureHash::iterator it = m_textures.find(_name);
	if (it != m_textures.end())
		texInfo = it->second;
	
	if( loadingScreenRendering )
		m_mutex.Unlock();

	// If the texture wasn't there, then look in our bitmap store
	if( !texInfo )
	{
		BitmapRGBA *bmp = m_bitmaps.GetData(_name);
		if (bmp)
		{
			texInfo = ProcessBmp( *bmp, _flags );
		}
	}

	// If we still didn't find it, try to load it from a file on the disk
    if( !texInfo )
    {
	    char fullPath[512];
        sprintf( fullPath, "%s", _name );
		strlwr(fullPath);

		char const *extension = GetExtensionPart(fullPath);

		BinaryReader *reader = GetBinaryReader(fullPath);
            		
		if( reader )
		{
			BitmapRGBA bmp(reader, extension);
			delete reader;

			texInfo = ProcessBmp( bmp, _flags );
		}
	}

    if( !texInfo )
    {
        char errorString[512];
        sprintf( errorString, "Failed to load texture %s", _name );
        AppReleaseAssert( false, errorString );
		return -1;
    }
   
    // We're potentially leaking a texture here, but making this fully threadsafe is a bit of work,
	// due to ProcessBmp() pushing work to the main thread.
	if( loadingScreenRendering )
		m_mutex.Lock();
	m_textures[_name] = texInfo;
	if( loadingScreenRendering )
		m_mutex.Unlock();
	
	return texInfo->m_id;
}


int Resource::GetTexture( char const *_name, bool _mipMapping, bool _masked, bool _compressed )
{
	return GetTextureWithFlags( _name, 
		(_mipMapping ? WithMipmaps		: 0) |
		(_masked	 ? WithMask			: 0 ) | 
		(_compressed ? WithCompression	: 0) );
}

int Resource::GetTextureWithAlpha( char const *_name )
{
	return GetTextureWithFlags( _name, WithAlpha | WithMipmaps );
}

bool Resource::GetTextureInfo( char const *_name, int &_width, int &_height )
{
	TextureHash::iterator i = m_textures.find( _name );

	if( i == m_textures.end() )
		return false;

	TextureInfo *texInfo = i->second;
	_width = texInfo->m_width;
	_height = texInfo->m_height;

	return true;
}

bool Resource::DoesTextureExist(char const *_name)
{
	NetLockMutex lock( m_mutex);
	
    // First lookup this name in the BTree of existing textures
	TextureHash::iterator i = m_textures.find( _name );
	if( i != m_textures.end() ) return true;

	// If the texture wasn't there, then look in our bitmap store
	BitmapRGBA *bmp = m_bitmaps.GetData(_name);
	if (bmp) return true;


	// If we still didn't find it, try to load it from a file on the disk
	char fullPath[512];
    sprintf( fullPath, "%s", _name );
	strlwr(fullPath);
    BinaryReader *reader = GetBinaryReader(fullPath);
    bool success = false;
    if( reader ) 
    {
        success = true;
    }
    delete reader;    

    return success;
}


void Resource::DeleteTexture(char const *_name)
{
	TextureHash::iterator i = m_textures.find( _name );
	if( i != m_textures.end() )
	{
		TextureInfo *texInfo = i->second;
		GLuint id = texInfo->m_id;
		m_textures.erase( i );
		glDeleteTextures(1, &id);
		
		delete texInfo;
	}
}


class DeleteTextureByName : public Job
{
public:
	DeleteTextureByName( Resource *_resource, char const *_name )
		:	m_resource( _resource ),
			m_name( _name )
	{
	}

protected:
	void Run()
	{
		m_resource->DeleteTexture( m_name );
	}

private:
	Resource   *m_resource;
	char const *m_name;
};


void Resource::DeleteTextureAsync(char const *_name)
{
	if( NetGetCurrentThreadId() == g_app->m_mainThreadId )
		DeleteTexture( _name );
	else 
	{
		DeleteTextureByName t( this, _name );
		g_loadingScreen->QueueJob( &t );
		t.Wait();
	}
}

bool Resource::DoesShapeExist(const char *_filename)
{
    if( m_shapes.GetData( _filename ) ) return true;

    char fullPath[512];
    sprintf( fullPath, "data/shapes/%s", _filename );
	strlwr(fullPath);
    if (DoesFileExist(fullPath)) return true;

	MemMappedFile *file = GetUncompressedFile( fullPath ); 
    if( file ) return true;
		
    return false;
}


void Resource::AddShape( Shape *_shape, char const *_name )
{
    if( !m_shapes.GetData( _name ) )
    {
        m_shapes.PutData( _name, _shape );
    }
}


Shape *Resource::GetShape( char const *_name, bool _makeNew )
{
	Shape *theShape = m_shapes.GetData( _name );

	// If we haven't loaded the shape before, or _makeNew is true, then
	// try to load it from the disk
    if( !theShape && _makeNew)
    {
		theShape = GetShapeCopy(_name, false);
		m_shapes.PutData( _name, theShape );
	}

    return theShape;
}


Shape *Resource::GetShapeCopy( char const *_name, bool _animating, bool _buildDisplayList )
{
	char fullPath[512];
	Shape *theShape = NULL;

#ifdef USE_DARWINIA_MOD_SYSTEM

    if( m_modName )
    {
        sprintf( fullPath, "%smods/%s/shapes/%s", g_app->GetProfileDirectory(), m_modName, _name );
        strlwr(fullPath);
        if (DoesFileExist(fullPath)) theShape = new Shape( fullPath, _animating, _buildDisplayList );		
    }

#ifdef TARGET_OS_VISTA

    if( !theShape )
    {
        sprintf( fullPath, "mods/%s/shapes/%s", m_modName, _name );
        strlwr(fullPath);
        if (DoesFileExist(fullPath)) theShape = new Shape( fullPath, _animating, _buildDisplayList );		
    }

#endif // TARGET_OS_VISTA

#endif // USE_DARWINIA_MOD_SYSTEM

    if( !theShape )
    {
        sprintf( fullPath, "data/shapes/%s", _name );
		strlwr(fullPath);
        if (DoesFileExist(fullPath)) theShape = new Shape( fullPath, _animating, _buildDisplayList );
    }

	// If it wasn't present on the disk, look in the data archive
	if( !theShape )
	{
		MemMappedFile *file = GetUncompressedFile( fullPath ); 
		AppReleaseAssert(file, "Couldn't find shape file %s", fullPath);
		TextDataReader reader((char*)file->m_data, file->m_size, file->m_filename);
		theShape = new Shape(&reader, _animating, _buildDisplayList);
	}

    AppReleaseAssert( theShape, "Couldn't create shape file %s", _name );
    return theShape;
}


#ifdef USE_SEPULVEDA_HELP_TUTORIAL
GestureDemo *Resource::GetGestureDemo(char const *_name)
{
	GestureDemo *demo = m_gestureDemos.GetData(_name);

	if (demo == NULL)
	{
	    char fullPath[512];
        sprintf( fullPath, "gesture_demos/%s", _name );
		TextReader *in = GetTextReader(fullPath);        
        
		AppReleaseAssert(in && in->IsOpen(), "Couldn't find mouse gesture demo %s", _name);
		demo = new GestureDemo(in);
		delete in;

		m_gestureDemos.PutData(_name, demo);
	}

	return demo;
}
#endif

bool Resource::SoundExists(char const* _soundName)
{
	bool exists = false;
	char buf[512];
	
	sprintf( buf, "sounds/%s.ogg", _soundName );
	if(exists = FileExists(buf)) return exists;
	
	sprintf( buf, "mwsounds/%s.ogg", _soundName );
	if(exists = FileExists(buf)) return exists;
	return false;
}

SoundStreamDecoder *Resource::GetSoundStreamDecoder(char const *_filename)
{
	char buf[256];
	sprintf( buf, "%s.ogg", _filename );
	BinaryReader *binReader = GetBinaryReader(buf);
		
	if(!binReader || !binReader->IsOpen() )
    {
        if( binReader )
        {
            delete binReader;
        }
        return NULL;
    }
	
	SoundStreamDecoder *ssd = new SoundStreamDecoder(binReader);
	if(!ssd)
    {
        return NULL;
    }

	//AppDebugOut("GetSoundStreamDecoder: %s\n", _filename );
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

int Resource::CreateDisplayList(char const *_name )
{
	unsigned int id = glGenLists(1);

	// NULL _name indicates an anonymous display list
	if( _name )
	{
		// Make sure name isn't NULL and isn't too long
		AppDebugAssert( strlen(_name) < 20 );

		m_displayLists.PutData(_name, id);
	}

	return id;
}

int Resource::CreateDisplayList(char const *_name, const Runnable &_render, const Runnable &_before, const Runnable &_after )
{	
	unsigned int id = CreateDisplayList( _name );

	_before.Run();
	
	glNewList( id, GL_COMPILE );
	_render.Run();
	glEndList();

	_after.Run();

	return id;
}

class DisplayListToCreate : public Job
{
public:
	DisplayListToCreate( char const *_name, const Runnable &_render, const Runnable &_before, const Runnable &_after )
		:	m_name( _name ),
			m_render( _render ),
			m_before( _before ),
			m_after( _after ),
			m_displayListId( -1 )
	{
	}

	int WaitId()
	{
		Wait();
		return m_displayListId;
	}

protected:

	void Run()
	{
		//AppDebugOut( "Creating Display List in Loading Thread\n" );
		m_displayListId = g_app->m_resource->CreateDisplayList( m_name, m_render, m_before, m_after );
	}

private:
	char const		*m_name;
	const Runnable	&m_render;
	const Runnable	&m_before; 
	const Runnable	&m_after;
	int				m_displayListId;
};

int Resource::CreateDisplayListAsync(char const *_name, const Runnable &_render, const Runnable &_before, const Runnable &_after )
{
#ifdef USE_DIRECT3D_NOTTRUEANYMORE // JAKHACK
	// Our Direct3D implementation of display lists works nicely from any thread
	return CreateDisplayList( _name, _render, _before, _after );
#else
	if( NetGetCurrentThreadId() == g_app->m_mainThreadId )
	{
		return CreateDisplayList( _name, _render, _before, _after );
	}
	else {
		DisplayListToCreate t( _name, _render, _before, _after );
		g_loadingScreen->QueueJob( &t );
		return t.WaitId();
	}
#endif
}

int Resource::GetDisplayList(char const *_name)
{
	// Make sure name isn't NULL and isn't too long
	AppDebugAssert(_name && strlen(_name) < 20);

	return m_displayLists.GetData(_name, -1);
}

class DeleteDisplayListByName : public Job
{
public:
	DeleteDisplayListByName( char const *_name )
		:	m_name( _name )
	{
	}

protected:
	void Run()
	{
		//AppDebugOut( "Deleting Display List in Loading Thread\n" );
		g_app->m_resource->DeleteDisplayList( m_name );
	}

private:
	char const		*m_name;
};

class DeleteDisplayListById : public Job
{
public:
	DeleteDisplayListById( int _id )
		:	m_id( _id )
	{
	}

protected:
	void Run()
	{
		//AppDebugOut( "Deleting Display List in Loading Thread\n" );
		glDeleteLists( m_id, 1 );
	}

private:
	int	m_id;
};

void Resource::DeleteDisplayListAsync( int _id )
{
#ifdef USE_DIRECT3D
	// Our Direct3D implementation of display lists works nicely from any thread
	glDeleteLists( _id, 1 );
#else
	if( NetGetCurrentThreadId() == g_app->m_mainThreadId )
	{
		glDeleteLists( _id, 1 );
	}
	else {
		DeleteDisplayListById t( _id );
		g_loadingScreen->QueueJob( &t );
		t.Wait();
	}
#endif
}

void Resource::DeleteDisplayListAsync(char const *_name)
{
#ifdef USE_DIRECT3D
	// Our Direct3D implementation of display lists works nicely from any thread
	DeleteDisplayList( _name );
#else
	if( NetGetCurrentThreadId() == g_app->m_mainThreadId )
	{
		DeleteDisplayList( _name );
	}
	else {
		DeleteDisplayListByName t( _name );
		g_loadingScreen->QueueJob( &t );
		t.Wait();
	}
#endif
}

void Resource::DeleteDisplayList(char const *_name)
{
	if (!_name) return;

	// Make sure name isn't too long
	AppDebugAssert(strlen(_name) < 20);

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
		// Tell OpenGL to delete the textures
		for( TextureHash::iterator i = m_textures.begin(); i != m_textures.end(); i++ )
		{
			TextureInfo *texInfo = i->second;
			GLuint id = texInfo->m_id;
			glDeleteTextures(1, &id);
			delete texInfo;
		}

		m_textures.clear();

#endif

	// Tell all the shapes to delete the display list
	for (int i = 0; i < m_shapes.Size(); ++i)
	{
		if (!m_shapes.ValidIndex(i)) continue;

		m_shapes[i]->FlushDisplayList();
	}

	// Tell OpenGL to delete the display lists
	for (int i = 0; i < m_displayLists.Size(); ++i)
	{
		if (m_displayLists.ValidIndex(i)) 
		{
			glDeleteLists(m_displayLists[i], 1);
		}
	}

	// Forget all the display lists
	m_displayLists.Empty();

	if (g_app->m_location)
	{
		g_app->m_location->FlushOpenGlState();
	}
}


void Resource::RegenerateOpenGlState()
{
	// Tell the text renderers to reload their font
	if (g_editorFont.IsUnicode())
	{
		g_editorFont.BuildUnicodeArray();
	}
	else
	{
		g_editorFont.BuildOpenGlState();
	}

	if (g_gameFont.IsUnicode())
	{
		g_gameFont.BuildUnicodeArray();
	}
	else
	{
		g_gameFont.BuildOpenGlState();
	}

	g_titleFont.BuildUnicodeArray();
	
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

	// Get the loading screen Darwinian sprite texture id
	g_loadingScreen->m_texId = GetTexture( "sprites/darwinian.bmp" );

    Sphere::s_regenerateDisplayList = true;

	if( g_app->m_multiwiniaHelp->m_animatedPanel )
	{
		float scale = g_app->m_renderer->ScreenH()/1300.0f;    

		g_app->m_multiwiniaHelp->m_animatedPanel->m_screenX = g_app->m_renderer->ScreenW()/2.0f - scale * g_app->m_multiwiniaHelp->m_animatedPanel->m_width/2.0f;
		g_app->m_multiwiniaHelp->m_animatedPanel->m_screenY = g_app->m_renderer->ScreenH() -  g_app->m_multiwiniaHelp->m_animatedPanel->m_height * 1.15f * scale;
		g_app->m_multiwiniaHelp->m_animatedPanel->m_screenW = g_app->m_multiwiniaHelp->m_animatedPanel->m_width * scale;
		g_app->m_multiwiniaHelp->m_animatedPanel->m_screenH = g_app->m_multiwiniaHelp->m_animatedPanel->m_height * scale;

		g_app->m_multiwiniaHelp->m_animatedPanel->m_borderSize = 5;
		g_app->m_multiwiniaHelp->m_animatedPanel->m_titleH = 25;
		g_app->m_multiwiniaHelp->m_animatedPanel->m_captionH = 15;
	}
}


char *Resource::GenerateName()
{
	int digits = log10f(m_nameSeed) + 1;
	char *name = new char [digits + 1];
	snprintf(name, digits+1, "%d", m_nameSeed);
	m_nameSeed++;

	return name;
}


void Resource::LoadMod( char const *_modName )
{
#ifndef DEMOBUILD
#ifndef PURITY_CONTROL
#ifdef USE_DARWINIA_MOD_SYSTEM
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
		const char* lang = g_app->GetDefaultLanguage();
        g_app->SetLanguage( g_prefsManager->GetString( "TextLanguage", lang ), false );
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
#endif // USE_DARWINIA_MOD_SYSTEM
#endif // !PURITY_CONTROL
#endif // !DEMOBUILD
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


TextFileWriter *Resource::GetTextFileWriter( char const *_filename, bool _encrypt )
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
            AppReleaseAssert( result, "Failed to write to %s", fullFilename );
            *nextSlash = '/';
            ++nextSlash;
        }

        return new TextFileWriter( fullFilename, _encrypt );
    }
    
    sprintf( fullFilename, "data/%s", _filename );
    return new TextFileWriter( fullFilename, _encrypt );
}


bool Resource::FileExists( const char *_file )
{
	BinaryReader *r = GetBinaryReader( _file );

	if( !r )
		return false;

	delete r;
	return true;
}

// Implemented in SystemIV\lib\filesys\file_system.cpp
void OrderedInsert( LList<char *> *_llist, const char *_newString );

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


#ifdef USE_DARWINIA_MOD_SYSTEM

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
		delete modResults;

#ifdef TARGET_OS_VISTA
        sprintf( fullDirectory, "mods/%s/%s", m_modName, _dir );
        modResults = ListDirectory( fullDirectory, _filter, _longResults );
        for( int i = 0; i < modResults->Size(); ++i )
        {
            char *thisResult = modResults->GetData(i);
            OrderedInsert( results, thisResult );
        }
        modResults->EmptyAndDelete();
		delete modResults;
#endif // TARGET_OS_VISTA
    }

#endif // USE_DARWINIA_MOD_SYSTEM

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

		DArray <const char *> unfilteredResults;

		for( int i = 0 ; i < m_resourceFiles.Size(); i++ )
		{
			if( m_resourceFiles.ValidIndex( i ) )
			{
				unfilteredResults.PutData( m_resourceFiles.GetName( i ) );
			}
		}

		for (unsigned int i = 0; i < unfilteredResults.Size(); ++i)
		{
			if (!unfilteredResults.ValidIndex(i)) continue;

			const char *fullname = unfilteredResults.GetData(i);
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
	}
    
    return results;
}
