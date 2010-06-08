#include "lib/universal_include.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef TARGET_MSVC
	#include <io.h>
#endif

#include "lib/filesys/binary_stream_readers.h"
#include "lib/bitmap.h"
#include "lib/debug_utils.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/resource.h"
#include "lib/vector3.h"
#include "lib/preferences.h"

#include "app.h"
#include "camera.h"
#include "text_renderer.h"

#include "lib/unicode/unicode_string.h"


TextRenderer g_gameFont;
TextRenderer g_editorFont;
TextRenderer g_oldGameFont;
TextRenderer g_oldEditorFont;
TextRenderer g_titleFont;

// Horizontal size as a proportion of vertical size
//#define HORIZONTAL_SIZE		0.6f

#define TEX_MARGIN			0.003f
#define TEX_STRETCH			(1.0f - (26.0f * TEX_MARGIN))

#define TEX_WIDTH			((1.0f / 16.0f) * TEX_STRETCH * 0.9f)
#define TEX_HEIGHT			((1.0f / 14.0f) * TEX_STRETCH)

#define STARTING_FONT_ARRAY_SIZE	8

// ****************************************************************************
//  Class TextRenderer
// ****************************************************************************


void TextRenderer::Initialise(char const *_filename)
{
	// If it's a filename, use the old code
	m_filename = strdup(_filename);

	memset(m_oldTextureIds, -1, OLD_TEX_ID_ARRAY_SIZE);
	m_oldTextureIdsSize = 0;

	if (strlen(m_filename) > 4 && strcmp(&m_filename[strlen(m_filename)-4],".bmp") == 0)
	{
		m_isunicode = false;
		BuildOpenGlState();
	}
	// Otherwise, it's a partial filename for use with unicode bmps
	else
	{
		m_isunicode = true;
		m_bmparraysize = STARTING_FONT_ARRAY_SIZE;
		BuildUnicodeArray();
	}

	m_renderShadow = false;
	m_renderOutline = false;
    m_horizSpacingFactor = 1.0f;
	m_horizScaleFactor = 0.6f;
	m_horizScaleFactorDefault = 0.6f;
}

void TextRenderer::SetHorizScaleFactor( float factor )
{
	m_horizScaleFactor = factor;
}

void TextRenderer::SetHorizSpacingFactor( float factor )
{
    m_horizSpacingFactor = factor;
}

void TextRenderer::Shutdown()
{
	for( int i = 0; i < m_bmpnum; i++ )
	{
		delete m_unicodebmpfiles[i].bitmapFile;
		m_unicodebmpfiles[i].bitmapFile = NULL;
	}
	delete [] m_unicodebmpfiles;
	m_unicodebmpfiles = NULL;
	m_bmpnum = 0;
}
BitmapRGBA* TextRenderer::GetScaledUp ( BitmapRGBA* _bmp )
{
	m_bitmapWidth = _bmp->m_width * 2;
	m_bitmapHeight = _bmp->m_height * 2;

	BitmapRGBA* scaledUp = new BitmapRGBA(m_bitmapWidth, m_bitmapHeight);
	scaledUp->Blit(0, 0, _bmp->m_width, _bmp->m_height, _bmp, 0, 0, scaledUp->m_width, scaledUp->m_height, false);

	delete _bmp;

	return scaledUp;
}


void TextRenderer::AddUnicodeBitmapInfo(int lower, int upper, char* _name, int id, float linesOfCharacters, int width, int height )
{
	UnicodeFontInfo uni;
	uni.lowerBound = lower;
	uni.upperBound = upper;
	uni.textureID = id;
	uni.lines = linesOfCharacters;
	uni.width = width;
	uni.height = height;
	uni.bitmapFile = NULL;
	uni.textHeight = ((1.0f / uni.lines) * (1.0f - (26.0f * (0.003f * (uni.height/448)))));
	strncpy(uni.filename, _name, 256);

	//AppDebugOut("Adding Unicode bmp info: %d:%d:%d", lower, upper, id);

	if (m_bmpnum >= m_bmparraysize)
	{
		//AppDebugOut("Increasing Unicode info array size from %d to %d", m_bmparraysize, m_bmparraysize*2);
		UnicodeFontInfo* newArray = new UnicodeFontInfo[m_bmparraysize*2];
		memcpy(newArray, m_unicodebmpfiles,sizeof(UnicodeFontInfo)*m_bmparraysize);
		delete[] m_unicodebmpfiles;
		m_unicodebmpfiles = newArray;
		m_bmparraysize*=2;
	}

	m_unicodebmpfiles[m_bmpnum] = uni;
	m_currentbmpnum = m_bmpnum;
	m_bmpnum++;
	m_textureID = id;
}

