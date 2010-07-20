#include "lib/universal_include.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/binary_stream_readers.h"
#include "lib/bitmap.h"
#include "lib/debug_utils.h"
#include "lib/filesys_utils.h"
#include "lib/resource.h"
#include "lib/vector3.h"

#include "app.h"
#include "camera.h"
#include "text_renderer.h"


TextRenderer g_gameFont;
TextRenderer g_editorFont;


// Horizontal size as a proportion of vertical size
#define HORIZONTAL_SIZE		0.6f

#define TEX_MARGIN			0.003f
#define TEX_STRETCH			(1.0f - (26.0f * TEX_MARGIN))

#define TEX_WIDTH			((1.0f / 16.0f) * TEX_STRETCH * 0.9f)
#define TEX_HEIGHT			((1.0f / 14.0f) * TEX_STRETCH)


// ****************************************************************************
//  Class TextRenderer
// ****************************************************************************

void TextRenderer::Initialise(char const *_filename)
{
	m_filename = strdup(_filename);
	BuildOpenGlState();
    m_renderShadow = false;
    m_renderOutline = false;
}


void TextRenderer::BuildOpenGlState()
{
	BinaryReader *reader = g_app->m_resource->GetBinaryReader(m_filename);
	char const *extension = GetExtensionPart(m_filename);
	BitmapRGBA bmp(reader, extension);
	delete reader;

	m_bitmapWidth = bmp.m_width * 2;
	m_bitmapHeight = bmp.m_height * 2;

	BitmapRGBA scaledUp(m_bitmapWidth, m_bitmapHeight);
	scaledUp.Blit(0, 0, bmp.m_width, bmp.m_height, &bmp, 0, 0, scaledUp.m_width, scaledUp.m_height, false);
	m_textureID = scaledUp.ConvertToTexture();
}


void TextRenderer::BeginText2D()
{
	GLint matrixMode;
	GLint v[4];

	/* Setup OpenGL matrices */
	glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
	glGetIntegerv(GL_VIEWPORT, &v[0]);
	glMatrixMode(GL_MODELVIEW);
	glGetDoublev(GL_MODELVIEW_MATRIX, m_modelviewMatrix);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glGetDoublev(GL_PROJECTION_MATRIX, m_projectionMatrix);
	glLoadIdentity();
	gluOrtho2D(v[0] - 0.325, v[0] + v[2] - 0.325, v[1] + v[3] - 0.325, v[1] - 0.325);

    glDisable( GL_LIGHTING );
    glColor3f(1.0f, 1.0f, 1.0f);

    glEnable    ( GL_BLEND );
    glDisable   ( GL_CULL_FACE );
    glDisable   ( GL_DEPTH_TEST );
    glDepthMask ( false );

	glMatrixMode(matrixMode);
}


void TextRenderer::EndText2D()
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(m_projectionMatrix);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(m_modelviewMatrix);

    glDepthMask ( true );
    glEnable    ( GL_DEPTH_TEST );
    glEnable    ( GL_CULL_FACE );
    glDisable   ( GL_BLEND );
}


float TextRenderer::GetTexCoordX( unsigned char theChar )
{
	float const charWidth = 1.0f / 16.0f; // In OpenGL texture UV space
    float xPos = (theChar % 16);
    float texX = xPos * charWidth + TEX_MARGIN + 0.002f;
    return texX;
}


float TextRenderer::GetTexCoordY( unsigned char theChar )
{
	float const charHeight = 1.0f / 14.0f;
    float yPos = (theChar >> 4) - 2;
    float texY = yPos * charHeight; // In OpenGL texture UV space
    texY = 1.0f - texY - charHeight + TEX_MARGIN + 0.001f;
    return texY;
}


