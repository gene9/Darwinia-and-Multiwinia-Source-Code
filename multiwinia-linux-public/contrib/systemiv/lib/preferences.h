#ifndef INCLUDED_PREFERENCES_H
#define INCLUDED_PREFERENCES_H

#include <stdio.h>

#include "lib/tosser/hash_table.h"
#include "lib/tosser/fast_darray.h"


#define PREFS_SCREEN_WIDTH  		"ScreenWidth"
#define PREFS_SCREEN_HEIGHT		    "ScreenHeight"
#define PREFS_SCREEN_REFRESH		"ScreenRefresh"
#define PREFS_SCREEN_COLOUR_DEPTH	"ScreenColourDepth"
#define PREFS_SCREEN_Z_DEPTH		"ScreenZDepth"
#define PREFS_SCREEN_WINDOWED		"ScreenWindowed"
#define PREFS_SCREEN_ANTIALIAS		"ScreenAntiAliasing"

#define PREFS_SOUND_LIBRARY         "SoundLibrary"
#define PREFS_SOUND_MIXFREQ         "SoundMixFreq"
#define PREFS_SOUND_CHANNELS        "SoundChannels"
#define PREFS_SOUND_HW3D            "SoundHW3D"
#define PREFS_SOUND_SWAPSTEREO      "SoundSwapStereo"
#define PREFS_SOUND_DSPEFFECTS      "SoundDSP"
#define PREFS_SOUND_MEMORY          "SoundMemoryUsage"
#define PREFS_SOUND_MASTERVOLUME    "SoundMasterVolume"


class PreferencesItem;


// ******************
// Class PrefsManager
// ******************

class Preferences
{
protected:
    char                           *m_filename;
	HashTable<PreferencesItem *>    m_items;
    FastDArray<char *>              m_fileText;

protected:
	bool    IsLineEmpty     (char *_line);
	void    SaveItem        (FILE *out, PreferencesItem *_item);

public:
	Preferences             ();
	~Preferences            ();

	void    Load            (const char *_filename=NULL);        // If filename is NULL, then m_filename is used
	void    Save            ();

	char    *GetString      (char *_key, char *_default=NULL);
	float   GetFloat        (char *_key, float _default=-1.0f);
	int	    GetInt          (char *_key, int _default=-1);
	
	void    SetString       (char *_key, char *_string);
	void    SetFloat	    (char *_key, float _val);
	void    SetInt		    (char *_key, int _val);
    
    void    AddLine         (char *_line);
    
    bool    DoesKeyExist    (char *_key);
};




// ***************
// Class PrefsItem
// ***************

class PreferencesItem
{
public:	
	enum
	{
		TypeString,
		TypeFloat,
		TypeInt
	};
	
	char        *m_key;
	int			m_type;
	char		*m_str;
    int		    m_int;
	float	    m_float;
	
	bool		m_hasBeenWritten;

    PreferencesItem   ();
	PreferencesItem   (char *_line);
	PreferencesItem   (char *_key, char *_str);
	PreferencesItem   (char *_key, float _float);
	PreferencesItem   (char *_key, int _int);
	~PreferencesItem  ();
};



extern Preferences *g_preferences;


#endif