#ifdef USE_DIRECT3D
bool GeneratingMipmaps();
#endif


void TextRenderer::BuildUnicodeArray()
{
	// Clean up
	if( m_unicodebmpfiles )
	{
		GLuint* gluintArray = new GLuint[m_bmpnum];
		int counter = 0;
		for( int i = 0; i < m_bmpnum; i++ )
		{
			if( m_unicodebmpfiles[i].bitmapFile )
			{
				delete m_unicodebmpfiles[i].bitmapFile;
			}
			gluintArray[counter] = m_unicodebmpfiles[i].textureID;
			counter++;
		}
		glDeleteTextures(counter, gluintArray);
		delete [] m_unicodebmpfiles;
		delete [] gluintArray;
	}

	m_unicodebmpfiles = new UnicodeFontInfo[m_bmparraysize];
	m_bmpnum = 0;

	char bmpfilename[128];
	char filter[128];

#define EXTENSION "bmp"

	sprintf(filter, "%s.*.*." EXTENSION, m_filename);
	LList<char*> *unicodefiles = g_app->m_resource->ListResources("textures/unicode/",filter, false);

	int numFiles = unicodefiles->Size();
	AppDebugOut("%d unicode " EXTENSION "s found: %s\n", numFiles, m_filename);
	for (int i = 0; i < numFiles; i++)
	{
		char* currentfilename = unicodefiles->GetData(i);
		sprintf(bmpfilename,"textures/unicode/%s",currentfilename);

		int textureId = g_app->m_resource->GetTexture( bmpfilename );

		int width, height;
		g_app->m_resource->GetTextureInfo( bmpfilename, width, height );

		int lower, upper;
		float lines = height / (width / 16.0f);
		char format[64];
		sprintf(format, "%s.%s", m_filename, "%X.%X." EXTENSION );
		//AppDebugOut("Format is %s",format);

		if( sscanf(currentfilename,format,&lower,&upper) == 2 )
		{
			AddUnicodeBitmapInfo(
				lower, upper, currentfilename, textureId, lines, width, height );
		}
	}
	
	unicodefiles->EmptyAndDeleteArray();
	delete unicodefiles;

	AppDebugOut("Finished building unicode textures.\n");
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
	m_textureID = scaledUp.ConvertToTextureAsync();
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

    float left = v[0] - 0.325;
    float right = v[0] + v[2] - 0.325;
    float bottom = v[1] + v[3] - 0.325;
    float top = v[1] - 0.325;

    float overscan = 0.0f;
    left -= overscan;
    right += overscan;
    bottom += overscan;
    top -= overscan;
    
	gluOrtho2D(left,right,bottom,top);

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


float TextRenderer::GetTexCoordX( wchar_t theChar )
{
	if (m_isunicode)
	{
		float const charWidth = 1.0f / 16.0f; // In OpenGL texture UV space
		float xPos = ((theChar-(m_unicodebmpfiles[m_currentbmpnum].lowerBound%16)) % 16);
		float texX = xPos * charWidth + (0.003f * (m_unicodebmpfiles[m_currentbmpnum].width/512)) + 0.002f;
		return texX;
	}
	else
	{
		float const charWidth = 1.0f / 16.0f; // In OpenGL texture UV space
		float xPos = (theChar % 16);
		float texX = xPos * charWidth + TEX_MARGIN + 0.002f;
		return texX;
	}
}


float TextRenderer::GetTexCoordY( wchar_t theChar )
{
	if (m_isunicode)
	{
		float const charHeight = 1.0f / m_unicodebmpfiles[m_currentbmpnum].lines; //14.0f;
		float yPos = (theChar >> 4) - (m_unicodebmpfiles[m_currentbmpnum].lowerBound >> 4);
		float texY = yPos * charHeight; // In OpenGL texture UV space
		texY = 1.0f - texY - charHeight + (0.003f * (m_unicodebmpfiles[m_currentbmpnum].height/448)) + 0.001f;
		return texY;
	}
	else
	{
		float const charHeight = 1.0f / 14.0f;
		float yPos = (theChar >> 4) - 2;
		float texY = yPos * charHeight; // In OpenGL texture UV space
		texY = 1.0f - texY - charHeight + TEX_MARGIN + 0.001f;
		return texY;
	}
}

int TextRenderer::GetTextureID(WCHAR theChar)
{
	if (!m_isunicode)
	{
		return m_textureID;
	}

	if (theChar >= m_unicodebmpfiles[m_currentbmpnum].lowerBound &&
		theChar <= m_unicodebmpfiles[m_currentbmpnum].upperBound)
	{
		return m_currentbmpnum;
	}

	for (int i = 0; i < m_bmpnum; i++)
	{
		if (theChar >= m_unicodebmpfiles[i].lowerBound &&
			theChar <= m_unicodebmpfiles[i].upperBound)
		{
			return i;
		}
	}

	return -1;
}

void TextRenderer::DrawText2DUp( float _x, float _y, float _size, char const *_text )
{
	UnicodeString u(_text);
	DrawText2DUp(_x,_y,_size, u);
}

void TextRenderer::DrawText2DUp( float _x, float _y, float _size, const UnicodeString &_text )
{
	float horiSize = _size * m_horizScaleFactor;
	float defaultHorizSize = _size * m_horizScaleFactorDefault;

	if( m_renderShadow )    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );    
    else                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );    

	float textHeight = ((1.0f / 14.0f) * TEX_STRETCH);

	if (m_isunicode)
	{
		textHeight = m_unicodebmpfiles[m_currentbmpnum].textHeight;
	}

    glEnable        ( GL_TEXTURE_2D );
    glEnable        ( GL_BLEND );
    glBindTexture   ( GL_TEXTURE_2D, m_textureID );
    
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    
	glBegin( GL_QUADS );

	unsigned numChars = _text.WcsLen();
	wchar_t lastChar = L'\0';
    for( unsigned int i = 0; i < numChars; ++i )
    {       
		WCHAR thisChar = _text.m_unicodestring[i];

		if( thisChar > 0xD800 && thisChar < 0xE000 )
		{
			if( lastChar != L'\0' )
			{
				thisChar = DecodeSurrogatePair(lastChar, thisChar);
				lastChar = L'\0';
			}
			else
			{
				lastChar = thisChar;
				continue;
			}
		}

		if( thisChar > 32 )
		{
			if (m_isunicode)
			{
				int bmpID = GetTextureID(thisChar);
				if (bmpID != m_currentbmpnum && bmpID >= 0)
				{
					m_currentbmpnum = bmpID;
					m_textureID = m_unicodebmpfiles[bmpID].textureID;
					textHeight = m_unicodebmpfiles[m_currentbmpnum].textHeight;

					glEnd();
					glBindTexture( GL_TEXTURE_2D, m_textureID );
					glBegin( GL_QUADS );
				}
				else if (bmpID == -1)
				{
					// Didn't find the correct texture...
				}
			}

			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			glTexCoord2f(texX, texY + textHeight);		        glVertex2f( _x,				_y + (thisChar < 0x3000 ? defaultHorizSize : horiSize) );
			glTexCoord2f(texX + TEX_WIDTH, texY + textHeight);	glVertex2f( _x,	            _y );
			glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex2f( _x + _size,	    _y );
			glTexCoord2f(texX, texY);							glVertex2f( _x + _size,     _y + (thisChar < 0x3000 ? defaultHorizSize : horiSize) );
		}

        _y -= (thisChar < 0x3000 ? defaultHorizSize : horiSize) ;    
    }

	glEnd();

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //glDisable       ( GL_BLEND );                             // Not here, Blending is enabled during Eclipse render
    glDisable       ( GL_TEXTURE_2D );
}


