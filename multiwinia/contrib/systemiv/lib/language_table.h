#ifndef LANGUAGE_TABLE_H
#define LANGUAGE_TABLE_H

#include "lib/tosser/btree.h"
#include "lib/tosser/llist.h"


#define    PREFS_INTERFACE_LANGUAGE        "InterfaceTextLanguage"


class Language;


class LanguageTable
{
protected:
	BTree       <char *> m_baseLanguage;                                                // Reference language (english)
    BTree       <char *> m_translation;                                                 // replacement translations

	char        *m_defaultLanguage;
	bool        m_onlyDefaultLanguageSelectable;

    void        LoadBaseLanguage    (char *_filename);
    void        LoadTranslation     (char *_filename);

	void        LoadLanguage        (char *_filename, BTree<char *> &_langTable);

    bool        LoadLanguageCaption (Language *lang);

#ifdef TRACK_LANGUAGEPHRASE_ERRORS
    BTree       <char *> m_languagePhraseErrors;
    void        ClearLanguagePhraseErrors             ();

    bool        LanguageTableIsNewLanguagePhraseError (char *_file, int _line, char flag, char *subject, int nbFound);
#endif

public:
	Language    *m_lang;
    LList       <Language *> m_languages;

public:
	LanguageTable   ();
	~LanguageTable  ();

    void        Initialise              ();        
    void        LoadLanguages           ();

	void        SetDefaultLanguage      (char *_name, bool _onlySelectable = false);
	void        LoadCurrentLanguage     ();
	bool        SaveCurrentLanguage     (Language *_lang);
	bool        GetLanguageSelectable   (char *_name);
	void        SetLanguageSelectable   (char *_name, bool _selectable);

    void        TestTranslation         (char *_logFilename);     

    void        ClearBaseLanguage       ();
    void        ClearTranslation        ();

    bool        DoesPhraseExist         (char *_key);
    bool        DoesTranslationExist    (char *_key);

	char        *LookupBasePhrase       (char *_key);
	char        *LookupPhrase           (char *_key);

    int         ReplaceStringFlag       (char flag, char *string, char *subject);
    int         ReplaceIntegerFlag      (char flag, int num, char *subject);

    bool        DoesFlagExist           (char flag, char *subject);

#ifdef TRACK_LANGUAGEPHRASE_ERRORS
	char        *DebugLookupPhrase      (char *_file, int _line, char *_key);
    int         DebugReplaceStringFlag  (char *_file, int _line, char flag, char *string, char *subject);
    int         DebugReplaceIntegerFlag (char *_file, int _line, char flag, int num, char *subject);
#endif

};



extern LanguageTable *g_languageTable;

#ifdef TRACK_LANGUAGEPHRASE_ERRORS
	#define LANGUAGEPHRASE(x)                           g_languageTable->DebugLookupPhrase(__FILE__,__LINE__,x)
	#define LPREPLACESTRINGFLAG(flag,string,subject)    g_languageTable->DebugReplaceStringFlag(__FILE__,__LINE__,flag,string,subject)
	#define LPREPLACEINTEGERFLAG(flag,num,subject)      g_languageTable->DebugReplaceIntegerFlag(__FILE__,__LINE__,flag,num,subject)
#else
	#define LANGUAGEPHRASE(x)                           g_languageTable->LookupPhrase(x)
	#define LPREPLACESTRINGFLAG(flag,string,subject)    g_languageTable->ReplaceStringFlag(flag,string,subject)
	#define LPREPLACEINTEGERFLAG(flag,num,subject)      g_languageTable->ReplaceIntegerFlag(flag,num,subject)
#endif



// ============================================================================


class Language
{
public:
    char    m_name      [256];
    char    m_path      [256];
    char    m_caption   [256];
	bool    m_selectable;

public:
    Language();
};


#endif
