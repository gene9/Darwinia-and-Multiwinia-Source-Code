#include "lib/universal_include.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/file_system.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/debug_utils.h"
#include "lib/language_table.h"
#include "lib/preferences.h"


LanguageTable *g_languageTable = NULL;


// ****************************************************************************
// Class LanguageTable
// ****************************************************************************

LanguageTable::LanguageTable()
 : m_lang(NULL),
   m_defaultLanguage(NULL),
   m_onlyDefaultLanguageSelectable(false)
{
	SetDefaultLanguage( "english" );
}


LanguageTable::~LanguageTable()
{
    ClearTranslation();
    ClearBaseLanguage();

#ifdef TRACK_LANGUAGEPHRASE_ERRORS
	ClearLanguagePhraseErrors();
#endif

    m_languages.EmptyAndDelete();

	if( m_lang ) delete m_lang;
	if( m_defaultLanguage ) delete [] m_defaultLanguage;
}


void LanguageTable::Initialise()
{
    //
    // Load languages

    LoadLanguages();
}


void LanguageTable::LoadLanguages()
{
    //
    // Clear out all known languages

    m_languages.EmptyAndDelete();


    //
    // Explore the data/language directory, looking for languages

    LList<char *> *files = g_fileSystem->ListArchive( "data/language/", "*.txt", false );

    for( int i = 0; i < files->Size(); ++i )
    {
        char *thisFile = files->GetData(i);

        Language *lang = new Language();

		snprintf( lang->m_name, sizeof(lang->m_name), thisFile );
		lang->m_name[ sizeof(lang->m_name) - 1 ] = '\0';
		for( char *curName = lang->m_name; *curName; curName++ )
		{
			if( *curName == '.' )
			{
				*curName = '\0';
				break;
			}
		}
		strcpy( lang->m_caption, lang->m_name );

        snprintf( lang->m_path, sizeof(lang->m_path), "data/language/%s", thisFile );
		lang->m_path[ sizeof(lang->m_path) - 1 ] = '\0';

        LoadLanguageCaption( lang );

		if( m_onlyDefaultLanguageSelectable && stricmp( m_defaultLanguage, lang->m_name ) != 0 )
		{
			lang->m_selectable = false;
		}
		else
		{
			lang->m_selectable = true;
		}

        m_languages.PutData( lang );

        AppDebugOut( "Found language '%s' with caption '%s' in '%s'\n", 
                     lang->m_name, 
                     lang->m_caption, 
                     lang->m_path );
    }

    files->EmptyAndDelete();
    delete files;
}


bool LanguageTable::LoadLanguageCaption( Language *lang )
{
    TextReader *in = g_fileSystem->GetTextReader( lang->m_path );

	if( !in ) return false;

	if( !in->IsOpen() )
	{
		delete in;
		return false;
	}

    //
	// Read all the phrases from the language file

	while (in->ReadLine())
	{
		if (!in->TokenAvailable()) continue;

		char *key = in->GetNextToken();

		char *aString = in->GetRestOfLine();

		if( key && aString && stricmp( key, "lang" ) == 0 )
		{
			//
			// Strip trailing '\n'
	        
			int stringLength = strlen( aString );
			if( aString[stringLength-1] == '\n' ) 
			{
				aString[stringLength-1] = '\x0';
			}

			if( aString[stringLength-2] == '\r' )
			{
				aString[stringLength-2] = '\x0';
			}

			snprintf( lang->m_caption, sizeof(lang->m_caption), aString );
			lang->m_caption[ sizeof(lang->m_caption) - 1 ] = '\0';

			delete in;
			return true;
		}
	}

    delete in;
	return false;
}


void LanguageTable::SetDefaultLanguage( char *_name, bool _onlySelectable )
{
	if( !_name ) return;

	if( m_defaultLanguage ) delete [] m_defaultLanguage;

	m_defaultLanguage = new char[ strlen( _name ) + 1 ];
	strcpy( m_defaultLanguage, _name );

	m_onlyDefaultLanguageSelectable = _onlySelectable;
	if( m_onlyDefaultLanguageSelectable )
	{
		int lenLanguages = m_languages.Size();
		for( int i = 0; i < lenLanguages; ++i )
		{
			Language *thisLang = m_languages.GetData( i );
			if( stricmp( thisLang->m_name, m_defaultLanguage ) != 0 )
			{
				thisLang->m_selectable = false;
			}
			else
			{
				thisLang->m_selectable = true;
			}
		}
	}
}


