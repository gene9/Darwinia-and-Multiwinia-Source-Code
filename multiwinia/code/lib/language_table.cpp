#include "lib/universal_include.h"

#include <strstream>
#include <iostream>
#include <fstream>

#include "lib/debug_utils.h"
#include "lib/language_table.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/unicode/unicode_text_stream_reader.h"
#include "lib/unicode/unicode_string.h"

#include "app.h"

#include "lib/input/input.h"

#define DEBUG_PRINT_LANGTABLE 0

#ifdef TARGET_MSVC
	#define snprintf _snprintf
#endif

#ifndef SEPULVEDA_MAX_PHRASE_LENGTH
	#define SEPULVEDA_MAX_PHRASE_LENGTH	1024
#endif
#define MAX_READ_KEY_LENGTH 64
#define MAX_FINAL_KEY_LENGTH 96

// Return 0, or the length of the marker at the beginning of _in,
// including the brackets.
int markerLength( wchar_t const *_in ) {
	int len = 0;
	if ( _in[0] == L'[' ) {
		while ( _in[++len] != L']' ) {
			if ( _in[len] == L'\0' ) {
				len = -1;
				break;
			}
		}
		++len;
	}
	return len;
}

// ****************************************************************************
// Class LangPhrase
// ****************************************************************************

LangPhrase::LangPhrase()
:	m_key(NULL),
	m_string()
{
}


LangPhrase::~LangPhrase()
{
	free(m_key);
	//free(m_string);
}


// ****************************************************************************
// Class LangTable
// ****************************************************************************

LangTable::LangTable(char *_filename)
{
	// Create the "not found" Phrase that will be returned when 
	// the LookupPhrase is called with an unknown key
	m_notFound.m_key = NULL;
	m_notFound.m_string = UnicodeString();

	m_phrasesKbd = NULL;
	m_phrasesXin = NULL;

	// Open the required language file
    ParseLanguageFile( _filename );
}


LangTable::~LangTable()
{
    DArray<LangPhrase *> *phrasesRaw = m_phrasesRaw.ConvertToDArray();
    phrasesRaw->EmptyAndDelete();
	delete phrasesRaw;

	if( m_phrasesKbd )
		m_phrasesKbd->EmptyAndDelete();
	delete m_phrasesKbd;

	if( m_phrasesXin )
		m_phrasesXin->EmptyAndDelete();
	delete m_phrasesXin;
}


void LangTable::ParseLanguageFile(char const *_filename)
{
    TextReader *in = g_app->m_resource->GetTextReader(_filename);
	if( !in || !in->IsOpen() )
	{
		AppDebugOut("WARNING: Can not find language file %s to parse. Soft failing\n", _filename);
		return;
	}

	AppReleaseAssert(in && in->IsOpen(), "Couldn't open language file %s", _filename );

	UnicodeTextFileReader* uniIn = NULL;
	m_loadedUnicodeFile = false;
	if( in ) m_loadedUnicodeFile = in->IsUnicode();
	else AppDebugOut("Did not have TextReader. No checking it'a unicodeness\n");

	if (m_loadedUnicodeFile)
	{
		uniIn = (UnicodeTextFileReader*)in;
	}

	unsigned int i = 0;

	double startTime = GetHighResTime();
	int counter = 0;
	// Read all the phrases from the language file
	while (in->ReadLine())
	{
		counter++;
		if (!in->TokenAvailable()) continue;

		LangPhrase *phrase = new LangPhrase;

		char *key = in->GetNextToken();	
		phrase->m_key = strdup(key);
	
		// Make sure the this key isn't already used
		if(m_phrasesRaw.LookupTree(key))
        {
			LangPhrase *oldPhrase = m_phrasesRaw.GetData( key );
            m_phrasesRaw.RemoveData( key );
			delete oldPhrase;
        }

		UnicodeString aString;
		
		if (m_loadedUnicodeFile)
		{
			aString = uniIn->GetRestOfUnicodeLine();
		}
		else
		{
			aString = in->GetRestOfLine();
		}
         
		unsigned int aStringLength = aString.WcsLen();
		wchar_t* slashPos = aString.m_unicodestring;
		// Find a \ in the string
		while( (slashPos = wcsstr(slashPos, L"\\")) != NULL )
        {
			// As long as we're not one character away from the end
			if( slashPos + 1 < aString.m_unicodestring + aStringLength )
			{
				// if the character after the slash is an 'n'
				if( slashPos[1] == L'n' )
				{
					// Replace the slash with a space and the n with a \n
					slashPos[0] = L' ';
					slashPos[1] = L'\n';
				}
				slashPos++;
			}
			else
			{
				// Otherwise, we're too close to the end of the string, so end
				break;
			}
        }
        
        phrase->m_string = aString;
		phrase->m_string.CopyUnicodeToCharArray();

		int stringLength = phrase->m_string.Length();
        if( phrase->m_string.m_unicodestring[stringLength-1] == L'\n' ) 
        {
            phrase->m_string.m_unicodestring[stringLength-1] = L'\x0';
        }

        if( phrase->m_string.m_unicodestring[stringLength-2] == L'\r' ||
			phrase->m_string.m_unicodestring[stringLength-2] == L'\f')
        {
            phrase->m_string.m_unicodestring[stringLength-2] = L'\x0';
        }

		m_phrasesRaw.PutData(phrase->m_key, phrase);
		i++;
	}

	AppDebugOut("Read %d strings: %f seconds\n", i, GetHighResTime() - startTime );

	delete in;
}


