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
#include "lib/filesys/text_stream_readers.h"

#include "interface/prefs_other_window.h"
#include "lib/unicode/unicode_text_stream_reader.h"
#include "unicode/unicode_text_file_writer.h"

#ifdef TARGET_OS_MACOSX
#include "lib/macosx_hardware_detect.h"
#endif


PrefsManager *g_prefsManager = NULL;

static bool s_overwrite = false;

// ***************
// Class PrefsItem
// ***************

class PrefsItem
{
public:
	UnicodeString	m_key;
	
	enum
	{
		TypeString,
		TypeFloat,
		TypeInt
	};
	
	int				m_type;
	UnicodeString	m_str;
	union {
		int		m_int;
		float	m_float;
	};

	bool		m_hasBeenWritten;

	PrefsItem(UnicodeString _line=UnicodeString());
	PrefsItem(UnicodeString _key, UnicodeString _str);
	PrefsItem(UnicodeString _key, float _float);
	PrefsItem(UnicodeString _key, int _int);
	~PrefsItem();
};

#include <sstream>
typedef std::basic_string<wchar_t> WString;
typedef std::basic_istringstream<wchar_t> WIStringStream;

float wtof( const WString &x )
{
	WIStringStream i( x );
	float r;
	if( i >> r ) return r;
	else return 0.0;
}

int wtoi( const WString &x )
{
	WIStringStream i( x );
	int r;
	if( i >> r ) return r;
	else return 0.0;
}


PrefsItem::PrefsItem(UnicodeString _line)
:	m_str(UnicodeString())
{
	if( _line.m_unicodestring[0] == L'\0' )
	{
		m_type = TypeString;
		m_key = UnicodeString();
		return;
	}

	// Get key
	wchar_t *key = _line.m_unicodestring;
	while (!iswalnum(*key) && *key != L'\0')		// Skip leading whitespace
	{
		key++;
	}
	wchar_t *c = key;
	while (iswalnum(*c))							// Find the end of the key word
	{
		c++;
	}
	*c = L'\0';
	m_key = UnicodeString(key);

	// Get value
	wchar_t *value = c + 1;
	while (iswspace(*value) || *value == L'=')
	{
		if (value == L'\0') break;
		value++;
	}

	// Is value a number?
	if (value[0] == L'-' || iswdigit(value[0]))
	{
		// Guess that number is an int
		m_type = TypeInt;
		
		// Verify that guess
		c = value;
        int numDots = 0;
		while (*c != L'\0')
		{
			if (*c == L'.')
			{
                ++numDots;
			}
			++c;
		}
        if( numDots == 1 ) m_type = TypeFloat;
        else if(  numDots > 1 ) m_type = TypeString;


		// Convert string into a real number
		if      (m_type == TypeFloat)	    m_float = wtof(value);
        else if (m_type == TypeString)      m_str = UnicodeString(value);
		else						        m_int = wtoi(value); 
	}
	else
	{
		m_type = TypeString;
		m_str = UnicodeString(value);
	}
}


PrefsItem::PrefsItem(UnicodeString _key, UnicodeString _str)
:	m_type(TypeString)
{
	m_key = _key;
	m_str = _str;
}


PrefsItem::PrefsItem(UnicodeString _key, float _float)
:	m_type(TypeFloat),
	m_float(_float),
	m_str( UnicodeString() )
{
	m_key = _key;
}


PrefsItem::PrefsItem(UnicodeString _key, int _int)
:	m_type(TypeInt),
	m_int(_int),
	m_str( UnicodeString() )
{
	m_key = _key;
}


PrefsItem::~PrefsItem()
{	
	/*if( m_key )
	{
		free(m_key);
		m_key = NULL;
	}
	
	if( m_str )
	{
		free(m_str);
		m_str = NULL;
	}*/
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
}


