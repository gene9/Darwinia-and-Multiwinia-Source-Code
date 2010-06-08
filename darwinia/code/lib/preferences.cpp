#include "lib/universal_include.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "app.h"

#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "lib/resource.h"
#include "lib/text_stream_readers.h"

#include "interface/prefs_other_window.h"

#ifdef TARGET_OS_MACOSX
#include "lib/macosx_hardware_detect.h"
#endif


PrefsManager *g_prefsManager = NULL;

static bool s_overwrite = false;

// ***************
// Class PrefsItem
// ***************

PrefsItem::PrefsItem()
:   m_key(NULL),
    m_str(NULL),
    m_int(0)
{
}


PrefsItem::PrefsItem(char *_line)
:	m_str(NULL)
{
	// Get key
	char *key = _line;
	while (!isalnum(*key) && *key != '\0')		// Skip leading whitespace
	{
		key++;
	}
	char *c = key;
	while (isalnum(*c))							// Find the end of the key word
	{
		c++;
	}
	*c = '\0';
	m_key = strdup(key);

	// Get value
	char *value = c + 1;
	while (isspace(*value) || *value == '=')
	{
		if (value == '\0') break;
		value++;
	}

	// Is value a number?
	if (value[0] == '-' || isdigit(value[0]))
	{
		// Guess that number is an int
		m_type = TypeInt;
		
		// Verify that guess
		c = value;
        int numDots = 0;
		while (*c != '\0')
		{
			if (*c == '.')
			{
                ++numDots;
			}
			++c;
		}
        if( numDots == 1 ) m_type = TypeFloat;
        else if(  numDots > 1 ) m_type = TypeString;


		// Convert string into a real number
		if      (m_type == TypeFloat)	    m_float = atof(value);
        else if (m_type == TypeString)      m_str = strdup(value);
		else						        m_int = atoi(value);
	}
	else
	{
		m_type = TypeString;
		m_str = strdup(value);
	}
}


PrefsItem::PrefsItem(char const *_key, char const *_str)
:	m_type(TypeString)
{
	m_key = strdup(_key);
	m_str = strdup(_str);
}


PrefsItem::PrefsItem(char const *_key, float _float)
:	m_type(TypeFloat),
	m_float(_float)
{
	m_key = strdup(_key);
}


PrefsItem::PrefsItem(char const *_key, int _int)
:	m_type(TypeInt),
	m_int(_int)
{
	m_key = strdup(_key);
}


PrefsItem::~PrefsItem()
{
	free(m_key);
	m_key = NULL;
	free(m_str);
	m_str = NULL;
}


// ******************
// Class PrefsManager
// ******************

PrefsManager::PrefsManager(char const *_filename)
{    
    m_filename = strdup(_filename);

	Load();

    if( GetInt("ControlMethod") == -1 ) AddLine( "ControlMethod = 1" );

}

PrefsManager::PrefsManager(std::string const &_filename)
{
    m_filename = strdup(_filename.c_str());

	Load();

    if( GetInt("ControlMethod") == -1 ) AddLine( "ControlMethod = 1" );
}


PrefsManager::~PrefsManager()
{
	free(m_filename);
	m_items.EmptyAndDelete();
	m_fileText.EmptyAndDelete();
}


bool PrefsManager::IsLineEmpty(char const *_line)
{
	while (_line[0] != '\0')
	{
		if (_line[0] == '#') return true;
		if (isalnum(_line[0])) return false;
		++_line;
	}

	return true;
}	

int GetDefaultHelpEnabled()
{
#ifndef DEMOBUILD
	return 1;
#else
	return 0;				// Demo has a tutorial instead
#endif
}

const char *GetDefaultSoundLibrary()
{
#ifdef HAVE_DSOUND
    return "dsound";
#else
	return "software";
#endif
}

int GetDefaultSoundDSP()
{
#ifdef DEMOBUILD
    return 0;
#elif defined(TARGET_OS_MACOSX)
	if (MacOSXSlowCPU())
		return 0;
	else
		return 1;
#else
	return 1;
#endif
}

