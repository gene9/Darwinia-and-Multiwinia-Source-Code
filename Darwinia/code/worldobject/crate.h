
#ifndef _included_crate_h
#define _included_crate_h

#include "worldobject/building.h"


class Crate : public Building
{
public:
	enum
	{
		TypeNone=0
	};

    int     m_contents;

public:
    Crate();

    void Initialise			( Building *_template );
    bool Advance            ( );

    void Render				( float _predictionTime );
    void RenderAlphas       ( float _predictionTime );
    bool RenderPixelEffect	( float _predictionTime );

    void Read				( TextReader *_in, bool _dynamic );
    void Write				( FileWriter *_out );

    bool IsInView();

    void ListSoundEvents    ( LList<char *> *_list );
};



#endif