bool is_suffix( const char * str, const char * suf )
{
	int suf_len = strlen( suf );
	int off = strlen( str ) - suf_len;
	if ( off >= 0 ) {
		return strncmp( str + off, suf, suf_len ) == 0;
	} else return false;
}


bool wrong_suffix( const char * key, InputMode _mood )
{
	if ( key ) {
		switch ( _mood ) {
			case INPUT_MODE_KEYBOARD:
				return is_suffix( key, "_xin" ); break;
			case INPUT_MODE_GAMEPAD:
				return is_suffix( key, "_kbd" ); break;
		}
	}
	return true; // It's not the suffix, but something's wrong.
}


bool chomp_mode_suffix( char * key )
{
	if ( is_suffix( key, "_kbd" ) || is_suffix( key, "_xin" ) ) {
		key[strlen(key) - 4] = '\0';
		return true;
	}
	return false;
}


bool LangTable::specific_key_exists( const char * _key, InputMode _mood )
{
	if ( _key && _mood ) {
		char key[128];
		int len = strlen(_key);
		strncpy( key, _key, 123 ); key[123] = '\0';

		switch ( _mood ) {
			case INPUT_MODE_KEYBOARD:
				strcpy( key + len, "_kbd" ); break;
			case INPUT_MODE_GAMEPAD:
				strcpy( key + len, "_xin" ); break;
			default:
				return false;
		}

		return RawDoesPhraseExist( key, _mood );
	} else return false;
}


bool buildCaption(UnicodeString const &_baseString, UnicodeString &_dest, InputMode _mood); // see sepulveda_strings.cpp

// Only used for debugging
bool printTable( HashTable<int> const *keys, const UnicodeString *table, std::ostream &out )
{
	if ( keys && table && out.good() ) {
		for ( int i = 0; i < keys->Size(); ++i ) {
			if ( keys->ValidIndex( i ) ) {
				out << keys->GetName( i ) << '\t'
				    << ( table + keys->GetData( i ) ) << std::endl;
			}
		}
		return true;
	}
	return false;
}

bool LangTable::ReadUnicode()
{
	return m_loadedUnicodeFile;
}

void LangTable::RebuildTables()
{
	if( m_phrasesKbd )
		m_phrasesKbd->EmptyAndDelete();
	delete m_phrasesKbd;

	if( m_phrasesXin )
		m_phrasesXin->EmptyAndDelete();
	delete m_phrasesXin;

	m_phrasesKbd = new HashTable<UnicodeString*>();
	m_phrasesXin = new HashTable<UnicodeString*>();

	RebuildTable( m_phrasesKbd, INPUT_MODE_KEYBOARD );
	RebuildTable( m_phrasesXin, INPUT_MODE_GAMEPAD );
}