void TextRenderer::DrawText2DDown( float _x, float _y, float _size, char const *_text )
{
	UnicodeString u(_text);
	DrawText2DDown(_x,_y,_size,u);
}

void TextRenderer::DrawText2DDown( float _x, float _y, float _size, const UnicodeString &_text )
{
	float horiSize = _size * m_horizScaleFactor;
	float defaultHorizSize = _size * m_horizScaleFactorDefault;

	if( m_renderShadow ||
        m_renderOutline )   glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );    
    else                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );  

	float textHeight = ((1.0f / 14.0f) * TEX_STRETCH);

	if (m_isunicode)
	{
		textHeight = m_unicodebmpfiles[m_currentbmpnum].textHeight;
	}

    glEnable        ( GL_TEXTURE_2D );
    glEnable        ( GL_BLEND );
    glBindTexture   ( GL_TEXTURE_2D, m_textureID );
    
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    
	glBegin( GL_QUADS );

	unsigned numChars = _text.WcsLen();
	wchar_t lastChar = L'\0';
    for( unsigned int i = 0; i < numChars; ++i )
    {       
		WCHAR thisChar = _text.m_unicodestring[i];

		if( thisChar > 0xD800 && thisChar < 0xE000 )
		{
			if( lastChar != L'\0' )
			{
				thisChar = DecodeSurrogatePair(lastChar, thisChar);
				lastChar = L'\0';
			}
			else
			{
				lastChar = thisChar;
				continue;
			}
		}

		if (thisChar > 32)
		{
			if (m_isunicode)
			{
				int bmpID = GetTextureID(thisChar);
				if (bmpID != m_currentbmpnum && bmpID >= 0)
				{
					m_currentbmpnum = bmpID;
					m_textureID = m_unicodebmpfiles[bmpID].textureID;
					textHeight = m_unicodebmpfiles[m_currentbmpnum].textHeight;

					glEnd();
					glBindTexture( GL_TEXTURE_2D, m_textureID );
					glBegin( GL_QUADS );
				}
				else if (bmpID == -1)
				{
					// Didn't find the correct texture...
				}
			}

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
				                glTexCoord2f(texX, texY + textHeight);		        glVertex2f( _x+x + _size,     _y+y);
				                glTexCoord2f(texX + TEX_WIDTH, texY + textHeight);	glVertex2f( _x+x + _size,	    _y+y+(thisChar < 0x3000 ? defaultHorizSize : horiSize)  );
				                glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex2f( _x+x,	            _y+y+(thisChar < 0x3000 ? defaultHorizSize : horiSize)  );
				                glTexCoord2f(texX, texY);							glVertex2f( _x+x,             _y+y);
                        }
                    }
                }
            }
            else
            {
				    glTexCoord2f(texX, texY + textHeight);		        glVertex2f( _x + _size,     _y);
				    glTexCoord2f(texX + TEX_WIDTH, texY + textHeight);	glVertex2f( _x + _size,	    _y+(thisChar < 0x3000 ? defaultHorizSize : horiSize)  );
				    glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex2f( _x,	            _y+(thisChar < 0x3000 ? defaultHorizSize : horiSize)  );
				    glTexCoord2f(texX, texY);							glVertex2f( _x,             _y);
            }
		}

        _y += (thisChar < 0x3000 ? defaultHorizSize : horiSize) ;    
    }

	glEnd();

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
	UnicodeString u(_text);
	DrawText2DSimple(_x,_y,_size,u);
}

