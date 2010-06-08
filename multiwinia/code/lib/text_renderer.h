#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

class RGBAColour;
class BitmapRGBA;
class Vector3;

#include "lib/unicode/unicode_string.h"


#define DEF_FONT_SIZE 12.0
#define OLD_TEX_ID_ARRAY_SIZE 256

static int	DecodeSurrogatePair		(WCHAR word1, WCHAR word2);

// ****************************************************************************
//  Class UnicodeFontInfo
// ****************************************************************************
class UnicodeFontInfo
{
public:
	BitmapRGBA*	bitmapFile;
	int upperBound, lowerBound, textureID, width, height;
	float lines, textHeight;
	char filename[512];
};

// ****************************************************************************
//  Class TextRenderer
// ****************************************************************************

class TextRenderer
{
protected:
	UnicodeFontInfo	*m_unicodebmpfiles;
	int				m_oldTextureIds[256];
	int				m_oldTextureIdsSize;
	double			m_projectionMatrix[16];
	double			m_modelviewMatrix[16];
	char			*m_filename;
	unsigned int	m_textureID;
	unsigned int	m_bitmapWidth;
	unsigned int	m_bitmapHeight;
    bool			m_renderShadow;    
    bool			m_renderOutline;
	bool			m_isunicode;
	int				m_bmpnum;
	int				m_currentbmpnum;
	int				m_bmparraysize;
    float           m_horizSpacingFactor;
	float			m_horizScaleFactor;
	float			m_horizScaleFactorDefault;

    float GetTexCoordX  ( wchar_t theChar );
    float GetTexCoordY  ( wchar_t theChar );
	int	GetTextureID	( WCHAR theChar );
	void AddUnicodeBitmapInfo	( int lower, int upper, char* _name, int id, float linesOfCharacters, int width, int height );
	BitmapRGBA* GetScaledUp		( BitmapRGBA* _bmp );
    
public:
	void Initialise(char const *_filename);
	void Shutdown();

    void SetHorizSpacingFactor( float factor=1.0f );
	void SetHorizScaleFactor( float factor=0.6f );
	void BuildOpenGlState();
	void BuildUnicodeArray();

    void BeginText2D	 ();
    void EndText2D		 ();
    
    void SetRenderShadow ( bool _renderShadow );
    void SetRenderOutline( bool _renderOutline );

    void DrawText2DSimple( float _x, float _y, float _size, char const *_text );
	void DrawText2DSimple( float _x, float _y, float _size, const UnicodeString &_text );

    void DrawText2D      ( float _x, float _y, float _size, char const *_text, ... );	// Like simple but with variable args
	void DrawText2D      ( float _x, float _y, float _size, const UnicodeString &_text );

    void DrawText2DRight ( float _x, float _y, float _size, char const *_text, ... );	// Like above but with right justify
	void DrawText2DRight ( float _x, float _y, float _size, const UnicodeString &_text );

    void DrawText2DCentre( float _x, float _y, float _size, char const *_text, ... );	// Like above but with centre justify
	void DrawText2DCentre( float _x, float _y, float _size, const UnicodeString &_text );

	void DrawText2DJustified( float _x, float _y, float _size, int _xJustification, char const *_text, ... );	// Like above but with variable justification
	void DrawText2DJustified( float _x, float _y, float _size, int _xJustification, const UnicodeString &_text );

    void DrawText2DUp    ( float _x, float _y, float _size, char const *_text );   // Like above but rotated 90 ccw
	void DrawText2DUp    ( float _x, float _y, float _size, const UnicodeString &_text );

    void DrawText2DDown  ( float _x, float _y, float _size, char const *_text );   // Like above but rotated 90 cw
	void DrawText2DDown  ( float _x, float _y, float _size, const UnicodeString &_text );
	
	
	void DrawText3DSimple( Vector3 const &_pos, float _size, char const *_text );
	void DrawText3DSimple( Vector3 const &_pos, float _size, const UnicodeString &_text );

	void DrawText3D		 ( Vector3 const &_pos, float _size, char const *_text, ... );
	void DrawText3D		 ( Vector3 const &_pos, float _size, const UnicodeString &_text );

	void DrawText3DCentre( Vector3 const &_pos, float _size, char const *_text, ... );
	void DrawText3DCentre( Vector3 const &_pos, float _size, const UnicodeString &_text );

	void DrawText3DRight ( Vector3 const &_pos, float _size, char const *_text, ... );
	void DrawText3DRight ( Vector3 const &_pos, float _size, const UnicodeString &_text );
    
    void DrawText3D      ( Vector3 const &_pos, Vector3 const &_front, Vector3 const &_up,
                           float _size, char const *_text, ... );

	void DrawText3D      ( Vector3 const &_pos, Vector3 const &_front, Vector3 const &_up,
                           float _size, const UnicodeString &_text );

    float GetTextWidth   ( unsigned int _numChars, float _size=13.0 );
    float GetTextWidthReal( const UnicodeString &_text, float _size=13.0f );

	bool IsUnicode		 ( );
};


extern TextRenderer g_gameFont;
extern TextRenderer g_editorFont;
extern TextRenderer g_oldGameFont;
extern TextRenderer g_oldEditorFont;
extern TextRenderer g_titleFont;


#endif