void LangTable::RebuildTable( HashTable<UnicodeString*> *_phrases, InputMode _mood )
{
	DArray<LangPhrase*> *ary = m_phrasesRaw.ConvertToDArray();

	for ( int i = 0; i < ary->Size(); ++i ) {
		if ( ary->ValidIndex( i ) ) {
			LangPhrase const *phrase = ary->GetData( i );
			if ( strncmp( phrase->m_key, "part_", 5 ) != 0 ) {
				char key[1024];
				strcpy( key, phrase->m_key );

				if ( !wrong_suffix( phrase->m_key, _mood ) ) {
					// Make sure this is the most specific string, or ignore it
					if ( chomp_mode_suffix( key ) || !specific_key_exists( key, _mood ) ) {
						// AppDebugOut("%d Adding key %d: %s -> %s\n", _mood, i, key, theString );
						UnicodeString data;
						UnicodeString base (phrase->m_string.m_unicodestring);
						buildCaption( base, data, _mood );
						UnicodeString* tempData = new UnicodeString(data.m_unicodestring);
						_phrases->PutData( key, tempData);
					}
				} else {
					// Make sure this is the most specific string, or ignore it
					if ( chomp_mode_suffix( key ) && !RawDoesPhraseExist( key, _mood ) &&
					     !specific_key_exists( key, _mood ) ) {
						
						// AppDebugOut("%d Adding key %d: %s -> 0\n", _mood, i, key );
						_phrases->PutData( key, new UnicodeString() ); // Give me an empty string
					}
				}
			}
		}
	}

	delete ary;
}


HashTable<UnicodeString*> *LangTable::GetCurrentTable()
{
	if ( g_inputManager )
		return GetCurrentTable( g_inputManager->getInputMode() );
	return NULL;
}


HashTable<UnicodeString*> *LangTable::GetCurrentTable( InputMode _mood )
{
	if ( !m_phrasesKbd || !m_phrasesXin ) 
		RebuildTables();

	switch ( _mood ) {
		case INPUT_MODE_KEYBOARD: 
			return m_phrasesKbd;

		case INPUT_MODE_GAMEPAD:  
			return m_phrasesXin;
	}
	return NULL;
}


bool LangTable::DoesPhraseExist(char const *_key)
{
	HashTable<UnicodeString*> *phrases = GetCurrentTable();
    if( !_key || !phrases )
    {
        return false;
    }
    else
    {
        UnicodeString* phrase = phrases->GetData( _key, NULL );
        return( phrase != NULL );
    }
}


bool LangTable::RawDoesPhraseExist(char const *_key, InputMode _mood)
{
	bool ans = false;
	if ( _key ) {
		if ( RawDoesPhraseExist( _key ) ) {
			ans = true;
		} else {
			int len = strlen( _key );
			char *key = new char[ len + 5 ];
			strcpy( key, _key );
			switch( _mood ) {
				case INPUT_MODE_KEYBOARD: strcpy( key + len, "_kbd" ); break;
				case INPUT_MODE_GAMEPAD: strcpy( key + len, "_xin" ); break;
			}
			ans = RawDoesPhraseExist( key );
			delete [] key;
		}
	}
	return ans;
}

bool LangTable::buildCaption( UnicodeString const &_baseString, UnicodeString& _dest, CaptionParserMode &_mode ) {
	wchar_t wch;
	UnicodeString temp;
	temp.Resize( SEPULVEDA_MAX_PHRASE_LENGTH );

	while ( ( wch = _baseString.m_unicodestring[ _mode.inOffset ] ) != L'\0' ) {
		if ( wch == '[' ) {
			if ( !consumeMarker( _baseString, temp, _mode ) ) {
				_dest = temp;
				return false;
			}
		} else if ( _mode.writing ) {
			if ( _mode.outOffset < SEPULVEDA_MAX_PHRASE_LENGTH - 1 )
				temp.m_unicodestring[ _mode.outOffset++ ] = wch;
			else {
				temp.m_unicodestring[ _mode.outOffset ] = L'\0';
				_dest = temp;
				return false;
			}
		}
		++_mode.inOffset;
	}
	temp.m_unicodestring[ _mode.outOffset ] = '\0';
	_dest = temp;
	return true;
}

bool LangTable::buildPhrase( UnicodeString const &_key, UnicodeString &_dest, CaptionParserMode &_mode ) {

	bool done = false;
	UnicodeString temp;
	temp.Resize( SEPULVEDA_MAX_PHRASE_LENGTH );
	if ( _mode.writing ) {
		UnicodeString key;
		key.Resize(MAX_FINAL_KEY_LENGTH);
		wcsncpy( key.m_unicodestring, _key.m_unicodestring, MAX_FINAL_KEY_LENGTH - 1 );
		key.m_unicodestring[MAX_FINAL_KEY_LENGTH - 1] = L'\0';
		key.StrLwr();
		key.CopyUnicodeToCharArray();
		if ( RawDoesPhraseExist( (const char*)key, _mode.mood ) ) {
			UnicodeString definition = RawLookupPhrase( (const char*)key, _mode.mood );
			if ( &definition ) {
				CaptionParserMode mode;
				mode.outOffset = _mode.outOffset;
				done = buildCaption( definition, temp, mode );
				if ( done )
					_mode.outOffset = mode.outOffset;
			}
		}
	}

	_dest = temp;
	return done;
}