void TextRenderer::DrawText2DSimple( float _x, float _y, float _size, const UnicodeString &_text )
{
	// Compatibility wank - needed to achieve the same behaviour the old code had
	_y -= 7.0f;
    _x -= 3.0f;

	float horiSize = _size * m_horizScaleFactor;
	float defaultHorizSize = _size * m_horizScaleFactorDefault;
	float textHeight = ((1.0f / 14.0f) * TEX_STRETCH);

	if (m_isunicode)
	{
		textHeight = m_unicodebmpfiles[m_currentbmpnum].textHeight;
	}

	if( m_renderShadow ||
        m_renderOutline )   glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );    
    else                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );    

    glEnable        ( GL_TEXTURE_2D );
    glEnable        ( GL_BLEND );
    glBindTexture   ( GL_TEXTURE_2D, m_textureID );
    
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    
	glBegin( GL_QUADS );

	unsigned numChars = _text.WcsLen();
	wchar_t lastChar = L'\0';
    for( unsigned int i = 0; i < numChars; ++i )
    {       
		WCHAR thisChar = _text.m_unicodestring[i];

		if( thisChar > 0xD800 && thisChar < 0xE000 )
		{
			if( lastChar != L'\0' )
			{
				thisChar = DecodeSurrogatePair(lastChar, thisChar);
				lastChar = L'\0';
			}
			else
			{
				lastChar = thisChar;
				continue;
			}
		}

		if (thisChar > 32)
		{
			if (m_isunicode)
			{
				int bmpID = GetTextureID(thisChar);
				if (bmpID != m_currentbmpnum && bmpID >= 0)
				{
					m_currentbmpnum = bmpID;
					m_textureID = m_unicodebmpfiles[bmpID].textureID;
					textHeight = m_unicodebmpfiles[m_currentbmpnum].textHeight;

					glEnd();
					glBindTexture( GL_TEXTURE_2D, m_textureID );
					glBegin( GL_QUADS );
				}
				else if (bmpID == -1)
				{
					// Didn't find the correct texture...
				}
			}

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
                            float xDiff = x * 1.0f;
                            float yDiff = y * 1.0f;

							glTexCoord2f(texX, texY + textHeight);		        glVertex2f( _x+xDiff,				_y+yDiff );
							glTexCoord2f(texX + TEX_WIDTH, texY + textHeight);	glVertex2f( _x+xDiff + (thisChar < 0x3000 ? defaultHorizSize : horiSize),	_y+yDiff );
							glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex2f( _x+xDiff + (thisChar < 0x3000 ? defaultHorizSize : horiSize),	_y+yDiff + _size );
							glTexCoord2f(texX, texY);							glVertex2f( _x+xDiff,				_y+yDiff + _size );
						}
					}
				}
			}
			else
			{
				glTexCoord2f(texX, texY + textHeight);		        glVertex2f( _x,				_y );
				glTexCoord2f(texX + TEX_WIDTH, texY + textHeight);	glVertex2f( _x + (thisChar < 0x3000 ? defaultHorizSize : horiSize),	_y );
				glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex2f( _x + (thisChar < 0x3000 ? defaultHorizSize : horiSize),	_y + _size );
				glTexCoord2f(texX, texY);							glVertex2f( _x,				_y + _size );
			}
		}

		_x += (thisChar < 0x3000 ? defaultHorizSize : horiSize)  * m_horizSpacingFactor;    
    }

	glEnd();

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

	UnicodeString u(buf);

	if (_xJustification > 0)
		DrawText2DSimple( _x, _y, _size, u );			// Left Justification
	else {
		float width = GetTextWidthReal( u, _size );
		
		if (_xJustification < 0)
			DrawText2DSimple( _x - width, _y, _size, u );	// Right Justification
		else
			DrawText2DSimple( _x - width/2, _y, _size, u );	// Centre
	}
}

