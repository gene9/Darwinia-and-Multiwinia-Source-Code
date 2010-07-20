#ifndef LANGUAGE_TABLE_H
#define LANGUAGE_TABLE_H

#include "lib/input/input_types.h"

#include <strstream>

#include "lib/llist.h"
#include "lib/btree.h"
#include "lib/hash_table.h"


class LangPhrase
{
public:
	char				*m_key;			// malloced. The name used as an ID
	char				*m_string;		// malloced. This bit is different for each language

	LangPhrase();
	~LangPhrase();
};


class LangTable
{
private:
    LangPhrase  m_notFound;
	BTree       <LangPhrase*>	m_phrasesRaw;
	HashTable   <int>          *m_phrasesKbd;
	HashTable   <int>          *m_phrasesXin;
	char *                      m_chunk;

	bool specific_key_exists  (const char * _key, InputMode _mood);
	bool RawDoesPhraseExist   (char const *_key);
	HashTable<int> *GetCurrentTable();
	HashTable<int> *GetCurrentTable(InputMode _mood);

	void RebuildTable( HashTable<int> *_phrases, std::ostrstream &stream, InputMode _mood );

public:
	LangTable   (char *_filename);
	~LangTable  ();

    void ParseLanguageFile    (char const *_filename);
	void RebuildTables        ();

	bool DoesPhraseExist      (char const *_key);
	char *LookupPhrase        (char const *_key);

	char *RawLookupPhrase     (char const *_key);

	bool RawDoesPhraseExist   (char const *_key, InputMode _mood);
	char *RawLookupPhrase     (char const *_key, InputMode _mood);

    DArray<LangPhrase *> *GetPhraseList();

    void TestAgainstEnglish();
};


LList <char *> *WordWrapText(const char *_string,
                             float _linewidth,
                             float _fontWidth,
                             bool _wrapToWindow=true);

#define LANGUAGEPHRASE(x)       g_app->m_langTable->LookupPhrase(x)
#define ISLANGUAGEPHRASE(x)     g_app->m_langTable->DoesPhraseExist(x)
#define ISLANGUAGEPHRASE_ANY(x) g_app->m_langTable->DoesPhraseExist(x)

#define RAWLANGUAGEPHRASE(x)           g_app->m_langTable->RawLookupPhrase(x)
#define MOODYLANGUAGEPHRASE(x,y)       g_app->m_langTable->RawLookupPhrase((x),(y))
#define MOODYISLANGUAGEPHRASE(x,y)     g_app->m_langTable->RawDoesPhraseExist((x),(y))

#endif