bool LangTable::consumeMarker( UnicodeString const &_baseString, UnicodeString &_dest, CaptionParserMode &_mode ) 
{
	UnicodeString temp;
	temp.Resize(_dest.Length()+1);
	if ( consumeIfMarker( _baseString, temp, _mode )
		|| consumeKeyMarker( _baseString, temp, _mode )
		|| consumeOtherMarker( _baseString, temp, _mode ) ) // Other must be last. It always succeeds.
	{
		_dest = temp;
		return true;
	}
	wcsncpy( temp.m_unicodestring, _baseString.m_unicodestring, SEPULVEDA_MAX_PHRASE_LENGTH - 1 );
	temp.m_unicodestring[SEPULVEDA_MAX_PHRASE_LENGTH - 1] = L'\0';
	_dest = temp;
	return false;
}


bool LangTable::consumeIfMarker( UnicodeString const &_baseString, UnicodeString &_dest, CaptionParserMode &_mode ) {
	UnicodeString temp;
	temp.Resize(_dest.Length()+1);
	bool done = false;
	UnicodeString in(_baseString.m_unicodestring + _mode.inOffset);
	if (  in.StrOffsetCmp( "[IFMODE ", 0, false ) == 0 ) {
		++_mode.ifdepth;
		if ( _mode.writing ) {
			if ( in.StrOffsetCmp( L"KEYBOARD]", 8, false ) == 0 ) {
				if ( _mode.mood != INPUT_MODE_KEYBOARD ) {
					_mode.writing = false;
					_mode.writingStopDepth = _mode.ifdepth;
				}
				_mode.inOffset += 16;
				done = true;
			}
		} else {
			_mode.inOffset += markerLength( in );
			done = true;
		}
	}
	else if ( in.StrOffsetCmp( L"[ELSE]", 0, false ) == 0 ) {
		if ( _mode.writing ) {
			_mode.writing = false;
			_mode.writingStopDepth = _mode.ifdepth;
		} else if ( _mode.ifdepth == _mode.writingStopDepth )
			_mode.writing = true;
		_mode.inOffset += 5;
		done = true;
	}
	else if ( in.StrOffsetCmp( L"[ENDIF]", 0, false ) == 0 ) {
		if ( !_mode.writing && _mode.ifdepth == _mode.writingStopDepth )
			_mode.writing = true;
		--_mode.ifdepth;
		_mode.inOffset += 6;
		done = true;
	}

	_dest = temp;
	return done;
}


bool LangTable::consumeKeyMarker( UnicodeString const &_baseString, UnicodeString &_dest, CaptionParserMode &_mode ) {
	UnicodeString temp;
	temp.Resize(_dest.Length()+1);
	UnicodeString in(_baseString.m_unicodestring + _mode.inOffset);
	bool done = false;
	if ( in.StrOffsetCmp(L"[KEY", 0, false ) == 0 ) {
		int len = markerLength( in ) - 5;
		if ( !_mode.writing && 0 < len && len < MAX_READ_KEY_LENGTH )
			done = true;
		else if ( _mode.writing && 0 < len && len < MAX_READ_KEY_LENGTH ) {
			wchar_t buf[MAX_READ_KEY_LENGTH];
			wcsncpy( buf, in.m_unicodestring + 4, len );
			buf[len] = L'\0';

			UnicodeString tempString(len+1,buf,len+1);

			controltype_t eventId = g_inputManager->getControlID( (const char*)tempString );
			if ( eventId >= 0 ) {
				// Lookup the key name for the binding
				InputDescription desc;
				if ( g_inputManager->getBoundInputDescription( static_cast<ControlType>(eventId), desc ) )
				{
					UnicodeString const keyName(desc.noun.c_str());

					// Is the caption long enough
					if( _mode.outOffset + keyName.Length() < SEPULVEDA_MAX_PHRASE_LENGTH - 1 ) {
						// Write the key name into the caption
						wcscpy(temp.m_unicodestring + _mode.outOffset, keyName.m_unicodestring);
						_mode.outOffset += keyName.Length();
						done = true;
					}
				}
			}
		}
		if ( done )
			_mode.inOffset += len + 4;
	}

	_dest = temp;
	return done;
}