void LanguageTable::LoadCurrentLanguage()
{
	if( !m_lang )
	{
		char *interfaceLanguage = g_preferences->GetString( PREFS_INTERFACE_LANGUAGE, "english" );

		AppReleaseAssert( interfaceLanguage, "Couldn't find language preference '%s'", PREFS_INTERFACE_LANGUAGE );

		if( !GetLanguageSelectable( interfaceLanguage ) )
		{
			interfaceLanguage = m_defaultLanguage;
		}

		for( int i = 0; i < m_languages.Size(); i++ )
		{
			Language *thisLang = m_languages.GetData( i );
			if( stricmp( thisLang->m_name, interfaceLanguage ) == 0 )
			{
				m_lang = new Language( *thisLang );
				break;
			}
		}

		AppReleaseAssert( m_lang, "Couldn't find language '%s'", interfaceLanguage );
	}
	
    g_languageTable->LoadBaseLanguage( "data/language/english.txt" );
	if( stricmp( m_lang->m_name, "english" ) != 0 )
	{
		g_languageTable->LoadTranslation( m_lang->m_path );
	}

	SaveCurrentLanguage( m_lang );
}


bool LanguageTable::SaveCurrentLanguage( Language *_lang )
{
	char *curInterfaceLanguage = g_preferences->GetString( PREFS_INTERFACE_LANGUAGE );
	if( !curInterfaceLanguage || stricmp( curInterfaceLanguage, _lang->m_name ) != 0 )
	{
		g_preferences->SetString( PREFS_INTERFACE_LANGUAGE, _lang->m_name );
		g_preferences->Save();
		return true;
	}
	return false;
}


bool LanguageTable::GetLanguageSelectable( char *_name )
{
	int lenLanguages = m_languages.Size();
    for( int i = 0; i < lenLanguages; ++i )
    {
		Language *thisLang = m_languages.GetData( i );
		if( stricmp( thisLang->m_name, _name ) == 0 )
		{
			return thisLang->m_selectable;
		}
    }
	return false;
}


void LanguageTable::SetLanguageSelectable( char *_name, bool _selectable )
{
	int lenLanguages = m_languages.Size();
    for( int i = 0; i < lenLanguages; ++i )
    {
		Language *thisLang = m_languages.GetData( i );
		if( stricmp( thisLang->m_name, _name ) == 0 )
		{
			thisLang->m_selectable = _selectable;
			break;
		}
    }
}


void LanguageTable::LoadLanguage(char *_filename, BTree<char *> &_langTable )
{
    TextReader *in = g_fileSystem->GetTextReader(_filename);
	AppReleaseAssert(in && in->IsOpen(), "Couldn't open language file %s", _filename );

    //
	// Read all the phrases from the language file

	while (in->ReadLine())
	{
		if (!in->TokenAvailable()) continue;

		char *key = in->GetNextToken();	


#ifdef TARGET_OS_MACOSX
        // 
        // Special case hack
        // If this is the Mac version and there is a replacement string
        // Use the replacement string instead
        #define MACOSX_MARKER "macosx_"
        if( strncmp( key, MACOSX_MARKER, strlen(MACOSX_MARKER) ) == 0 )
        {
            key += strlen(MACOSX_MARKER);
        }
#endif


        //
		// Make sure the this key isn't already used

		if(_langTable.LookupTree(key))
        {
            //AppDebugOut( "Warning : found duplicate key '%s' in language file %s\n", key, _filename );
            _langTable.RemoveData( key );
        }

		char *aString = strdup( in->GetRestOfLine() ); 
        
        //
		// Make sure a language key always has a some text with it

		if( !aString || aString[0] == '\0' )
		{
            AppDebugOut( "Warning : found key '%s' with no translation in language file %s\n", key, _filename );

			if( aString )
			{
				delete [] aString;
			}
			continue;
		}

        //
        // Convert the string "\n" into a genuine '\n'

        for( unsigned int i = 0; i < strlen(aString)-1; ++i )
        {
            if( aString[i] == '\\' && aString[i+1] == 'n' )
            {
                aString[i] = ' ';
                aString[i+1] = '\n';
            }
        }
        
        //
        // Strip trailing '\n'
        
        int stringLength = strlen( aString );
        if( aString[stringLength-1] == '\n' ) 
        {
            aString[stringLength-1] = '\x0';
        }

        if( aString[stringLength-2] == '\r' )
        {
            aString[stringLength-2] = '\x0';
        }

		_langTable.PutData(key, aString);
	}

    delete in;
}


void LanguageTable::LoadBaseLanguage(char *_filename)
{
    LoadLanguage( _filename, m_baseLanguage );
}


void LanguageTable::LoadTranslation(char *_filename)
{
    LoadLanguage( _filename, m_translation );

#ifdef _DEBUG
    DArray<char *> *keys = m_baseLanguage.ConvertIndexToDArray();

    for( int i = 0; i < keys->Size(); ++i )
    {
        if( keys->ValidIndex(i) )
        {
            char *key = keys->GetData(i);
			if( !m_translation.GetData( key ) )
			{
				AppDebugOut( "Warning : base language key '%s' with no translation in language file %s\n", key, _filename );
			}
        }
    }

    delete keys;
#endif
}