void TextRenderer::DrawText2DJustified( float _x, float _y, float _size, int _xJustification, const UnicodeString &_text )
{
	if (_xJustification > 0)
		DrawText2DSimple( _x, _y, _size, _text );			// Left Justification
	else {
		float width = GetTextWidthReal( _text, _size );
		
		if (_xJustification < 0)
			DrawText2DSimple( _x - width, _y, _size, _text );	// Right Justification
		else
			DrawText2DSimple( _x - width/2, _y, _size, _text );	// Centre
	}
}

void TextRenderer::DrawText2D( float _x, float _y, float _size, char const *_text, ... )
{
	char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);
	UnicodeString u(buf);
    DrawText2DSimple( _x, _y, _size, u );
}

void TextRenderer::DrawText2D( float _x, float _y, float _size, const UnicodeString &_text )
{
    DrawText2DSimple( _x, _y, _size, _text );
}

void TextRenderer::DrawText2DRight( float _x, float _y, float _size, char const *_text, ... )
{    
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);
	UnicodeString u(buf);

    float width = GetTextWidthReal( u, _size );

    DrawText2DSimple( _x - width, _y, _size, u );
}

void TextRenderer::DrawText2DRight( float _x, float _y, float _size, const UnicodeString &_text )
{
	float width = GetTextWidthReal( _text, _size);
	
	DrawText2DSimple(_x - width, _y, _size, _text);
}

