#include "lib/universal_include.h"

#include <strstream>
#include <iostream>
#include <fstream>

#include "lib/debug_utils.h"
#include "lib/language_table.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/text_stream_readers.h"

#include "app.h"

#include "lib/input/input.h"

#define DEBUG_PRINT_LANGTABLE 0

// ****************************************************************************
// Class LangPhrase
// ****************************************************************************

LangPhrase::LangPhrase()
:	m_key(NULL), 
	m_string(NULL)
{
}


LangPhrase::~LangPhrase()
{
	free(m_key);
	free(m_string);
}


// ****************************************************************************
// Class LangTable
// ****************************************************************************

LangTable::LangTable(char *_filename)
{
	// Create the "not found" Phrase that will be returned when 
	// the LookupPhrase is called with an unknown key
	m_notFound.m_key = NULL;
	m_notFound.m_string = (char*)malloc(256);

	m_phrasesKbd = NULL;
	m_phrasesXin = NULL;
	m_chunk = NULL;

	// Open the required language file
    ParseLanguageFile( _filename );
}


LangTable::~LangTable()
{
    DArray<LangPhrase *> *phrasesRaw = m_phrasesRaw.ConvertToDArray();
    phrasesRaw->EmptyAndDelete();
	delete phrasesRaw;
	delete m_phrasesKbd;
	delete m_phrasesXin;
	/*
	if ( m_phrasesKbd ) {
		m_phrasesKbd->Empty();
		delete m_phrasesKbd;
	}
	if ( m_phrasesXin ) {
		m_phrasesXin->Empty();
		delete m_phrasesXin;
	}
	*/
	if ( m_chunk ) delete [] m_chunk;
}