void LanguageTable::ClearBaseLanguage()
{
    DArray<char *> *base = m_baseLanguage.ConvertToDArray();

    for( int i = 0; i < base->Size(); ++i )
    {
        if( base->ValidIndex(i) )
        {
            char *data = base->GetData(i);
            delete data;
        }
    }

    delete base;
    m_baseLanguage.Empty();
}


void LanguageTable::ClearTranslation()
{
    DArray<char *> *translation = m_translation.ConvertToDArray();
    
    for( int i = 0; i < translation->Size(); ++i )
    {
        if( translation->ValidIndex(i) )
        {
            char *data = translation->GetData(i);
            delete data;
        }
    }

    delete translation;
    m_translation.Empty();
}


bool LanguageTable::DoesPhraseExist(char *_key)
{
    if( !_key )
    {
        return false;
    }
    else
    {
        char *phrase = m_baseLanguage.GetData(_key);
        return( phrase != NULL );
    }
}


bool LanguageTable::DoesTranslationExist(char *_key)
{
    if( !_key )
    {
        return false;
    }
    else
    {
        char *phrase = m_translation.GetData(_key);
        return( phrase != NULL );
    }
}


char *LanguageTable::LookupBasePhrase(char *_key)
{
    char *result = NULL;

    if( _key )
    {
	    result = m_baseLanguage.GetData(_key);
    }
    
    if( !result )
    {
        static char safeAnswer[256];
        sprintf( safeAnswer, "[%s]", _key ? _key : "null" );
        result = safeAnswer;
    }

	return result;
}


char *LanguageTable::LookupPhrase(char *_key)
{
    char *result = NULL;

    if( _key )
    {
	    result = m_translation.GetData(_key);
	    if (!result) result = m_baseLanguage.GetData(_key);
    }
    
    if( !result )
    {
        static char safeAnswer[256];
        sprintf( safeAnswer, "[%s]", _key ? _key : "null" );
        result = safeAnswer;
    }

	return result;
}


void LanguageTable::TestTranslation( char *_logFilename )
{
    //
    // Open the errors file

    FILE *output = fopen( _logFilename, "wt" );
    AppReleaseAssert( output, "Failed to open logfile %s", _logFilename );
    
    
    //
    // Look for strings in the base that are not in the translation
    
    DArray<char *> *basePhraseIndex = m_baseLanguage.ConvertIndexToDArray();
    DArray<char *> *basePhrases = m_baseLanguage.ConvertToDArray();

    for( int i = 0; i < basePhraseIndex->Size(); ++i )
    {
        if( basePhraseIndex->ValidIndex(i) )
        {
            char *baseIndex = basePhraseIndex->GetData(i);
            char *basePhrase = basePhrases->GetData(i);

            if( !DoesTranslationExist( baseIndex ) )
            {
                fprintf( output, "ERROR : Failed to find translation for string ID '%s'\n", baseIndex );
            }
            else
            {
                char *translatedPhrase = m_translation.GetData( baseIndex );
                if( strcmp( basePhrase, translatedPhrase ) == 0 )
                {
                    fprintf( output, "Warning : String ID appears not to be translated : '%s'\n", baseIndex );
                }
            }
        }
    }

    delete basePhraseIndex;
    delete basePhrases;


    //
    // Look for phrases in the translation that are not in the base

    DArray<char *> *translatedIndex = m_translation.ConvertIndexToDArray();

    for( int i = 0; i < translatedIndex->Size(); ++i )
    {
        if( translatedIndex->ValidIndex(i) )
        {
            char *index = translatedIndex->GetData(i);

            if( !DoesPhraseExist( index ) )
            {
                fprintf( output, "Warning : Found new string ID not present in original English : '%s'\n", index );               
            }
        }
    }

    delete translatedIndex;

    
    //
    // Clean up
    
    fclose( output );
}


