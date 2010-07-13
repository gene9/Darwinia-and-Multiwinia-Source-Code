#include "lib/universal_include.h"

#include <string>

#include "lib/input/input_types.h"
#include "app.h"
#include "lib/language_table.h"

using std::string;


InputDescription::InputDescription()
: noun(), verb(), pref() {}


InputDescription::InputDescription( InputDescription const &_desc )
: noun( _desc.noun ), verb( _desc.verb ),
  pref( _desc.pref ), translated( _desc.translated ) {}


InputDescription::InputDescription( string const &_noun, string const &_verb,
                                    string const &_pref, bool _translated )
: noun( _noun ), verb( _verb ),
  pref( _pref ), translated( _translated ) {}


void InputDescription::translate()
{
	if ( !translated ) {
		if ( ISLANGUAGEPHRASE( noun.c_str() ) )
			noun = LANGUAGEPHRASE( noun.c_str() );
		if ( ISLANGUAGEPHRASE( verb.c_str() ) )
			verb = LANGUAGEPHRASE( verb.c_str() );
		translated = true;
	}
}
