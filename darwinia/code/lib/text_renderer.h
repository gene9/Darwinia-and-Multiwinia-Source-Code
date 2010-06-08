#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

class RGBAColour;
class Vector3;


#define DEF_FONT_SIZE 12.0f



// ****************************************************************************
//  Class TextRenderer
// ****************************************************************************

class TextRenderer
{
protected:
	double			m_projectionMatrix[16];
	double			m_modelviewMatrix[16];
	char			*m_filename;
	unsigned int    m_textureID;
	unsigned int	m_bitmapWidth;
	unsigned int	m_bitmapHeight;
    bool            m_renderShadow;    
    bool            m_renderOutline;

    float GetTexCoordX  ( unsigned char theChar );
    float GetTexCoordY  ( unsigned char theChar );
    
public:
	void Initialise(char const *_filename);

	void BuildOpenGlState();

    void BeginText2D	 ();
    void EndText2D		 ();
    
    void SetRenderShadow ( bool _renderShadow );
    void SetRenderOutline( bool _renderOutline );

    void DrawText2DSimple( float _x, float _y, float _size, char const *_text );
    void DrawText2D      ( float _x, float _y, float _size, char const *_text, ... );	// Like simple but with variable args
    void DrawText2DRight ( float _x, float _y, float _size, char const *_text, ... );	// Like above but with right justify
    void DrawText2DCentre( float _x, float _y, float _size, char const *_text, ... );	// Like above but with centre justify
	void DrawText2DJustified( float _x, float _y, float _size, int _xJustification, char const *_text, ... );	// Like above but with variable justification
    void DrawText2DUp    ( float _x, float _y, float _size, char const *_text, ... );   // Like above but rotated 90 ccw
    void DrawText2DDown  ( float _x, float _y, float _size, char const *_text, ... );   // Like above but rotated 90 cw
	
	
	void DrawText3DSimple( Vector3 const &_pos, float _size, char const *_text );
	void DrawText3D		 ( Vector3 const &_pos, float _size, char const *_text, ... );
	void DrawText3DCentre( Vector3 const &_pos, float _size, char const *_text, ... );
	void DrawText3DRight ( Vector3 const &_pos, float _size, char const *_text, ... );
    
    void DrawText3D      ( Vector3 const &_pos, Vector3 const &_front, Vector3 const &_up,
                           float _size, char const *_text, ... );

    float GetTextWidth   ( unsigned int _numChars, float _size=13.0f );
};


extern TextRenderer g_gameFont;
extern TextRenderer g_editorFont;


#endif