int LanguageTable::ReplaceStringFlag( char flag, char *string, char *subject )
{
    char final[4096];
    size_t bufferPosition = 0;
	size_t subjectLen = strlen( subject );
	int nbFound = 0;

    for( size_t i = 0; i <= subjectLen; ++i )
    {
        if( subject[i] == '*' &&
            i+1 < subjectLen )
        {
            if( subject[i+1] == flag )
            {
				size_t stringLen = strlen(string);
                for( size_t j = 0; j < stringLen; ++j )
                {                        
                    final[bufferPosition] = string[j];
                    bufferPosition++;
                }
                i++;
				nbFound++;
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
	return nbFound;
}


int LanguageTable::ReplaceIntegerFlag( char flag, int num, char *subject )
{
	char number[16];
	snprintf( number, sizeof(number), "%d", num );
	number[ sizeof(number) - 1 ] = '\0';

	return ReplaceStringFlag( flag, number, subject );
}


bool LanguageTable::DoesFlagExist( char flag, char *subject )
{
	if( !subject ) return false;

	size_t subjectLen = strlen( subject );
    for( size_t i = 0; i <= subjectLen; ++i )
    {
        if( subject[i] == '*' &&
            i+1 < subjectLen )
        {
            if( subject[i+1] == flag )
            {
				return true;
            }
        }
    }
	return false;
}


#ifdef TRACK_LANGUAGEPHRASE_ERRORS

void LanguageTable::ClearLanguagePhraseErrors()
{
    DArray<char *> *errors = m_languagePhraseErrors.ConvertToDArray();
    
    for( int i = 0; i < errors->Size(); ++i )
    {
        if( errors->ValidIndex(i) )
        {
            char *error = errors->GetData(i);
            delete [] error;
        }
    }

    delete errors;
    m_languagePhraseErrors.Empty();
}


char *LanguageTable::DebugLookupPhrase( char *_file, int _line, char *_key )
{
    char *result = LookupPhrase( _key );

	char *realKey;
	size_t lenRealKey;
	if( _key )
	{
		realKey = _key;
		lenRealKey = strlen( _key );
	}
	else
	{
		realKey = "null";
		lenRealKey = 4;
	}

	size_t len = strlen( result );
	if( len >= 2 && result[0] == '[' && result[len - 1] == ']' &&
	    strncmp( result + 1, realKey, lenRealKey ) == 0 )
	{
		size_t lenFullKey = lenRealKey + strlen( _file ) + 32;
		char *fullKey = new char[ lenFullKey + 1 ];
		snprintf( fullKey, lenFullKey + 1, "%s###%s###%d", realKey, _file, _line );

		if( !m_languagePhraseErrors.GetData( fullKey ) )
		{
			m_languagePhraseErrors.PutData( fullKey, fullKey );

			AppDebugOut( "Language Table cannot find phrase '%s'. %s:%d\n", 
						 realKey, _file, _line );
		}
		else
		{
			delete [] fullKey;
		}
	}

	return result;
}


bool LanguageTable::LanguageTableIsNewLanguagePhraseError( char *_file, int _line, char flag, char *subject, int nbFound )
{
	bool newError = false;

	if( nbFound > 1 || nbFound == 0 )
	{
		size_t lenFullKey = strlen( subject ) + strlen( _file ) + 32;
		char *fullKey = new char[ lenFullKey + 1 ];
		snprintf( fullKey, lenFullKey + 1, "%s###%c###%s###%d", subject, flag, _file, _line );

		if( !m_languagePhraseErrors.GetData( fullKey ) )
		{
			m_languagePhraseErrors.PutData( fullKey, fullKey );
			newError = true;
		}
		else
		{
			delete [] fullKey;
		}
	}

	return newError;
}


int LanguageTable::DebugReplaceStringFlag( char *_file, int _line, char flag, char *string, char *subject )
{
	int nbFound = ReplaceStringFlag( flag, string, subject );
	if( LanguageTableIsNewLanguagePhraseError( _file, _line, flag, subject, nbFound ) )
	{
		if( nbFound > 1 )
		{
			AppDebugOut( "Language Table found %d occurrence(s) of the flag '%c' in string '%s' and replaced it by string '%s'. %s:%d\n", 
						 nbFound, flag, subject, string, _file, _line );
		}
		else if( nbFound == 0 )
		{
			AppDebugOut( "Language Table cannot find flag '%c' in string '%s' and replace it by string '%s'. %s:%d\n", 
						 flag, subject, string, _file, _line );
		}
	}
	return nbFound;
}


int LanguageTable::DebugReplaceIntegerFlag( char *_file, int _line, char flag, int num, char *subject )
{
	int nbFound = ReplaceIntegerFlag( flag, num, subject );
	if( LanguageTableIsNewLanguagePhraseError( _file, _line, flag, subject, nbFound ) )
	{
		if( nbFound > 1 )
		{
			AppDebugOut( "Language Table found %d occurrence(s) of the flag '%c' in string '%s' and replaced it by the number '%d'. %s:%d\n", 
						 nbFound, flag, subject, num, _file, _line );
		}
		else if( nbFound == 0 )
		{
			AppDebugOut( "Language Table cannot find flag '%c' in string '%s' and replace it by the number '%d'. %s:%d\n", 
						 flag, subject, num, _file, _line );
		}
	}
	return nbFound;
}

#endif



// ============================================================================



Language::Language()
	: m_selectable(true)
{
    sprintf( m_name,    "unknown" );
    sprintf( m_path,    "unknown" );
    sprintf( m_caption, "unknown" );
}
