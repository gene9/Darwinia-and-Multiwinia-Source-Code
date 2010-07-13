#define _CRT_SECURE_NO_DEPRECATE
#include "lib/universal_include.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "lib/debug_utils.h"
#include "lib/filesys/file_system.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/string_utils.h"

#include "preferences.h"


Preferences *g_preferences = NULL;


// ***************
// Class PreferencesItem
// ***************

PreferencesItem::PreferencesItem()
:   m_key(NULL),
    m_str(NULL),
    m_int(0),
    m_float(0.0f)
{
}

PreferencesItem::PreferencesItem(char *_line)
:	m_str(NULL), m_key(NULL)
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

	m_key = newStr(key);

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
        else if (m_type == TypeString)      m_str = newStr(value);
		else						        m_int = atoi(value);
	}
	else
	{
		m_type = TypeString;
		m_str = newStr(value);

        size_t len = strlen(m_str);
        if( m_str[len-1] == '\n' || m_str[len-1] == '\r' ) 
        {
            m_str[len-1] = '\x0';
        }
	}
}


PreferencesItem::PreferencesItem(char *_key, char *_str)
:	m_type(TypeString)
{
	m_key = newStr(_key);
	m_str = newStr(_str);
}


PreferencesItem::PreferencesItem(char *_key, float _float)
:	m_type(TypeFloat),
	m_float(_float)
{
	m_key = newStr(_key);
}


PreferencesItem::PreferencesItem(char *_key, int _int)
:	m_type(TypeInt),
	m_int(_int)
{
	m_key = newStr(_key);
}


PreferencesItem::~PreferencesItem()
{
	delete [] m_key;
	m_key = NULL;
	delete [] m_str;
	m_str = NULL;
}


// ******************
// Class Preferences
// ******************

Preferences::Preferences()
:	m_filename(NULL)
{    
}


Preferences::~Preferences()
{
    delete [] m_filename;
	m_filename = NULL;

	m_items.EmptyAndDelete();
	
	for ( int i = 0; i < m_fileText.Size(); i++ )
	{
		if ( m_fileText.ValidIndex ( i ) )
		{
			char *text = m_fileText.GetData ( i );
			delete [] text;
			m_fileText.RemoveData ( i );
		}
	}

    m_fileText.EmptyAndDelete();
}


bool Preferences::IsLineEmpty(char *_line)
{
	while (_line[0] != '\0')
	{
		if (_line[0] == '#') return true;
		if (isalnum(_line[0])) return false;
		++_line;
	}

	return true;
}	


void Preferences::Load(const char *_filename)
{
    if( _filename ) 
    { 
        delete [] m_filename; 
        m_filename = newStr(_filename); 
    }

	if (!_filename) _filename = m_filename;
   
    // Try to read preferences if they exist

    TextReader *reader = g_fileSystem->GetTextReader(_filename);

    if( reader && reader->IsOpen() )
    {
        while( reader->ReadLine() )
        {
            if( reader->TokenAvailable() )
            {
                char *line = reader->GetRestOfLine();
                AddLine(line);
            }
            else
            {
                AddLine(" \n" );
            }
        }
    }

    delete reader;
}


void Preferences::SaveItem(FILE *out, PreferencesItem *_item)
{
	switch (_item->m_type)
	{
		case PreferencesItem::TypeFloat:
			fprintf(out, "%s = %.2f\n", _item->m_key, _item->m_float);
			break;
		case PreferencesItem::TypeInt:
			fprintf(out, "%s = %d\n", _item->m_key, _item->m_int);
			break;
		case PreferencesItem::TypeString:
			fprintf(out, "%s = %s\n", _item->m_key, _item->m_str);
			break;
	}
	_item->m_hasBeenWritten = true;
}

			
void Preferences::Save()
{
	// We've got a copy of the plain text from the prefs file that we initially
	// loaded in the variable m_fileText. We use that file as a template with
	// which to create the new save file. Updated prefs items values are looked up
	// as it their turn to be output comes around. When we've finished we then
	// write out all the new prefs items because they didn't exist in m_fileText.

	// First clear the "has been written" flags on all the items
	for (unsigned int i = 0; i < m_items.Size(); ++i)
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
        AppDebugOut( "Failed to write preferences file %s\n", m_filename );
		return;
	}

    bool writtenSpace = false;

	for (int i = 0; i < m_fileText.Size(); ++i)
	{
		char *line = m_fileText[i];
		if (IsLineEmpty(line))
		{
            if( !writtenSpace )
            {
			    fprintf(out, line);
                writtenSpace = true;
            }
		}
		else
		{
			char const *c = line;
			char const *keyStart = NULL;
			char const *keyEnd = NULL;
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
			PreferencesItem *item = m_items.GetData(itemIndex);
			SaveItem(out, item);
            writtenSpace = false;
		}
	}

	// Finally output any items that haven't already been written
	for (unsigned int i = 0; i < m_items.Size(); ++i)
	{
		if (m_items.ValidIndex(i)) 
		{
			PreferencesItem *item = m_items.GetData(i);
			if (!item->m_hasBeenWritten)
			{
				SaveItem(out, item);
			}
		}
	}

	fclose(out);
}