void TextRenderer::DrawText2DCentre( float _x, float _y, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);
	UnicodeString u(buf);

    float width = GetTextWidthReal( u, _size );

    DrawText2DSimple( _x - width/2, _y, _size, u );
}

void TextRenderer::DrawText2DCentre( float _x, float _y, float _size, const UnicodeString &_text )
{
	float width = GetTextWidthReal( _text, _size );

    DrawText2DSimple( _x - width/2, _y, _size, _text );
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
	UnicodeString u(_text);
	DrawText3DSimple(_pos,_size,u);
}

void TextRenderer::DrawText3DSimple( Vector3 const &_pos, float _size, const UnicodeString &_text )
{  
	// Store the GL state
	SaveGLFontDrawAttributes saveAttribs;		

	Camera *cam = g_app->m_camera;
    Vector3 pos(_pos);
	Vector3 vertSize = cam->GetUp() * _size;
	Vector3 horiSize = -cam->GetRight() * _size * m_horizScaleFactor;
	Vector3 defaultHorizSize = -cam->GetRight() * _size * m_horizScaleFactorDefault;
	unsigned int numChars =_text.WcsLen();
	pos += vertSize * 0.5f;

	float textHeight = ((1.0f / 14.0f) * TEX_STRETCH);

	if (m_isunicode)
	{
		textHeight = m_unicodebmpfiles[m_currentbmpnum].textHeight;
	}


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
    
	if( m_renderShadow ||
        m_renderOutline )    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );    
    else                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );    

	glBegin( GL_QUADS );
	wchar_t lastChar = L'\0';

	for( unsigned int i = 0; i < numChars; ++i )
    {       
		WCHAR thisChar = _text.m_unicodestring[i];

		if( thisChar > 0xD800 && thisChar < 0xE000 )
		{
			if( lastChar != L'\0' )
			{
				thisChar = DecodeSurrogatePair(lastChar, thisChar);
				lastChar = L'\0';
			}
			else
			{
				lastChar = thisChar;
				continue;
			}
		}
		
		if (thisChar > 32)
		{
			if (m_isunicode)
			{
				int bmpID = GetTextureID(thisChar);
				if (bmpID != m_currentbmpnum && bmpID >= 0)
				{
					m_currentbmpnum = bmpID;
					m_textureID = m_unicodebmpfiles[bmpID].textureID;
					textHeight = m_unicodebmpfiles[m_currentbmpnum].textHeight;

					glEnd();
					glBindTexture( GL_TEXTURE_2D, m_textureID );
					glBegin( GL_QUADS );
				}
				else if (bmpID == -1)
				{
					// Didn't find the correct texture...
				}
			}

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
                            Vector3 thisPos = pos
                                              + cam->GetUp() * y * _size * 0.05f 
                                              + cam->GetRight() * x * _size * 0.05f;                            
                            glTexCoord2f(texX, texY + textHeight);				glVertex3dv( (thisPos).GetData() );
                            glTexCoord2f(texX + TEX_WIDTH, texY + textHeight);	glVertex3dv( (thisPos + (thisChar < 0x3000 ? defaultHorizSize : horiSize)).GetData() );
                            glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex3dv( (thisPos + (thisChar < 0x3000 ? defaultHorizSize : horiSize) - vertSize).GetData() );
                            glTexCoord2f(texX, texY);							glVertex3dv( (thisPos - vertSize).GetData() );
                        }
                    }
                }
            }
            else
            {
			    glTexCoord2f(texX, texY + textHeight);				glVertex3dv( (pos).GetData() );
			    glTexCoord2f(texX + TEX_WIDTH, texY + textHeight);	glVertex3dv( (pos + (thisChar < 0x3000 ? defaultHorizSize : horiSize)).GetData() );
			    glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex3dv( (pos + (thisChar < 0x3000 ? defaultHorizSize : horiSize) - vertSize).GetData() );
			    glTexCoord2f(texX, texY);							glVertex3dv( (pos - vertSize).GetData() );
            }
		}

		pos += (thisChar < 0x3000 ? defaultHorizSize : horiSize) ;
    }

	glEnd();
}