bool PrefsManager::IsLineEmpty(UnicodeString const &_line)
{
	wchar_t* pos = _line.m_unicodestring;
	while (pos[0] != L'\0')
	{
		if (pos[0] == L'#') return true;
		if (iswalnum(pos[0])) return false;
		++pos;
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
#elif defined(HAVE_XAUDIO)
	return "xaudio";
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
	return 0;
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
	return 1;
}

void PrefsManager::CreateDefaultValues()
{
	char line[1024];
	
    AddLine( "ServerAddress = 127.0.0.1" );
    AddLine( "BypassNetwork = 0" );
    AddLine( "ServerPort = 4000" );

    AddLine( "\n" );

    AddLine( "TextLanguage = unknown" );

    AddLine( "TextSpeed = 15" );

	sprintf( line, "HelpEnabled = %d", GetDefaultHelpEnabled() );
	AddLine( line );
	
    AddLine( "\n" );
 
	sprintf( line, "SoundLibrary = %s", GetDefaultSoundLibrary() );
	AddLine( line );

#ifdef TARGET_OS_MACOSX
    AddLine( "SoundMixFreq = 44100" );
#else
	AddLine( "SoundMixFreq = 22050" );
#endif
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

    AddLine( "DarwinianScaleFactor = 2.00" );

#ifdef TARGET_OS_MACOSX
    AddLine( "ControlMouseButtons = 1" );
#else
    AddLine( "ControlMouseButtons = 3" );
#endif
    AddLine( "ControlMethod = 1" );
    AddLine( "MultiwiniaControlMethod = 1" );

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
    AddLine( "UserProfile = AccessAllAreas" ); 
    AddLine( "ScreenOverscan = 0" );
    AddLine( "RenderSpecialLighting = 0" );
	AddLine( OTHER_DIFFICULTY " = 1" );
#endif

	// Override the defaults above with stuff from a default preferences file
	if ( g_app && g_app->m_resource ) {
		TextReader *reader = g_app->m_resource->GetTextReader( "default_preferences.txt" );
		if ( reader && reader->IsOpen() ) 
		{
			s_overwrite = true;
			while ( reader->ReadLine() ) 
			{
				char* line = reader->GetRestOfLine();
				if( line ) AddLine( line );
			}
			s_overwrite = false;
		}
		delete reader;
	}

}


void PrefsManager::Load(char const *_filename)
{
	if (!_filename) _filename = m_filename;

	m_items.clear();
	m_fileText.clear();

	// Try to read preferences if they exist
	TextFileReader* reader = (TextFileReader*)new UnicodeTextFileReader( _filename ); 

	if (reader && !reader->IsUnicode())
	{
		delete reader;
		reader = new TextFileReader( _filename ); 
	}

	if( !reader || !reader->IsOpen() )
    {
        // Probably first time running the game
        CreateDefaultValues();
    }
    else
    {
		bool firstLine = true;
		bool fuckedUpPrefs = false;

		while( reader->ReadLine() )
	    {
			UnicodeString line = reader->GetRestOfUnicodeLine();

			// Unicode file fuck up check 
			if( firstLine )
			{
				int length = line.WcsLen();
				for( int i = 0; i < length; i++ )
				{
					if( line.m_unicodestring[i] == L' ' ||
						line.m_unicodestring[i] == L'=' ) 
						break;

					if( line.m_unicodestring[i] < L'A' ||
						line.m_unicodestring[i] > L'z' )
					{
						fuckedUpPrefs = true;
						break;
					}
				}
				firstLine = false;
			}

            if( !fuckedUpPrefs )
			{
				AddLine( line );
			}
        }

		// Unicode file fuck up fix
		if( fuckedUpPrefs )
		{
			CreateDefaultValues();
			Save();
		}
    }

#ifdef DEMOBUILD
	s_overwrite = true;
	AddLine( OTHER_DIFFICULTY " = 1" );
	s_overwrite = false;
#endif

    // Hack by Chris to disable old-style control
    SetInt( "MultiwiniaControlMethod", 1 );
}


void PrefsManager::SaveItem(UnicodeTextFileWriter& out, PrefsItem &_item)
{
	if( _item.m_hasBeenWritten ) return;

	switch (_item.m_type)
	{
		case PrefsItem::TypeFloat:
			out.printf(L"%ls = %.2f\n", _item.m_key.m_unicodestring, _item.m_float);
			break;
		case PrefsItem::TypeInt:
			out.printf(L"%ls = %d\n", _item.m_key.m_unicodestring, _item.m_int);
			break;
		case PrefsItem::TypeString:
			out.printf(L"%ls = %ls\n", _item.m_key.m_unicodestring, _item.m_str.m_unicodestring);
			break;
	}
	_item.m_hasBeenWritten = true;
}
			
void PrefsManager::Save()
{
	// We've got a copy of the plain text from the prefs file that we initially
	// loaded in the variable m_fileText. We use that file as a template with
	// which to create the new save file. Updated prefs items values are looked up
	// as it their turn to be output comes around. When we've finished we then
	// write out all the new prefs items because they didn't exist in m_fileText.

	// First clear the "has been written" flags on all the items
	
	for( PrefsItemMap::iterator i = m_items.begin(); i != m_items.end(); i++ )
	{
		i->second.m_hasBeenWritten = false;
	}

	// Now use m_fileText as a template to write most of the items
	UnicodeTextFileWriter out(m_filename, false, false);
	
	// If we couldn't open the prefs file for writing then just silently fail - 
	// it's better than crashing.
	if( !out )
	{
		return;
	}

	for( FileTextList::iterator i = m_fileText.begin(); i != m_fileText.end(); i++ )
	{
		UnicodeString &line = *i;
		
		if (IsLineEmpty(line))
		{
			out.printf(line);
		}
		else
		{
			wchar_t const *c = line.m_unicodestring;
			wchar_t const *keyStart = NULL;
			wchar_t const *keyEnd = NULL;
			while (*c != L'=') 
			{
				if (keyStart)
				{
					if (!iswalnum(c[0]))
					{
						keyEnd = c;
					}
				}
				else
				{
					if (iswalnum(c[0]))
					{
						keyStart = c; 
					}
				}
				++c;
			}
			if (keyStart && keyEnd)
			{
				wchar_t key[128];
				int keyLen = keyEnd - keyStart;
				if (keyLen >= sizeof(key)/sizeof(wchar_t))
				{
					keyLen = (sizeof(key)/sizeof(wchar_t)) - 1;
				}
				wcsncpy(key, keyStart, keyLen);
				key[keyLen] = '\0';

				SaveItem(out, m_items[ key ]);
			}
			else
			{
				out.printf(line);
			}
		}
	}

	// Finally output any items that haven't already been written
	for( PrefsItemMap::iterator i = m_items.begin(); i != m_items.end(); i++ )
	{
		PrefsItem &item = i->second;
		if( !item.m_hasBeenWritten )
		{
			SaveItem( out, item );
		}
	}
}


void PrefsManager::Clear()
{
	m_items.clear();
	m_fileText.clear();
}


float PrefsManager::GetFloat(char const *_key, float _default) const
{
	PrefsItemMap::const_iterator i = m_items.find( _key );
	if( i == m_items.end() )	
		return _default;

	PrefsItem const &item = i->second; 
	if (item.m_type != PrefsItem::TypeFloat) 
		return _default;

	return item.m_float;
}


int PrefsManager::GetInt(char const *_key, int _default) const
{
	PrefsItemMap::const_iterator i = m_items.find( _key );
	if( i == m_items.end() )	
		return _default;

	PrefsItem const &item = i->second; 
	if (item.m_type != PrefsItem::TypeInt) 
		return _default;

	return item.m_int;
}


char const* PrefsManager::GetString(char const *_key, char const *_default) const
{
	PrefsItemMap::const_iterator i = m_items.find( _key );
	if( i == m_items.end() )	
		return _default;

	PrefsItem const &item = i->second; 
	if (item.m_type != PrefsItem::TypeString) 
		return _default;

	return item.m_str.m_charstring;
}

UnicodeString PrefsManager::GetUnicodeString(char const *_key, UnicodeString const &_default) const
{
	PrefsItemMap::const_iterator i = m_items.find( _key );
	if( i == m_items.end() )	
		return _default;

	PrefsItem const &item = i->second; 
	if (item.m_type != PrefsItem::TypeString) 
		return _default;

	return item.m_str;
}

void PrefsManager::SetString(char const *_key, UnicodeString const &_string)
{
	PrefsItemMap::iterator i = m_items.find(_key);
	if( i != m_items.end() )
	{
		PrefsItem &item = i->second;
		item.m_type = PrefsItem::TypeString;
		item.m_str = _string;
	}
	else
	{
		m_items[ _key ] = PrefsItem(_key, _string);
	}
}


void PrefsManager::SetFloat(char const *_key, float _float)
{
	PrefsItemMap::iterator i = m_items.find(_key);
	if( i != m_items.end() )
	{
		PrefsItem &item = i->second;
		item.m_type = PrefsItem::TypeFloat;
		item.m_float = _float;
	}
	else
	{
		m_items[ _key ] = PrefsItem(_key, _float);
	}
}


void PrefsManager::SetInt(char const *_key, int _int)
{
	PrefsItemMap::iterator i = m_items.find(_key);
	if( i != m_items.end() )
	{
		PrefsItem &item = i->second;
		item.m_type = PrefsItem::TypeInt;
		item.m_int = _int;
	}
	else
	{
		m_items[ _key ] = PrefsItem(_key, _int);
	}
}


void PrefsManager::AddLine(UnicodeString _line)
{
	bool saveLine = true;

	wchar_t* c = NULL;

	if (!IsLineEmpty(_line))				// Skip comment lines and blank lines
	{
		c = _line.m_unicodestring;
		while( c != NULL )
		{
			c = wcschr(c, L'\r');
			if( c ) c[0] = '\0';
		}
		c = _line.m_unicodestring;
		while( c != NULL )
		{
			c = wcschr(c, L'\n');
			if( c ) c[0] = '\0';
		}
		/*while (c[1] != L'\0') c++;
		if (c[0] == L'\n')
		{
			c[0] = '\0';
			_line.CopyUnicodeToCharArray();
		}*/
		_line.CopyUnicodeToCharArray();

		PrefsItem item( _line );
		PrefsItemMap::iterator idx = m_items.find( item.m_key );
		
		if( idx != m_items.end() && !s_overwrite )
		{
			PrefsItem &existing = idx->second;

			if ( s_overwrite ) 
			{				
				m_items.erase( idx );
				saveLine = false;
			}
			else
			{
				AppDebugOut("Duplicate preference item %s. Ignoring\n", item.m_key.m_charstring );
				return;
			}
		}

		m_items[ item.m_key ] = item;
	}

	if ( saveLine ) 
	{
		m_fileText.push_back( _line );
	}
}

bool PrefsManager::DoesKeyExist(char const *_key) const
{
	PrefsItemMap::const_iterator i = m_items.find(_key);
	return i != m_items.end();
}
