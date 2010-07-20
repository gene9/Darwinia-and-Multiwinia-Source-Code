#ifndef INCLUDED_INPUT_FILE_PATHS_H
#define INCLUDED_INPUT_FILE_PATHS_H

#include <string>

class InputPrefs {

private:
	InputPrefs(); // Don't instantiate
public:
	static const std::string & GetSystemPrefsPath();
	static const std::string & GetLocalePrefsPath();
	static const std::string & GetUserPrefsPath();

};

#endif // INCLUDED_INPUT_FILE_PATHS_H