void TextRenderer::DrawText3D( Vector3 const &_pos, float _size, char const *_text, ... )
{
	char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	UnicodeString u(buf);
	DrawText3DSimple(_pos, _size, u);
}

void TextRenderer::DrawText3D( Vector3 const &_pos, float _size, const UnicodeString &_text )
{  
	DrawText3DSimple(_pos, _size, _text);
}

void TextRenderer::DrawText3DCentre( Vector3 const &_pos, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	Vector3 pos = _pos;
	int defaultCount = 0;
	int bufLen = strlen(buf);
	for( int i = 0; i < bufLen; i++ )
	{
		if( buf[i] < 0x3000 )
		{
			defaultCount++;
		}
	}
	float amount =  (m_horizScaleFactor * (bufLen-defaultCount)) * (m_horizScaleFactorDefault * defaultCount) * 0.5f * _size;
	pos += g_app->m_camera->GetRight() * amount;

	UnicodeString u(buf);

	DrawText3DSimple(pos, _size, u);
}

void TextRenderer::DrawText3DCentre( Vector3 const &_pos, float _size, const UnicodeString &_text )
{
	WCHAR buf[512];
    swprintf(buf, sizeof(buf)/sizeof(WCHAR), _text.m_unicodestring);

	Vector3 pos = _pos;
	int defaultCount = 0;
	int bufLen = wcslen(buf);
	for( int i = 0; i < bufLen; i++ )
	{
		if( buf[i] < 0x3000 )
		{
			defaultCount++;
		}
	}
	float amount =  (m_horizScaleFactor * (bufLen-defaultCount)) * (m_horizScaleFactorDefault * defaultCount) * 0.5f * _size;
	pos += g_app->m_camera->GetRight() * amount;

	UnicodeString u(buf);
	DrawText3DSimple(pos, _size, u);
}


void TextRenderer::DrawText3DRight( Vector3 const &_pos, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	Vector3 pos = _pos;
	int defaultCount = 0;
	int bufLen = strlen(buf);
	for( int i = 0; i < bufLen; i++ )
	{
		if( buf[i] < 0x3000 )
		{
			defaultCount++;
		}
	}
	float amount =  (m_horizScaleFactor * (bufLen-defaultCount)) * (m_horizScaleFactorDefault * defaultCount) * _size;
	pos += g_app->m_camera->GetRight() * amount;

	UnicodeString u(buf);

	DrawText3DSimple(pos, _size, u);
}

void TextRenderer::DrawText3DRight ( Vector3 const &_pos, float _size, const UnicodeString &_text )
{
	WCHAR buf[512];
    swprintf(buf, sizeof(buf)/sizeof(WCHAR), _text.m_unicodestring);

	Vector3 pos = _pos;
	int defaultCount = 0;
	int bufLen = wcslen(buf);
	for( int i = 0; i < bufLen; i++ )
	{
		if( buf[i] < 0x3000 )
		{
			defaultCount++;
		}
	}
	float amount =  (m_horizScaleFactor * (bufLen-defaultCount)) * (m_horizScaleFactorDefault * defaultCount) * _size;
	pos += g_app->m_camera->GetRight() * amount;

	UnicodeString u(buf);
	DrawText3DSimple(pos, _size, u);
}

void TextRenderer::DrawText3D( Vector3 const &_pos, Vector3 const &_front, Vector3 const &_up,
                           float _size, char const *_text, ... )
{
	char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

	UnicodeString u(buf);
	DrawText3D(_pos,_front,_up,_size,u);
}

