#include "lib/universal_include.h"

#define JAL_DEBUG 0
#if JAL_DEBUG
	#include <iostream>
	#include "lib/debug.h"
	#define TRACE_RET_B(x)  TRACE_LINE( "-> " << B(x) )
	using namespace std;
#else
	#define TRACE_FUNC(x,y)
	#define TRACE_LINE(x)
	#define TRACE_RET_B(x)
#endif

#include "lib/input/input.h"
#include "lib/language_table.h"
#include "app.h"
#include "sepulveda.h"

#include "lib/hi_res_time.h"

#ifdef TARGET_MSVC
	#define snprintf _snprintf
#endif

#ifndef SEPULVEDA_MAX_PHRASE_LENGTH
	#define SEPULVEDA_MAX_PHRASE_LENGTH	1024
#endif
#define MAX_READ_KEY_LENGTH 64
#define MAX_FINAL_KEY_LENGTH 96


struct CaptionParserMode {
	bool writing;
	int ifdepth;
	int writingStopDepth;
	int inOffset, outOffset;
	InputMode mood;
	CaptionParserMode() : writing( true ), ifdepth( 0 ), writingStopDepth( 0 ),
	                      inOffset( 0 ), outOffset( 0 )
	{
		if ( g_inputManager ) mood = g_inputManager->getInputMode();
	}
};

bool buildCaption( char const *_baseString, char *_dest, CaptionParserMode &_mode );
bool buildPhrase( char const *_key, char *_dest, CaptionParserMode &_mode );
bool consumeMarker( char const *_baseString, char *_dest, CaptionParserMode &_mode );
bool consumeKeyMarker( char const *_baseString, char *_dest, CaptionParserMode &_mode );
bool consumeOtherMarker( char const *_baseString, char *_dest, CaptionParserMode &_mode );
bool consumeIfMarker( char const *_baseString, char *_dest, CaptionParserMode &_mode );


#if JAL_DEBUG
char head_str[50];

const char * head( char const * _str ) {
	int maxLen = 25;
	head_str[0] = '"';
	strncpy( head_str + 1, _str, maxLen - 2 );
	int len = 1;
	while ( len < maxLen && head_str[len] != '\0' ) ++len;
	if ( len >= maxLen )
		strcpy( head_str + (maxLen - 4), "...\"" );
	else
		strcpy( head_str + len, "\"" );
	return head_str;
}

ostream & operator<<( ostream &stream, CaptionParserMode &mode ) {
	stream << "CPM{ w = " << mode.writing << ", d = " << mode.ifdepth
	       << ", in = " << mode.inOffset << ", out = " << mode.outOffset
		   << ", mood = " << mode.mood << " }";
	return stream;
}
#endif


bool buildCaption( char const *_baseString, char *_dest, CaptionParserMode &_mode ) {
	TRACE_FUNC( "buildCaption", head( _baseString + _mode.inOffset ) << ", ..." );

	char ch;
	while ( ( ch = _baseString[ _mode.inOffset ] ) != '\0' ) {
		if ( ch == '[' ) {
			if ( !consumeMarker( _baseString, _dest, _mode ) ) {
				TRACE_RET_B( false );
				return false;
			}
		} else if ( _mode.writing ) {
			if ( _mode.outOffset < SEPULVEDA_MAX_PHRASE_LENGTH - 1 )
				_dest[ _mode.outOffset++ ] = ch;
			else {
				_dest[ _mode.outOffset ] = '\0';
				TRACE_RET_B( false );
				return false;
			}
		}
		++_mode.inOffset;
	}
	_dest[ _mode.outOffset ] = '\0';
	TRACE_RET_B( true );
	return true;
}


bool buildPhrase( char const *_key, char *_dest, CaptionParserMode &_mode ) {
	TRACE_FUNC( "buildPhrase", Q(_key) << ", ..." );

	bool done = false;
	if ( _mode.writing ) {
		char key[MAX_FINAL_KEY_LENGTH];
		strncpy( key, _key, MAX_FINAL_KEY_LENGTH - 1 );
		key[MAX_FINAL_KEY_LENGTH - 1] = '\0';
		strlwr( key );
		if ( MOODYISLANGUAGEPHRASE( key, _mode.mood ) ) {
			const char *definition = MOODYLANGUAGEPHRASE( key, _mode.mood );
			if ( definition ) {
				CaptionParserMode mode;
				mode.outOffset = _mode.outOffset;
				done = buildCaption( definition, _dest, mode );
				if ( done )
					_mode.outOffset = mode.outOffset;
			}
		}
	}

	TRACE_RET_B( done );

	return done;
}


// Return 0, or the length of the marker at the beginning of _in,
// including the brackets.
int markerLength( char const *_in ) {
	int len = 0;
	if ( _in[0] == '[' ) {
		while ( _in[++len] != ']' ) {
			if ( _in[len] == '\0' ) {
				len = -1;
				break;
			}
		}
		++len;
	}
	return len;
}