int GetDefaultSoundChannels()
{
#ifdef TARGET_OS_MACOSX
	if (MacOSXSlowCPU())
		return 16;
	else
		return 32;
#else
	return 32;
#endif
}

int GetDefaultPixelShader()
{
#ifdef TARGET_OS_MACOSX
	// Add call to graphics card check here
	if (MacOSXGraphicsNoAcceleration() || MacOSXGraphicsLowMemory())
		return 0;
	else
		return 1;
#else
	return 1;
#endif
}

int GetDefaultGraphicsDetail()
{
#ifdef TARGET_OS_MACOSX
	if (MacOSXGraphicsNoAcceleration())
		return 3;
	else if (MacOSXGraphicsLowMemory())
		return 2;
	else
		return 1;
#else
	return 1;
#endif
}

void PrefsManager::CreateDefaultValues()
{
	char line[1024];
	
    AddLine( "ServerAddress = 127.0.0.1" );
    AddLine( "BypassNetwork = 1" );
    AddLine( "IAmAServer = 1" );

    AddLine( "\n" );

    AddLine( "TextLanguage = unknown" );

    AddLine( "TextSpeed = 15" );

	sprintf( line, "HelpEnabled = %d", GetDefaultHelpEnabled() );
	AddLine( line );
	
    AddLine( "\n" );
 
	sprintf( line, "SoundLibrary = %s", GetDefaultSoundLibrary() );
	AddLine( line );

    AddLine( "SoundMixFreq = 22050" );
    AddLine( "SoundMasterVolume = 255" );
	sprintf( line, "SoundChannels = %d", GetDefaultSoundChannels() );
	AddLine( line );
    AddLine( "SoundHW3D = 0" );
    AddLine( "SoundSwapStereo = 0" );
    AddLine( "SoundMemoryUsage = 1" );
    AddLine( "SoundBufferSize = 512" ); // Must be a power of 2 for Linux
	sprintf( line, "SoundDSP = %d", GetDefaultSoundDSP() );
	AddLine( line );
	
    AddLine( "\n" );

    //AddLine( "ScreenWidth = 1024" );
    //AddLine( "ScreenHeight = 768" );
    AddLine( "ScreenWindowed = 0" );
    AddLine( "ScreenZDepth = 24" );
    //AddLine( "ScreenColourDepth = 32" );
    //AddLine( "ScreenRefresh = 60" );

    AddLine( "\n" );
	
    sprintf( line, "RenderLandscapeDetail = %d", GetDefaultGraphicsDetail() );
	AddLine( line );
    sprintf( line, "RenderWaterDetail = %d", GetDefaultGraphicsDetail() );
	AddLine( line );
    sprintf( line, "RenderBuildingDetail = %d", GetDefaultGraphicsDetail() );
	AddLine( line );
    sprintf( line, "RenderEntityDetail = %d", GetDefaultGraphicsDetail() );
	AddLine( line );
    sprintf( line, "RenderCloudDetail = %d", GetDefaultGraphicsDetail() );
	AddLine( line );
		
	sprintf( line, "RenderPixelShader = %d", GetDefaultPixelShader() );
    AddLine( line );

    AddLine( "\n" );

#ifdef TARGET_OS_MACOSX
    AddLine( "ControlMouseButtons = 1" );
#else
    AddLine( "ControlMouseButtons = 3" );
#endif
    AddLine( "ControlMethod = 1" );

#if defined(TARGET_OS_LINUX) || defined(TARGET_OS_MACOSX)
	AddLine( "RenderLandscapeMode = 2" );
	AddLine( "ManuallyScaleTextures = 0" );
#endif

#ifdef DEMOBUILD
    #ifndef DEMO2
        AddLine( "StartMap = mine" );
        AddLine( "RenderSpecialLighting = 0" );
    #else
        AddLine( "UserProfile = DemoUser" );
        AddLine( "RenderSpecialLighting = 1" );
        AddLine( "StartMap = launchpad" );
    #endif
#else
    AddLine( "BootLoader = firsttime" );
    AddLine( "UserProfile = NewUser" ); 
    AddLine( "RenderSpecialLighting = 0" );
	AddLine( OTHER_DIFFICULTY " = 1" );
#endif

	// Override the defaults above with stuff from a default preferences file
	if ( g_app && g_app->m_resource ) 
	{
		TextReader *reader = g_app->m_resource->GetTextReader( "default_preferences.txt" );
		if ( reader && reader->IsOpen() ) 
		{
			while ( reader->ReadLine() ) 
			{
				AddLine( reader->GetRestOfLine(), true );
			}
		}
	}

}