void LangTable::ParseLanguageFile(char const *_filename)
{
    TextReader *in = g_app->m_resource->GetTextReader(_filename);
	DarwiniaReleaseAssert(in && in->IsOpen(), "Couldn't open language file %s", _filename );

	// Read all the phrases from the language file
	while (in->ReadLine())
	{
		if (!in->TokenAvailable()) continue;

		char *key = in->GetNextToken();	

		LangPhrase *phrase = new LangPhrase;
		phrase->m_key = strdup(key);
	
		// Make sure the this key isn't already used
		if(m_phrasesRaw.LookupTree(key))
        {
            m_phrasesRaw.RemoveData( key );
        }

		char *aString = in->GetRestOfLine(); 
         
		// Make sure a language key always has a some text with it
		DarwiniaDebugAssert(aString && aString[0] != '\0');	

        for( unsigned int i = 0; i < strlen(aString)-1; ++i )
        {
            if( aString[i] == '\\' && aString[i+1] == 'n' )
            {
                aString[i] = ' ';
                aString[i+1] = '\n';
            }
        }
        
        phrase->m_string = strdup( aString );

        int stringLength = strlen( phrase->m_string );
        if( phrase->m_string[stringLength-1] == '\n' ) 
        {
            phrase->m_string[stringLength-1] = '\x0';
        }

        if( phrase->m_string[stringLength-2] == '\r' )
        {
            phrase->m_string[stringLength-2] = '\x0';
        }

		m_phrasesRaw.PutData(phrase->m_key, phrase);
	}

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


bool buildCaption(char const *_baseString, char *_dest, InputMode _mood); // see sepulveda_strings.cpp


// Only used for debugging
bool printTable( HashTable<int> const *keys, const char *table, std::ostream &out )
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


void LangTable::RebuildTables()
{
	std::ostrstream stream;

	delete m_phrasesKbd;
	delete m_phrasesXin;

	m_phrasesKbd = new HashTable<int>();
	m_phrasesXin = new HashTable<int>();

	RebuildTable( m_phrasesKbd, stream, INPUT_MODE_KEYBOARD );
	RebuildTable( m_phrasesXin, stream, INPUT_MODE_GAMEPAD );
	if ( m_chunk ) delete [] m_chunk;
	m_chunk = stream.str();

	if ( DEBUG_PRINT_LANGTABLE ) {
		std::ofstream dbg_out( "langtable_debug.txt", std::ios::app );
		dbg_out << "********** KEYBOARD MODE STRINGS **********" << std::endl;
		if ( !printTable( m_phrasesKbd, m_chunk, dbg_out ) ) {
			dbg_out << "KEYBOARD STRINGS NOT AVAILABLE." << std::endl;
		}
		dbg_out << std::endl << "********** GAMEPAD MODE STRINGS **********" << std::endl;
		if ( !printTable( m_phrasesXin, m_chunk, dbg_out ) ) {
			dbg_out << "KEYBOARD STRINGS NOT AVAILABLE." << std::endl;
		}
		dbg_out << std::endl << "********** FINISHED ALL STRINGS **********" << std::endl
			    << std::endl << std::endl;
	}
}

void LangTable::RebuildTable( HashTable<int> *_phrases, std::ostrstream &stream, InputMode _mood )
{
	char theString[1024];

	DArray<LangPhrase*> *ary = m_phrasesRaw.ConvertToDArray();

	stream << '\x0'; // This is our empty string

	// DebugOut(" There are %d keys to add\n", ary->Size() );

	for ( int i = 0; i < ary->Size(); ++i ) {
		if ( ary->ValidIndex( i ) ) {
			LangPhrase const *phrase = ary->GetData( i );
			if ( strncmp( phrase->m_key, "part_", 5 ) != 0 ) {
				char key[1024];
				strcpy( key, phrase->m_key );

				if ( !wrong_suffix( phrase->m_key, _mood ) ) {
					// Make sure this is the most specific string, or ignore it
					if ( chomp_mode_suffix( key ) || !specific_key_exists( key, _mood ) ) {
						int currPos = stream.tellp();
						buildCaption( phrase->m_string, theString, _mood );
						stream << theString << '\x0';						
						
						// DebugOut("%d Adding key %d: %s -> %s\n", _mood, i, key, theString );
						_phrases->PutData( key, currPos );
					}
				} else {
					// Make sure this is the most specific string, or ignore it
					if ( chomp_mode_suffix( key ) && !RawDoesPhraseExist( key, _mood ) &&
					     !specific_key_exists( key, _mood ) ) {
						
						// DebugOut("%d Adding key %d: %s -> 0\n", _mood, i, key );
						_phrases->PutData( key, 0 ); // Give me an empty string
					}
				}
			}
		}
	}

	delete ary;
}


HashTable<int> *LangTable::GetCurrentTable()
{
	if ( g_inputManager )
		return GetCurrentTable( g_inputManager->getInputMode() );
	return NULL;
}


HashTable<int> *LangTable::GetCurrentTable( InputMode _mood )
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
	HashTable<int> *phrases = GetCurrentTable();
    if( !_key || !phrases )
    {
        return false;
    }
    else
    {
        int phrase = phrases->GetData( _key, -1 );
        return( phrase != -1 );
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


char *LangTable::LookupPhrase(char const *_key)
{
	HashTable<int> *phrases = GetCurrentTable();
    char *phrase = NULL;

    if( !_key || !phrases || !m_chunk )
    {
        sprintf( m_notFound.m_string, "ERROR (null)" );
		phrase = m_notFound.m_string;
    }
    else
    {
		int offset = phrases->GetData( _key );
		if ( offset >= 0 )
		    phrase = m_chunk + offset;

	    if ( !phrase )
	    {
            //sprintf( m_notFound.m_string, "ERROR (%s)", _key );
			*(m_notFound.m_string) = '\0';
			phrase = m_notFound.m_string;
	    }
    }
    
	return phrase;
}


char *LangTable::RawLookupPhrase(char const *_key)
{
	if ( !g_inputManager )
    {
        sprintf( m_notFound.m_string, "ERROR (null)" );
		return m_notFound.m_string;
	}
	else
	{
		return RawLookupPhrase( _key, g_inputManager->getInputMode() );
	}

}


char *LangTable::RawLookupPhrase(char const *_key, InputMode _mood)
{
	LangPhrase *phrase = NULL;

    if( !_key )
    {
        sprintf( m_notFound.m_string, "ERROR (null)" );
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
			*(m_notFound.m_string) = '\0';
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
                char const *translatedPhrase = LookupPhrase( phrase->m_key );
                if( strcmp( phrase->m_string, translatedPhrase ) == 0 )
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
