#ifndef LANGUAGE_TABLE_H
#define LANGUAGE_TABLE_H

#include "lib/input/input_types.h"

#include <sstream>

#include "lib/tosser/llist.h"
#include "lib/tosser/btree.h"
#include "lib/tosser/hash_table.h"
#include "lib/input/input.h"

#include "lib/unicode/unicode_string.h"

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

class LangPhrase
{
public:
	char			*m_key;			// malloced. The name used as an ID
	UnicodeString	m_string;		// malloced. This bit is different for each language

	LangPhrase();
	~LangPhrase();
};


class LangTable
{
private:
    LangPhrase  m_notFound;
	BTree       <LangPhrase*>		m_phrasesRaw;
	HashTable   <UnicodeString*>	*m_phrasesKbd;
	HashTable   <UnicodeString*>	*m_phrasesXin;
	bool		m_loadedUnicodeFile;

	bool specific_key_exists  (const char * _key, InputMode _mood);
	bool RawDoesPhraseExist   (char const *_key);
	HashTable<UnicodeString*> *GetCurrentTable();
	HashTable<UnicodeString*> *GetCurrentTable(InputMode _mood);

	void RebuildTable( HashTable<UnicodeString*> *_phrases,  InputMode _mood );

public:
	LangTable   (char *_filename);
	~LangTable  ();

    void ParseLanguageFile					(char const *_filename);
	void RebuildTables						();

	bool DoesPhraseExist					(char const *_key);
	const UnicodeString &LookupPhrase		(char const *_key);

	const UnicodeString &RawLookupPhrase	(char const *_key);

	bool RawDoesPhraseExist					(char const *_key, InputMode _mood);
	const UnicodeString &RawLookupPhrase	(char const *_key, InputMode _mood);

    DArray<LangPhrase *> *GetPhraseList();

    void TestAgainstEnglish();     
    void ReplaceStringFlag					( char flag, char *string, char *subject );

	bool ReadUnicode();

	bool buildCaption(UnicodeString const &_baseString, UnicodeString& _dest);
	bool buildCaption(UnicodeString const &_baseString, UnicodeString& _dest, InputMode _mood);
	bool buildCaption( UnicodeString const &_baseString, UnicodeString& _dest, CaptionParserMode &_mode );
	bool buildPhrase( UnicodeString const &_baseString, UnicodeString& _dest, CaptionParserMode &_mode );
	bool consumeMarker( UnicodeString const &_baseString, UnicodeString& _dest, CaptionParserMode &_mode );
	bool consumeKeyMarker( UnicodeString const &_baseString, UnicodeString& _dest, CaptionParserMode &_mode );
	bool consumeOtherMarker( UnicodeString const &_baseString, UnicodeString& _dest, CaptionParserMode &_mode );
	bool consumeIfMarker( UnicodeString const &_baseString, UnicodeString& _dest, CaptionParserMode &_mode );
};


LList <char *> *WordWrapText(const char *_string, 
                             float _linewidth, 
                             float _fontWidth,
                             bool _wrapToWindow=true);

LList <UnicodeString *> *WordWrapText(const UnicodeString &_string, 
                             float _linewidth, 
                             float _fontWidth,
                             bool _wrapToWindow=true,
                             bool _forceWrap = false);

#define LANGUAGEPHRASE(x)       g_app->m_langTable->LookupPhrase(x)
#define ISLANGUAGEPHRASE(x)     g_app->m_langTable->DoesPhraseExist(x)
#define ISLANGUAGEPHRASE_ANY(x) g_app->m_langTable->DoesPhraseExist(x)

#define RAWLANGUAGEPHRASE(x)           g_app->m_langTable->RawLookupPhrase(x)
//#define MOODYLANGUAGEPHRASE(x,y)       g_app->m_langTable->RawLookupPhrase((x),(y))
//#define MOODYISLANGUAGEPHRASE(x,y)     g_app->m_langTable->RawDoesPhraseExist((x),(y))

#endif