void PrefsManager::Load(char const *_filename)
{
	if (!_filename) _filename = m_filename;

	m_items.EmptyAndDelete();
    
    // Try to read preferences if they exist
    FILE *in = fopen(_filename, "r");

    if( !in )
    {
        // Probably first time running the game
        CreateDefaultValues();
    }
    else
    {
	    char line[256];
	    while (fgets(line, 256, in) != NULL)
	    {
            AddLine( line );
        }
    	fclose(in);
    }
	
#ifdef DEMOBUILD
	AddLine( OTHER_DIFFICULTY " = 1", true );
#endif
}


void PrefsManager::SaveItem(FILE *out, PrefsItem *_item)
{
	switch (_item->m_type)
	{
		case PrefsItem::TypeFloat:
			fprintf(out, "%s = %.2f\n", _item->m_key, _item->m_float);
			break;
		case PrefsItem::TypeInt:
			fprintf(out, "%s = %d\n", _item->m_key, _item->m_int);
			break;
		case PrefsItem::TypeString:
			fprintf(out, "%s = %s\n", _item->m_key, _item->m_str);
			break;
	}
	_item->m_hasBeenWritten = true;
}

			
void PrefsManager::Save()
{
	// We've got a copy of the plain text from the prefs file that we initially
	// loaded in the variable m_fileText. We use that file as a template with
	// which to create the new save file. Updated prefs items values are looked up
	// as it their turn to be output comes around. When we've finished we then
	// write out all the new prefs items because they didn't exist in m_fileText.

	// First clear the "has been written" flags on all the items
	for (int i = 0; i < m_items.Size(); ++i)
	{
		if (m_items.ValidIndex(i)) 
		{
			m_items[i]->m_hasBeenWritten = false;
		}
	}

	// Now use m_fileText as a template to write most of the items
	FILE *out = fopen(m_filename, "w");
	
	// If we couldn't open the prefs file for writing then just silently fail - 
	// it's better than crashing.
	if (!out)
	{
		return;
	}

	for (int i = 0; i < m_fileText.Size(); ++i)
	{
		char const *line = m_fileText[i];
		if (IsLineEmpty(line))
		{
			fprintf(out, line);
		}
		else
		{
			char const *c = line;
			char const *keyStart = NULL;
			char const *keyEnd;
			while (*c != '=') 
			{
				if (keyStart)
				{
					if (!isalnum(c[0]))
					{
						keyEnd = c;
					}
				}
				else
				{
					if (isalnum(c[0]))
					{
						keyStart = c; 
					}
				}
				++c;
			}
			char key[128];
			int keyLen = keyEnd - keyStart;
			strncpy(key, keyStart, keyLen);
			key[keyLen] = '\0';
			int itemIndex = m_items.GetIndex(key);
			PrefsItem *item = m_items.GetData(itemIndex);
			SaveItem(out, item);
		}
	}

	// Finally output any items that haven't already been written
	for (int i = 0; i < m_items.Size(); ++i)
	{
		if (m_items.ValidIndex(i)) 
		{
			PrefsItem *item = m_items.GetData(i);
			if (!item->m_hasBeenWritten)
			{
				SaveItem(out, item);
			}
		}
	}

	fclose(out);
}


void PrefsManager::Clear()
{
	m_items.EmptyAndDelete();
	m_fileText.EmptyAndDelete();
}


float PrefsManager::GetFloat(char const *_key, float _default) const
{
	int index = m_items.GetIndex(_key);
	if (index == -1) return _default;
	PrefsItem *item = m_items.GetData(index);
	if (item->m_type != PrefsItem::TypeFloat) return _default;
	return item->m_float;
}