void TextRenderer::DrawText3D( Vector3 const &_pos, Vector3 const &_front, Vector3 const &_up,
                               float _size, const UnicodeString &_text )
{
	SaveGLFontDrawAttributes saveAttribs;	

	Camera *cam = g_app->m_camera;

    Vector3 pos = _pos;
	Vector3 vertSize = _up * _size;
	Vector3 horiSize = ( _up ^ _front ) * _size * m_horizScaleFactor;
	Vector3 defaultHorizSize = ( _up ^ _front ) * _size * m_horizScaleFactorDefault;
	unsigned int numChars = _text.WcsLen();
	pos -= horiSize * numChars * 0.5f;
    pos += vertSize * 0.5f;

	float textHeight = ((1.0f / 14.0f) * TEX_STRETCH);

	if (m_isunicode)
	{
		textHeight = m_unicodebmpfiles[m_currentbmpnum].textHeight;
	}


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

	glBegin( GL_QUADS );

	wchar_t lastChar = L'\0';

	for( unsigned int i = 0; i < numChars; ++i )
    {       
		WCHAR thisChar = _text.m_unicodestring[i];

		if( thisChar > 0xD800 && thisChar < 0xE000 )
		{
			if( lastChar != L'\0' )
			{
				thisChar = DecodeSurrogatePair(lastChar, thisChar);
				lastChar = L'\0';
			}
			else
			{
				lastChar = thisChar;
				continue;
			}
		}

		if (thisChar > 32)
		{
			if (m_isunicode)
			{
				int bmpID = GetTextureID(thisChar);
				if (bmpID != m_currentbmpnum && bmpID >= 0)
				{
					m_currentbmpnum = bmpID;
					m_textureID = m_unicodebmpfiles[bmpID].textureID;
					textHeight = m_unicodebmpfiles[m_currentbmpnum].textHeight;

					glEnd();
					glBindTexture( GL_TEXTURE_2D, m_textureID );
					glBegin( GL_QUADS );
				}
				else if (bmpID == -1)
				{
					// Didn't find the correct texture...
				}
			}

			float texX = GetTexCoordX( thisChar );
			float texY = GetTexCoordY( thisChar );

			glTexCoord2f(texX, texY + textHeight);				glVertex3dv( (pos).GetData() );
			glTexCoord2f(texX + TEX_WIDTH, texY + textHeight);	glVertex3dv( (pos + (thisChar < 0x3000 ? defaultHorizSize : horiSize) ).GetData() );
			glTexCoord2f(texX + TEX_WIDTH, texY);				glVertex3dv( (pos + (thisChar < 0x3000 ? defaultHorizSize : horiSize)  - vertSize).GetData() );
			glTexCoord2f(texX, texY);							glVertex3dv( (pos - vertSize).GetData() );
		}

		pos += (thisChar < 0x3000 ? defaultHorizSize : horiSize) ;
    }

	glEnd();
}


float TextRenderer::GetTextWidth( unsigned int _numChars, float _size )
{
    return _numChars * _size * m_horizScaleFactor;
}


float TextRenderer::GetTextWidthReal( const UnicodeString &_text, float _size )
{
	float _x = 0.0f;

	float horiSize = _size * m_horizScaleFactor;
	float defaultHorizSize = _size * m_horizScaleFactorDefault;

	unsigned numChars = _text.WcsLen();
	wchar_t lastChar = L'\0';
	for( unsigned int i = 0; i < numChars; ++i )
	{       
		WCHAR thisChar = _text.m_unicodestring[i];

		if( thisChar > 0xD800 && thisChar < 0xE000 )
		{
			if( lastChar != L'\0' )
			{
				thisChar = DecodeSurrogatePair(lastChar, thisChar);
				lastChar = L'\0';
			}
			else
			{
				lastChar = thisChar;
				continue;
			}
		}

		_x += (thisChar < 0x3000 ? defaultHorizSize : horiSize)  * m_horizSpacingFactor;    
	}

	return _x;
}


bool TextRenderer::IsUnicode()
{
	return m_isunicode;
}


static int DecodeSurrogatePair( WCHAR _word1, WCHAR _word2 )
{
	int ret = -1;
	// Check to see if this is a well formed surrogate pair
	if (_word1 >= 0xD800 && _word1 < 0xDC00 &&
		_word2 >= 0xDC00 && _word2 < 0xE000)
	{
		_word1 ^= 0xD800;
		_word2 ^= 0xDC00;

		ret = ((int)_word1 << 10) | _word2;
		ret |= 0x10000;
	}

	return ret;
}