bool consumeMarker( char const *_baseString, char *_dest, CaptionParserMode &_mode ) {
	if ( consumeIfMarker( _baseString, _dest, _mode )
		|| consumeKeyMarker( _baseString, _dest, _mode )
		|| consumeOtherMarker( _baseString, _dest, _mode ) ) // Other must be last. It always succeeds.
	{
		return true;
	}
	strncpy( _dest, _baseString, SEPULVEDA_MAX_PHRASE_LENGTH - 1 );
	_dest[SEPULVEDA_MAX_PHRASE_LENGTH - 1] = '\0';
	return false;
}


bool consumeIfMarker( char const *_baseString, char *_dest, CaptionParserMode &_mode ) {
	TRACE_FUNC( "consumeIfMarker", head( _baseString + _mode.inOffset ) << ", ..." );

	bool done = false;
	char const * in = _baseString + _mode.inOffset;
	if ( strnicmp( in, "[IFMODE ", 8 ) == 0 ) {
		++_mode.ifdepth;
		if ( _mode.writing ) {
			if ( strnicmp( in + 8, "KEYBOARD]", 9 ) == 0 ) {
				if ( _mode.mood != INPUT_MODE_KEYBOARD ) {
					_mode.writing = false;
					_mode.writingStopDepth = _mode.ifdepth;
				}
				_mode.inOffset += 16;
				done = true;
			}
		} else {
			_mode.inOffset += markerLength( _baseString );
			done = true;
		}
	}
	else if ( strnicmp( in, "[ELSE]", 6 ) == 0 ) {
		if ( _mode.writing ) {
			_mode.writing = false;
			_mode.writingStopDepth = _mode.ifdepth;
		} else if ( _mode.ifdepth == _mode.writingStopDepth )
			_mode.writing = true;
		_mode.inOffset += 5;
		done = true;
	}
	else if ( strnicmp( in, "[ENDIF]", 7 ) == 0 ) {
		if ( !_mode.writing && _mode.ifdepth == _mode.writingStopDepth )
			_mode.writing = true;
		--_mode.ifdepth;
		_mode.inOffset += 6;
		done = true;
	}

	TRACE_RET_B( done );

	return done;
}


bool consumeKeyMarker( char const *_baseString, char *_dest, CaptionParserMode &_mode ) {
	TRACE_FUNC( "consumeKeyMarker", head( _baseString + _mode.inOffset ) << ", ..." );

	char const *in = _baseString + _mode.inOffset;
	bool done = false;
	if ( strnicmp( in, "[KEY", 4 ) == 0 ) {
		int len = markerLength( in ) - 5;
		if ( !_mode.writing && 0 < len && len < MAX_READ_KEY_LENGTH )
			done = true;
		else if ( _mode.writing && 0 < len && len < MAX_READ_KEY_LENGTH ) {
			char buf[MAX_READ_KEY_LENGTH];
			strncpy( buf, in + 4, len );
			buf[len] = '\0';

			controltype_t eventId = g_inputManager->getControlID( buf );
			if ( eventId >= 0 ) {
				// Lookup the key name for the binding
				InputDescription desc;
				if ( g_inputManager->getBoundInputDescription( static_cast<ControlType>(eventId), desc ) )
				{
					char const *keyName = desc.noun.c_str();

					// Is the caption long enough
					if( _mode.outOffset + strlen(keyName) < SEPULVEDA_MAX_PHRASE_LENGTH - 1 ) {
						// Write the key name into the caption
						strcpy(_dest + _mode.outOffset, keyName);
						_mode.outOffset += strlen(keyName);
						done = true;
					}
				}
			}
		}
		if ( done )
			_mode.inOffset += len + 4;
	}

	TRACE_RET_B( done );

	return done;
}


bool consumeOtherMarker( char const *_baseString, char *_dest, CaptionParserMode &_mode ) {
	TRACE_FUNC( "consumeOtherMarker", head( _baseString + _mode.inOffset ) << ", ..." );

	// Look in hash table
	const char *in = _baseString + _mode.inOffset;
	bool done = false;

	int len = markerLength( in ) - 2;
	if ( !_mode.writing && 0 < len && len < MAX_READ_KEY_LENGTH )
		done = true;
	else if ( 0 < len && len < MAX_READ_KEY_LENGTH ) {
		char key[MAX_READ_KEY_LENGTH];
		CaptionParserMode mode( _mode );

		if ( in[1] == ' ' ) {
			if ( mode.outOffset < SEPULVEDA_MAX_PHRASE_LENGTH - 1 )
				_dest[ mode.outOffset++ ] = ' ';
			strncpy(key, in + 2, len - 1);
			key[len - 1] = '\0';
		} else {
			strncpy(key, in + 1, len);
			key[len] = '\0';
		}

		//done = buildModalPhrase( key, _dest, mode );
		done = buildPhrase( key, _dest, mode );
		if ( done )
			_mode = mode;
	}

	_mode.inOffset += len + 1; // Always eat the input

	TRACE_RET_B( done );

	return true; // Always succeed
}


bool buildCaption(char const *_baseString, char *_dest) {
	CaptionParserMode mode;
	return buildCaption( _baseString, _dest, mode );
}


bool buildCaption(char const *_baseString, char *_dest, InputMode _mood) {
	CaptionParserMode mode;
	mode.mood = _mood;
	return buildCaption( _baseString, _dest, mode );
}