int PrefsManager::GetInt(char const *_key, int _default) const
{
	int index = m_items.GetIndex(_key);
	if (index == -1) return _default;
	PrefsItem *item = m_items.GetData(index);
	if (item->m_type != PrefsItem::TypeInt) return _default;
	return item->m_int;
}


char *PrefsManager::GetString(char const *_key, char *_default) const
{
	int index = m_items.GetIndex(_key);
	if (index == -1) return _default;
	PrefsItem *item = m_items.GetData(index);
	if (item->m_type != PrefsItem::TypeString) return _default;
	return item->m_str;
}


//void PrefsManager::GetData(char const *_key, void *_data, int _length) const
//{
//    unsigned char *data = (unsigned char *)_data;
//    char *stringData = GetString(_key);
//
//    if( stringData )
//    {
//        DarwiniaDebugAssert (_length * 2 == strlen(stringData));
//
//        for( int i = 0; i < _length; ++i )
//        {
//            data[i] = (stringData[i*2] - 'A') << 4;
//            data[i] |= (stringData[i*2+1] - 'A');
//        }
//    }
//}


void PrefsManager::SetString(char const *_key, char const *_string)
{
	int index = m_items.GetIndex(_key);

	if (index == -1)
	{
		PrefsItem *item = new PrefsItem(_key, _string);
		m_items.PutData(item->m_key, item);
	}
	else
	{
		PrefsItem *item = m_items.GetData(index);
		DarwiniaDebugAssert(item->m_type == PrefsItem::TypeString);
		char *newString = strdup(_string);
        free(item->m_str);
        // Note by Chris:
        // The incoming value of _string might also be item->m_str
        // So it is essential to copy _string before freeing item->m_str
		item->m_str = newString;
	}
}


void PrefsManager::SetFloat(char const *_key, float _float)
{
	int index = m_items.GetIndex(_key);

	if (index == -1)
	{
		PrefsItem *item = new PrefsItem(_key, _float);
		m_items.PutData(item->m_key, item);
	}
	else
	{
		PrefsItem *item = m_items.GetData(index);
		DarwiniaDebugAssert(item->m_type == PrefsItem::TypeFloat);
		item->m_float = _float;
	}
}


void PrefsManager::SetInt(char const *_key, int _int)
{
	int index = m_items.GetIndex(_key);

	if (index == -1)
	{
		PrefsItem *item = new PrefsItem(_key, _int);
		m_items.PutData(item->m_key, item);
	}
	else
	{
		PrefsItem *item = m_items.GetData(index);
		DarwiniaDebugAssert(item->m_type == PrefsItem::TypeInt);
		item->m_int = _int;
	}
}


void PrefsManager::AddLine(char const*_line, bool _overwrite)
{
	if ( !_line ) 
		return;

	bool saveLine = true;
    
	if (!IsLineEmpty(_line))				// Skip comment lines and blank lines
	{
		char *localCopy = strdup( _line );
		char *c = strchr(localCopy, '\n');
		if (c)
			*c = '\0';

		PrefsItem *item = new PrefsItem(localCopy);

		int idx = m_items.GetIndex( item->m_key );
		if ( _overwrite && idx >= 0 ) {
			delete m_items.GetData( idx );
			m_items.RemoveData( item->m_key );
			saveLine = false;
		}

		m_items.PutData(item->m_key, item);
		free(localCopy);
	}

	if ( saveLine ) {
		char *lineCopy = strdup(_line);
		m_fileText.PutData(lineCopy);
	}
}


//void PrefsManager::AddData(char const *_key, void *_data, int _length)
//{
//    char *newString = new char[_length*2 + 1];
//    unsigned char *data = (unsigned char *)_data;
//	int i;
//    for( i = 0; i < _length; ++i )
//    {
//        newString[i*2]      = 'A' + ((data[i] & 0xf0) >> 4);
//        newString[i*2+1]    = 'A' + (data[i] & 0xf);
//    }
//
//    newString[i*2] = '\0';
//
//    PrefsItem *item = new PrefsItem();
//    item->m_key = strdup(_key);
//    item->m_str = newString;
//    m_items.PutData(item->m_key, item);
//}


bool PrefsManager::DoesKeyExist(char const *_key)
{
	int index = m_items.GetIndex(_key);

	return index != -1;
}
