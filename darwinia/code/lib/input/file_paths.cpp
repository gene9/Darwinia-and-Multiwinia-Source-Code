#include "lib/universal_include.h"

#include <map>

#include "lib/input/file_paths.h"
#include "app.h"
#include "lib/preferences.h"
#include "lib/resource.h"
#include "lib/text_stream_readers.h"

using std::string;

static const string INPUT_DATA_DIR        = "input/";
static const string INPUT_PREFS_FILE      = "input_preferences.txt";
static const string USER_INPUT_PREFS_FILE = "user_input_preferences.txt";
static const string KEYBOARD_LAYOUT       = "KeyboardLayout";

static std::map<int,string> s_localeNames;
typedef std::map<int,string>::const_iterator localeIt;

void readLocaleNames()
{
	TextReader *reader = g_app->m_resource->GetTextReader( INPUT_DATA_DIR + "locales.txt" );
	if ( reader && reader->IsOpen() ) {
		while ( reader->ReadLine() ) {
			char *k = reader->GetNextToken();
			if ( k ) {
				int key = atoi( k );
				char *rest = reader->GetRestOfLine();
				if ( rest )
					s_localeNames.insert( std::make_pair( key, rest ) );
			}
		}
	}
}

const string & InputPrefs::GetSystemPrefsPath()
{
	static string path = "";

	if ( path == "" ) {
		path = INPUT_DATA_DIR + INPUT_PREFS_FILE;
	}

	return path;
}


const string & InputPrefs::GetLocalePrefsPath()
{
	static string path = "";
	static string defLocale = "";

	if ( path == "" ) {
		if ( s_localeNames.empty() )
			readLocaleNames();
		int localeID = int(GetKeyboardLayout( 0 )) & 0xFFFF;
		localeIt it = s_localeNames.find( localeID );
		if ( it != s_localeNames.end() ) defLocale = it->second;
		string kb = g_prefsManager->GetString( KEYBOARD_LAYOUT.c_str(), const_cast<char*>(defLocale.c_str()) );
		if ( kb != "" ) {
			path = INPUT_DATA_DIR + "keyboards/" + kb + ".txt";
		}
	}

	return path;
}


const string & InputPrefs::GetUserPrefsPath()
{
	static string path = "";

	if ( path == "" ) {
		path = App::GetProfileDirectory();
		path.append( USER_INPUT_PREFS_FILE );
	}

	return path;
}