bool LangTable::consumeOtherMarker( UnicodeString const &_baseString, UnicodeString &_dest, CaptionParserMode &_mode ) {
	UnicodeString temp;
	temp.Resize(_dest.Length()+1);

	// Look in hash table
	const wchar_t *in = _baseString.m_unicodestring + _mode.inOffset;
	bool done = false;

	int len = markerLength( in ) - 2;
	if ( !_mode.writing && 0 < len && len < MAX_READ_KEY_LENGTH )
		done = true;
	else if ( 0 < len && len < MAX_READ_KEY_LENGTH ) {
		UnicodeString key;
		key.Resize(MAX_READ_KEY_LENGTH);
		CaptionParserMode mode( _mode );

		if ( in[1] == ' ' ) {
			if ( mode.outOffset < SEPULVEDA_MAX_PHRASE_LENGTH - 1 )
				temp.m_unicodestring[ mode.outOffset++ ] = L' ';
			wcsncpy(key.m_unicodestring, in + 2, len - 1);
			key.m_unicodestring[len - 1] = L'\0';
		} else {
			wcsncpy(key.m_unicodestring, in + 1, len);
			key.m_unicodestring[len] = L'\0';
		}

		//done = buildModalPhrase( key, _dest, mode );
		done = buildPhrase( key, temp, mode );
		if ( done )
			_mode = mode;
	}

	_mode.inOffset += len + 1; // Always eat the input

	_dest = temp;
	return true; // Always succeed
}

bool LangTable::buildCaption(UnicodeString const &_baseString, UnicodeString& _dest) {
	CaptionParserMode mode;
	return buildCaption( _baseString, _dest, mode );
}


bool LangTable::buildCaption(UnicodeString const &_baseString, UnicodeString& _dest, InputMode _mood) {
	CaptionParserMode mode;
	mode.mood = _mood;
	return buildCaption( _baseString, _dest, mode );
}


bool LangTable::RawDoesPhraseExist(char const *_key)
{
    if( !_key )
    {
        return false;
    }
    else
    {
        LangPhrase const *phrase = m_phrasesRaw.GetData( _key );
        return( phrase != NULL );
    }
}


const UnicodeString &LangTable::LookupPhrase(char const *_key)
{
	HashTable<UnicodeString*> *phrases = GetCurrentTable();

    if( !_key || !phrases )
    {
		m_notFound.m_string = UnicodeString("ERROR (null)");
		return m_notFound.m_string;
    }
    else
    {
		UnicodeString* usp = phrases->GetData( _key );
		
	    if ( !usp )
	    {
			/*char temp[256];
			sprintf( temp, "ERROR (%s)", _key );*/
			m_notFound.m_string = UnicodeString(_key); // was temp
			return m_notFound.m_string;
	    }
		else
		{
			return *usp;
		}
    }
}


const UnicodeString &LangTable::RawLookupPhrase(char const *_key)
{
	if ( !g_inputManager )
    {
        //sprintf( m_notFound.m_string, "ERROR (null)" );
		char* text = "ERROR (null)";
		m_notFound.m_string = UnicodeString(text);
		return m_notFound.m_string;
	}
	else
	{
		return RawLookupPhrase( _key, g_inputManager->getInputMode() );
	}

}


const UnicodeString &LangTable::RawLookupPhrase(char const *_key, InputMode _mood)
{
	LangPhrase *phrase = NULL;

    if( !_key )
    {
        //sprintf( m_notFound.m_string, "ERROR (null)" );
		char* text = "ERROR (null)";
		m_notFound.m_string = UnicodeString(text);
		phrase = &m_notFound;
    }
    else
    {
		if ( !phrase ) {
			int len = strlen( _key );
			char *key = new char[ len + 5 ];
			strcpy( key, _key );
			switch( _mood ) {
				case INPUT_MODE_KEYBOARD: strcpy( key + len, "_kbd" ); break;
				case INPUT_MODE_GAMEPAD: strcpy( key + len, "_xin" ); break;
			}
			phrase = m_phrasesRaw.GetData( key );
			delete [] key;
		}

		if ( !phrase )
		{
			phrase = m_phrasesRaw.GetData( _key );
		}

	    if ( !phrase )
	    {
            //sprintf( m_notFound.m_string, "ERROR (%s)", _key );
			char text[256];
			sprintf(text, "ERROR (%s)", _key);
			m_notFound.m_string = UnicodeString(text);
			phrase = &m_notFound;
	    }
    }

	return phrase->m_string;
}