void TextRenderer::DrawText2DUp( float _x, float _y, float _size, char const *_text, ... )
{
	float horiSize = _size * HORIZONTAL_SIZE;

	if( m_renderShadow )    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    else                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    glEnable        ( GL_TEXTURE_2D );
    glEnable        ( GL_BLEND );
    glBindTexture   ( GL_TEXTURE_2D, m_textureID );

    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

	unsigned numChars = strlen(_text);
    for( unsigned int i = 0; i < numChars; ++i )
    {
        unsigned char thisChar = _text[i];

		if (thisChar > 32)
		{
			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			glBegin( GL_QUADS );
				glTexCoord2f(texX, texY + TEX_HEIGHT);		        glVertex2f( _x,				_y + horiSize);
				glTexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	glVertex2f( _x,	            _y );
				glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex2f( _x + _size,	    _y );
				glTexCoord2f(texX, texY);							glVertex2f( _x + _size,     _y + horiSize);
			glEnd();
		}

        _y -= horiSize;
    }

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //glDisable       ( GL_BLEND );                             // Not here, Blending is enabled during Eclipse render
    glDisable       ( GL_TEXTURE_2D );
}


void TextRenderer::DrawText2DDown( float _x, float _y, float _size, char const *_text, ... )
{
	float horiSize = _size * HORIZONTAL_SIZE;

	if( m_renderShadow ||
        m_renderOutline )   glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    else                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    glEnable        ( GL_TEXTURE_2D );
    glEnable        ( GL_BLEND );
    glBindTexture   ( GL_TEXTURE_2D, m_textureID );

    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

	unsigned numChars = strlen(_text);
    for( unsigned int i = 0; i < numChars; ++i )
    {
        unsigned char thisChar = _text[i];

		if (thisChar > 32)
		{
			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			if( m_renderOutline )
			{
				for( int x = -1; x <= 1; ++x )
				{
					for( int y = -1; y <= 1; ++y )
					{
						if( x != 0 || y != 0 )
						{
			                glBegin( GL_QUADS );
				                glTexCoord2f(texX, texY + TEX_HEIGHT);		        glVertex2f( _x+x + _size,     _y+y);
				                glTexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	glVertex2f( _x+x + _size,	    _y+y+horiSize );
				                glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex2f( _x+x,	            _y+y+horiSize );
				                glTexCoord2f(texX, texY);							glVertex2f( _x+x,             _y+y);
			                glEnd();
                        }
                    }
                }
            }
            else
            {
			    glBegin( GL_QUADS );
				    glTexCoord2f(texX, texY + TEX_HEIGHT);		        glVertex2f( _x + _size,     _y);
				    glTexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	glVertex2f( _x + _size,	    _y+horiSize );
				    glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex2f( _x,	            _y+horiSize );
				    glTexCoord2f(texX, texY);							glVertex2f( _x,             _y);
			    glEnd();
            }
		}

        _y += horiSize;
    }

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //glDisable       ( GL_BLEND );                             // Not here, Blending is enabled during Eclipse render
    glDisable       ( GL_TEXTURE_2D );
}


void TextRenderer::SetRenderShadow ( bool _renderShadow )
{
    m_renderShadow = _renderShadow;
}


void TextRenderer::SetRenderOutline( bool _renderOutline )
{
    m_renderOutline = _renderOutline;
}


void TextRenderer::DrawText2DSimple(float _x, float _y, float _size, char const *_text )
{
	// Compatibility wank - needed to achieve the same behaviour the old code had
	_y -= 7.0f;
    _x -= 3.0f;

	float horiSize = _size * HORIZONTAL_SIZE;

	if( m_renderShadow ||
        m_renderOutline )   glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    else                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    glEnable        ( GL_TEXTURE_2D );
    glEnable        ( GL_BLEND );
    glBindTexture   ( GL_TEXTURE_2D, m_textureID );

    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

	unsigned numChars = strlen(_text);
    for( unsigned int i = 0; i < numChars; ++i )
    {
        unsigned char thisChar = _text[i];

		if (thisChar > 32)
		{
			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			if( m_renderOutline )
			{
				for( int x = -1; x <= 1; ++x )
				{
					for( int y = -1; y <= 1; ++y )
					{
						if( x != 0 || y != 0 )
						{
							glBegin( GL_QUADS );
								glTexCoord2f(texX, texY + TEX_HEIGHT);		        glVertex2f( _x+x,				_y+y );
								glTexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	glVertex2f( _x+x + horiSize,	_y+y );
								glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex2f( _x+x + horiSize,	_y+y + _size );
								glTexCoord2f(texX, texY);							glVertex2f( _x+x,				_y+y + _size );
							glEnd();
						}
					}
				}
			}
			else
			{
				glBegin( GL_QUADS );
					glTexCoord2f(texX, texY + TEX_HEIGHT);		        glVertex2f( _x,				_y );
					glTexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	glVertex2f( _x + horiSize,	_y );
					glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex2f( _x + horiSize,	_y + _size );
					glTexCoord2f(texX, texY);							glVertex2f( _x,				_y + _size );
				glEnd();
			}
		}

		_x += horiSize;
    }

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //glDisable       ( GL_BLEND );                             // Not here, Blending is enabled during Eclipse render
    glDisable       ( GL_TEXTURE_2D );
}

