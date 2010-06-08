#ifndef BITMAPFONT_H
#define BITMAPFONT_H

class Colour;
class BinaryReader;


// ****************************************************************************
//  Class BitmapFont
// ****************************************************************************

class BitmapFont
{
public:
	char			*m_filename;

protected:
	unsigned int    m_textureID;
    bool            m_horizontalFlip;
    float           m_spacing;
    bool            m_fixedWidth;
    float           m_leftMargin[256];
    float           m_rightMargin[256];
    
    float GetTexCoordX  ( unsigned char theChar );
    float GetTexCoordY  ( unsigned char theChar );
    float GetTexWidth   ( unsigned char theChar );
    
public:
    BitmapFont( const char *_filename, BinaryReader *_reader = NULL );
    
    void SetFixedWidth      ( bool fixedWidth );
    void SetHoriztonalFlip  ( bool _flip );
    void SetSpacing         ( float _spacing );

    void DrawText2DSimple( float _x, float _y, float _size, char const *_text );
    void DrawText2D      ( float _x, float _y, float _size, char const *_text, ... );	// Like simple but with variable args
    void DrawText2DRight ( float _x, float _y, float _size, char const *_text, ... );	// Like above but with right justify
    void DrawText2DCentre( float _x, float _y, float _size, char const *_text, ... );	// Like above but with centre justify

    float GetTextWidth   ( char const *_string, float _size );
};


#endif