void LangTable::TestAgainstEnglish()
{
    //
    // Open the errors file

    FILE *output = fopen( "language_errors.txt", "wt" );


    //
    // Open the English language file as a base
    
    LangTable *english = new LangTable( "language/english.txt" );
    
    
    //
    // Look for strings in English that are not in this language
    
    DArray<LangPhrase *> *englishPhrases = english->m_phrasesRaw.ConvertToDArray();

    for( int i = 0; i < englishPhrases->Size(); ++i )
    {
        if( englishPhrases->ValidIndex(i) )
        {
            LangPhrase *phrase = englishPhrases->GetData(i);

            if( !DoesPhraseExist( phrase->m_key ) )
            {
                fprintf( output, "ERROR : Failed to find translation for string ID '%s'\n", phrase->m_key );
            }
            else
            {
                UnicodeString const translatedPhrase = LookupPhrase( phrase->m_key );
				if( wcscmp( phrase->m_string.m_unicodestring, translatedPhrase.m_unicodestring ) == 0 )
                {
                    fprintf( output, "ERROR : String ID appears not to be translated : '%s'\n", phrase->m_key );
                }
            }
        }
    }

    delete englishPhrases;


    //
    // Look for phrases in this language that are not in English

    DArray<LangPhrase *> *langPhrases = m_phrasesRaw.ConvertToDArray();

    for( int i = 0; i < langPhrases->Size(); ++i )
    {
        if( langPhrases->ValidIndex(i) )
        {
            LangPhrase *phrase = langPhrases->GetData(i);

            if( !english->DoesPhraseExist( phrase->m_key ) )
            {
                fprintf( output, "ERROR : Found new string ID not present in original English : '%s'\n", phrase->m_key );               
            }
        }
    }

	delete langPhrases;


    //
    // Clean up

    delete english;
    fclose( output );
}


DArray<LangPhrase *> *LangTable::GetPhraseList()
{
    DArray<LangPhrase *> *englishPhrases = m_phrasesRaw.ConvertToDArray();
    return englishPhrases;
}

// UnicodeString version
LList <UnicodeString*> *WordWrapText (const UnicodeString &_string, float _lineWidth, float _fontWidth, bool _wrapToWindow, bool _forceWrap) 
{
	if ( !&_string ) return NULL;
    if ( _lineWidth < 0 && _wrapToWindow ) return NULL;

	// Calculate the maximum width in characters for 1 line
    int linewidth = int(_lineWidth / _fontWidth);
	
	// Build a linked list of pointers into this new string
	// Each pointer representing another line
	LList <UnicodeString*> *llist = new LList <UnicodeString*> ();

	// An object which is the remainder of the line to process.
	UnicodeString remaining(_string);

	while ( true ) 
    {		
		int nextnewline = remaining.FirstPositionOf( L'\n' );
		if ( nextnewline == 0 ) 
		{
			if (wcslen(remaining.m_unicodestring) > linewidth && _wrapToWindow )
			{
				nextnewline = wcslen(remaining.m_unicodestring);
			}
			else
			{
				llist->PutData(new UnicodeString(remaining));
				break;
			}
		}

		if ( _wrapToWindow && nextnewline > linewidth ) 
        {
			// This line is too long and needs trimming down
			// Ignore the newline char we found - it will be dealt with
			// as part of the next line
			// Place a terminater in the space between the last 2 words

			UnicodeString temp = UnicodeString(linewidth+1, 
				remaining.m_unicodestring,  
				linewidth);

			int spacePos = temp.LastPositionOf(L' ');			
			if( spacePos <= 0 && remaining.Length() >= linewidth && remaining.m_unicodestring[linewidth] == L' ')
			{
				spacePos = linewidth;
			}
			if( spacePos <= 0 )
			{
				spacePos = temp.LastHanCharacter();
			}

			//remaining.m_unicodestring[nextnewline] = L' ';
			
			if ( spacePos > 0) 
            {
				UnicodeString* ustring = new UnicodeString(spacePos+1, temp.m_unicodestring, spacePos);
				llist->PutData(ustring);

				remaining = UnicodeString( remaining.Length()-spacePos, &(remaining.m_unicodestring[spacePos+1]), remaining.Length()-spacePos );
			}
			else 
            {
                if( _forceWrap )
                {
                    // We need to split this line no matter what. Use line width
                    UnicodeString* ustring = new UnicodeString(UnicodeString(linewidth, temp.m_unicodestring, linewidth-1) + UnicodeString("-"));
                    llist->PutData(ustring);

                    remaining = UnicodeString( remaining.Length()-linewidth+2, &(remaining.m_unicodestring[linewidth-1]), remaining.Length()-linewidth+1 );
                }
                else
                {
				    // We cannot wrap this line - the word is longer than the max line width   
				    llist->PutData(new UnicodeString(remaining));
				    break;
                }
			}

		}
		else 
        {
			// Found a newline char - replace with a terminator
			// then add this position in as the next line	
			// then continue from this position
			UnicodeString* ustring = new UnicodeString( nextnewline+1, remaining.m_unicodestring, nextnewline );
			llist->PutData( ustring );

            int remainingStart = remaining.Length()-nextnewline;
            int remainingLen = remaining.Length()-nextnewline-1;
            wchar_t *remainingData = &(remaining.m_unicodestring[nextnewline+1]);
			
            remaining = UnicodeString( remainingStart, remainingData, remainingLen ); 
		}
	}

    //
    // The very last line is always empty.
    // Remove it.
    //llist->RemoveData( llist->Size() - 1 );

	return llist;
}