// Draw the text, justified depending on the _xJustification parameter.
//		_xJustification < 0		Right justified text
//	    _xJustification == 0	Centred text
//		_xJustification > 0		Left justified text
void TextRenderer::DrawText2DJustified( float _x, float _y, float _size, int _xJustification, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	if (_xJustification > 0)
		DrawText2DSimple( _x, _y, _size, buf );			// Left Justification
	else {
		float width = GetTextWidth( strlen(buf), _size );

		if (_xJustification < 0)
			DrawText2DSimple( _x - width, _y, _size, buf );	// Right Justification
		else
			DrawText2DSimple( _x - width/2, _y, _size, buf );	// Centre
	}
}


void TextRenderer::DrawText2D( float _x, float _y, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);
    DrawText2DSimple( _x, _y, _size, buf );
}


void TextRenderer::DrawText2DRight( float _x, float _y, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

    float width = GetTextWidth( strlen(buf), _size );
    DrawText2DSimple( _x - width, _y, _size, buf );
}


void TextRenderer::DrawText2DCentre( float _x, float _y, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

    float width = GetTextWidth( strlen(buf), _size );
    DrawText2DSimple( _x - width/2, _y, _size, buf );
}

namespace {
	class SaveGLEnabled {
	public:
		SaveGLEnabled( GLenum _what )
			: m_what( _what ),
			  m_value( glIsEnabled( _what ) )
		{
		}

		~SaveGLEnabled()
		{
			if (m_value)
				glEnable( m_what );
			else
				glDisable( m_what );
		}

	private:
		GLenum m_what;
		bool m_value;
	};

	class SaveGLTex2DParamI {
	public:
		SaveGLTex2DParamI( GLenum _what )
			: m_what( _what )
		{
			glGetTexParameteriv( GL_TEXTURE_2D, _what, &m_value );
		}

		~SaveGLTex2DParamI()
		{
			glTexParameteri( GL_TEXTURE_2D, m_what, m_value );
		}

	private:
		GLenum m_what;
		GLint m_value;
	};

	class SaveGLBlendFunc {
	public:
		SaveGLBlendFunc()
		{
			glGetIntegerv( GL_BLEND_SRC, &m_srcFactor );
			glGetIntegerv( GL_BLEND_DST, &m_dstFactor );
		}

		~SaveGLBlendFunc()
		{
			glBlendFunc( m_srcFactor, m_dstFactor );
		}

	private:
		GLint m_srcFactor, m_dstFactor;
	};

	class SaveGLColour {
	public:
		SaveGLColour()
		{
			glGetFloatv( GL_CURRENT_COLOR, m_colour );
		}

		~SaveGLColour()
		{
			glColor4fv( m_colour);
		}
	private:

		float m_colour[4];
	};

	class SaveGLFontDrawAttributes {
	public:
		SaveGLFontDrawAttributes()
			:	m_textureMagFilter( GL_TEXTURE_MAG_FILTER ),
				m_textureMinFilter( GL_TEXTURE_MIN_FILTER ),
				m_textureEnabled( GL_TEXTURE_2D ),
				m_depthTestEnabled( GL_DEPTH_TEST ),
				m_cullFaceEnabled( GL_CULL_FACE ),
				m_lightingEnabled( GL_LIGHTING ),
				m_blendEnabled( GL_BLEND )
		{
		}


	private:
		SaveGLColour m_colour;
		SaveGLTex2DParamI m_textureMagFilter, m_textureMinFilter;
		SaveGLBlendFunc m_blendFunc;
		SaveGLEnabled
			m_textureEnabled,
			m_depthTestEnabled,
			m_cullFaceEnabled,
			m_lightingEnabled,
			m_blendEnabled;
	};
}