float Preferences::GetFloat(char *_key, float _default)
{
	int index = m_items.GetIndex(_key);
	if (index == -1) return _default;
	PreferencesItem *item = m_items.GetData(index);
	if (item->m_type != PreferencesItem::TypeFloat) return _default;
	return item->m_float;
}


int Preferences::GetInt(char *_key, int _default)
{
	int index = m_items.GetIndex(_key);
	if (index == -1) return _default;
	PreferencesItem *item = m_items.GetData(index);
	if (item->m_type != PreferencesItem::TypeInt) return _default;
	return item->m_int;
}


char *Preferences::GetString(char *_key, char *_default)
{
	int index = m_items.GetIndex(_key);
	if (index == -1) return _default;
	PreferencesItem *item = m_items.GetData(index);
	if (item->m_type != PreferencesItem::TypeString) return _default;
	return item->m_str;
}


void Preferences::SetString(char *_key, char *_string)
{
	int index = m_items.GetIndex(_key);

	if (index == -1)
	{
		PreferencesItem *item = new PreferencesItem(_key, _string);
		m_items.PutData(item->m_key, item);
	}
	else
	{
		PreferencesItem *item = m_items.GetData(index);
		AppDebugAssert(item->m_type == PreferencesItem::TypeString);
		char *newString = newStr(_string);
        delete [] item->m_str;
        // Note by Chris:
        // The incoming value of _string might also be item->m_str
        // So it is essential to copy _string before deleting item->m_str
		item->m_str = newString;
	}
}


void Preferences::SetFloat(char *_key, float _float)
{
	int index = m_items.GetIndex(_key);

	if (index == -1)
	{
		PreferencesItem *item = new PreferencesItem(_key, _float);
		m_items.PutData(item->m_key, item);
	}
	else
	{
		PreferencesItem *item = m_items.GetData(index);
		AppDebugAssert(item->m_type == PreferencesItem::TypeFloat);
		item->m_float = _float;
	}
}


void Preferences::SetInt(char *_key, int _int)
{
	int index = m_items.GetIndex(_key);

	if (index == -1)
	{
		PreferencesItem *item = new PreferencesItem(_key, _int);
		m_items.PutData(item->m_key, item);
	}
	else
	{
		PreferencesItem *item = m_items.GetData(index);
		AppDebugAssert(item->m_type == PreferencesItem::TypeInt);
		item->m_int = _int;
	}
}


void Preferences::AddLine(char *_line)
{	
    char localCopy[256];
    strcpy( localCopy, _line );
	if (!IsLineEmpty(localCopy))				// Skip comment lines and blank lines
	{
		char *c = localCopy;
		while (c[1] != '\0') c++;
		if (c[0] == '\n')
		{
			c[0] = '\0';
		}
		PreferencesItem *item = new PreferencesItem(localCopy);
		//m_items.PutData(item->m_key, item);

        if( !DoesKeyExist(item->m_key) )
        {
  	        char *lineCopy = newStr(_line);
            m_fileText.PutData(lineCopy);
        }

        switch( item->m_type )
        {
            case PreferencesItem::TypeInt:      SetInt( item->m_key, item->m_int );         break;
            case PreferencesItem::TypeFloat:    SetFloat( item->m_key, item->m_float );     break;
            case PreferencesItem::TypeString:   SetString( item->m_key, item->m_str );      break;
        }
	}  
    else
    {
  	    char *lineCopy = newStr(_line);
        m_fileText.PutData(lineCopy);
    }
}


bool Preferences::DoesKeyExist(char *_key)
{
	int index = m_items.GetIndex(_key);

	return index != -1;
}