// Goes through the string and divides it up into several 
// smaller strings, taking into account newline characters,
// and the width of the text area.
LList <char *> *WordWrapText (const char *_string, float _lineWidth, float _fontWidth, bool _wrapToWindow) 
{
	if ( !_string ) return NULL;
    if ( _lineWidth < 0 && _wrapToWindow ) return NULL;

	// Calculate the maximum width in characters for 1 line
    int linewidth = int(_lineWidth / _fontWidth);
	

	// Create a copy of the string which we will use as our output strings
	// (All connected together but seperated by 0)
	// And add a newline on at the end (to make sure a newline will be found)

	char *newstring = new char [ strlen(_string) + 2 ];
	sprintf( newstring, "%s\n", _string );

	// Build a linked list of pointers into this new string
	// Each pointer representing another line

	LList <char *> *llist = new LList <char *> ();

	char *currentpos = newstring;
	llist->PutData( currentpos );

	while ( true ) 
    {
		char *nextnewline = strchr( currentpos, '\n' );
		if ( !nextnewline ) break;

		if ( _wrapToWindow && nextnewline - currentpos > linewidth ) 
        {

			// This line is too long and needs trimming down
			// Ignore the newline char we found - it will be dealt with
			// as part of the next line
			// Place a terminater in the space between the last 2 words
			currentpos += linewidth;
			char oldchar = *(currentpos - 1);
			*(currentpos - 1) = 0;
			char *space = strrchr( currentpos - linewidth, ' ' );
			
			if ( space ) 
            {
				*(currentpos - 1) = oldchar;
				currentpos = space + 1;
				*space = 0;
				llist->PutData( currentpos );
			}
			else 
            {
				// We cannot wrap this line - the word is longer than the max line width                
			}
		}
		else 
        {
			// Found a newline char - replace with a terminator
			// then add this position in as the next line	
			// then continue from this position
			currentpos =  nextnewline + 1;
			*nextnewline = 0;
    		llist->PutData( currentpos );
		}
	}

    //
    // The very last line is always empty.
    // Remove it.
    llist->RemoveData( llist->Size() - 1 );

	return llist;
}


void LangTable::ReplaceStringFlag( char flag, char *string, char *subject )
{
    char final[1024];
    int bufferPosition = 0;

    for( int i = 0; i <= strlen( subject ); ++i )
    {
        if( subject[i] == '*' &&
            i+1 < strlen( subject ))
        {
            if( subject[i+1] == flag )
            {
                for( int j = 0; j < strlen(string); ++j )
                {                        
                    final[bufferPosition] = string[j];
                    bufferPosition++;
                }
                i++;
            }
            else
            {
                final[bufferPosition] = subject[i];
                bufferPosition++;
            }

        }
        else
        {
            final[bufferPosition] = subject[i];
            bufferPosition++;
        }
    }
    strcpy( subject, final );
}
