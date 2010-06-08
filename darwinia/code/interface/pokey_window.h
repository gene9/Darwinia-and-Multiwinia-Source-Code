#ifndef _included_pokeywindow_h
#define _included_pokeywindow_h

#ifdef SOUND_EDITOR

#include "interface/darwinia_window.h"
#include "sound/pokey.h"


struct FSOUND_STREAM;
class PokeyGraph;
class PokeyButton;


class PokeyWindow : public DarwiniaWindow
{
friend class PokeyGraph;
friend class PokeyButton;
private:
	FSOUND_STREAM *m_stream;
	int m_selectionType;
	int m_selectionId;

public:
    PokeyWindow( char *name );
	~PokeyWindow();

	void Create();

	void Advance();
    void Render( bool hasFocus );
};

#endif // SOUND_EDITOR

#endif
