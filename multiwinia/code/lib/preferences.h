#ifndef INCLUDED_PREFERENCES_H
#define INCLUDED_PREFERENCES_H

#include <string>

#include "lib/tosser/hash_table.h"
#include "lib/tosser/fast_darray.h"
#include "lib/unicode/unicode_string.h"
#include "unicode/unicode_text_file_writer.h"
#include <map>
#include <list>

class PrefsItem;

// ******************
// Class PrefsManager
// ******************

class PrefsManager
{
private:
	typedef std::map<UnicodeString, PrefsItem> PrefsItemMap;
	typedef std::list<UnicodeString> FileTextList;

	PrefsItemMap m_items;
	FileTextList m_fileText;
	
    char *m_filename;

	bool IsLineEmpty(UnicodeString const &_line);
	void SaveItem(UnicodeTextFileWriter& out, PrefsItem &_item);

    void CreateDefaultValues();

public:
	PrefsManager(char const *_filename);
	PrefsManager(std::string const &_filename);
	~PrefsManager();

	void Load		(char const *_filename=NULL); // If filename is NULL, then m_filename is used
	void Save		();

	void Clear		();

	char const* GetString			(char const *_key, char const *_default=NULL) const;
	UnicodeString GetUnicodeString	(char const *_key, UnicodeString const &_default) const;
	float GetFloat					(char const *_key, float _default=-1.0f) const;
	int	  GetInt					(char const *_key, int _default=-1) const;

	// Set functions update existing PrefsItems or create new ones if key doesn't yet exist
	void SetString  (char const *_key, UnicodeString const &_val);
	void SetFloat	(char const *_key, float _val);
	void SetInt		(char const *_key, int _val);
    
    void AddLine    (UnicodeString _line);

    bool DoesKeyExist(char const *_key) const;
};


extern PrefsManager *g_prefsManager;


#endif