void TextRenderer::DrawText3DSimple( Vector3 const &_pos, float _size, char const *_text )
{
	// Store the GL state
	SaveGLFontDrawAttributes saveAttribs;

	Camera *cam = g_app->m_camera;
    Vector3 pos(_pos);
	Vector3 vertSize = cam->GetUp() * _size;
	Vector3 horiSize = -cam->GetRight() * _size * HORIZONTAL_SIZE;
	unsigned int numChars = strlen(_text);
	pos += vertSize * 0.5f;


	//
	// Render the text

    glEnable        (GL_BLEND);
	glDisable		(GL_CULL_FACE);
	glDisable		(GL_LIGHTING);
	glDisable		(GL_DEPTH_TEST);
	//glColor3ub		(255,255,255);
	glEnable		(GL_TEXTURE_2D);
	glBindTexture   (GL_TEXTURE_2D, m_textureID);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

	if( m_renderShadow )    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    else                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    for( unsigned int i = 0; i < numChars; ++i )
    {
        unsigned char thisChar = _text[i];

		if (thisChar > 32)
		{
			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			glBegin( GL_QUADS );
				glTexCoord2f(texX, texY + TEX_HEIGHT);				glVertex3fv( (pos).GetData() );
				glTexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	glVertex3fv( (pos + horiSize).GetData() );
				glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex3fv( (pos + horiSize - vertSize).GetData() );
				glTexCoord2f(texX, texY);							glVertex3fv( (pos - vertSize).GetData() );
			glEnd();
		}

		pos += horiSize;
    }
}


void TextRenderer::DrawText3D( Vector3 const &_pos, float _size, char const *_text, ... )
{
	// Convert the variable argument list into a single string
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	DrawText3DSimple(_pos, _size, buf);
}


void TextRenderer::DrawText3DCentre( Vector3 const &_pos, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	Vector3 pos = _pos;
	float amount =  HORIZONTAL_SIZE * (float)strlen(buf) * 0.5f * _size;
	pos += g_app->m_camera->GetRight() * amount;

	DrawText3DSimple(pos, _size, buf);
}


void TextRenderer::DrawText3DRight( Vector3 const &_pos, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	Vector3 pos = _pos;
	float amount =  HORIZONTAL_SIZE * (float)strlen(buf) * _size;
	pos += g_app->m_camera->GetRight() * amount;

	DrawText3DSimple(pos, _size, buf);
}


void TextRenderer::DrawText3D( Vector3 const &_pos, Vector3 const &_front, Vector3 const &_up,
                               float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	SaveGLFontDrawAttributes saveAttribs;

	Camera *cam = g_app->m_camera;

    Vector3 pos = _pos;
	Vector3 vertSize = _up * _size;
	Vector3 horiSize = ( _up ^ _front ) * _size * HORIZONTAL_SIZE;
	unsigned int numChars = strlen(buf);
	pos -= horiSize * numChars * 0.5f;
    pos += vertSize * 0.5f;


	//
	// Render the text

    glEnable        (GL_BLEND);
	glDisable		(GL_CULL_FACE);
	glDisable		(GL_LIGHTING);
	//glDisable		(GL_DEPTH_TEST);
	//glColor3ub		(255,255,255);
	glEnable		(GL_TEXTURE_2D);
	glBindTexture   (GL_TEXTURE_2D, m_textureID);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

	if( m_renderShadow )    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    else                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    for( unsigned int i = 0; i < numChars; ++i )
    {
        unsigned char thisChar = buf[i];

		if (thisChar > 32)
		{
			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			glBegin( GL_QUADS );
				glTexCoord2f(texX, texY + TEX_HEIGHT);				glVertex3fv( (pos).GetData() );
				glTexCoord2f(texX + TEX_WIDTH, texY + TEX_HEIGHT);	glVertex3fv( (pos + horiSize).GetData() );
				glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex3fv( (pos + horiSize - vertSize).GetData() );
				glTexCoord2f(texX, texY);							glVertex3fv( (pos - vertSize).GetData() );
			glEnd();
		}

		pos += horiSize;
    }
}



float TextRenderer::GetTextWidth( unsigned int _numChars, float _size )
{
    return _numChars * _size * HORIZONTAL_SIZE;
}
