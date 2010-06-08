#include "lib/universal_include.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include "lib/filesys/binary_stream_readers.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/filesys/file_system.h"
#include "lib/render/colour.h"
#include "lib/debug_utils.h"
#include "lib/string_utils.h"

#include "bitmap.h"
#include "bitmapfont.h"


#define TEX_WIDTH			1.0f / 16.0f
#define TEX_HEIGHT			1.0f / 16.0f

// ****************************************************************************
//  Class BitmapFont
// ****************************************************************************

BitmapFont::BitmapFont(const char *_filename, BinaryReader *_reader)
{
	m_filename = newStr(_filename);
    m_horizontalFlip = false;
    m_spacing = 0.05f;
    m_fixedWidth = false;
	bool deleteReader = false;

	if ( !_reader ) {
		_reader = g_fileSystem->GetBinaryReader(m_filename);
		deleteReader = true;
	}
    Bitmap bmp(_reader, (char *)GetExtensionPart(m_filename) );
    m_textureID = bmp.ConvertToTexture();
	if ( deleteReader )
	    delete _reader;


    //
    // Attempt to determine the left and right margins for each letter

    int perCharWidth = bmp.m_width * TEX_WIDTH;
    int perCharHeight = bmp.m_height * TEX_HEIGHT;
    
    for( int c = 0; c < 256; ++c )
    {
        m_leftMargin[c] = 1.0f;
        m_rightMargin[c] = 0.0f;
        
        int left    = (c % 16) * perCharWidth;
        int right   = left + perCharWidth;
        int top     = (c / 16) * perCharHeight;
        int bottom  = top + perCharHeight;
        
        for( int x = left; x < right; ++x )
        {
            for( int y = top; y < bottom; ++y )
            {
                Colour pixelCol = bmp.GetPixel(x,bmp.m_height-y-1);

                if( pixelCol.m_r > 64 &&
                    pixelCol.m_g > 64 &&
                    pixelCol.m_b > 64 )
                {
                    float thisVal = TEX_WIDTH * float(x-left) / float(right-left);                    
                    if( thisVal < m_leftMargin[c] ) m_leftMargin[c] = thisVal;
                    if( thisVal > m_rightMargin[c] ) m_rightMargin[c] = thisVal;
                }
            }
        }

        m_leftMargin[c] -= 1.0f / (float)bmp.m_width;
        m_rightMargin[c] += 1.0f / (float)bmp.m_height;

        m_leftMargin[c] = std::min(m_leftMargin[c], TEX_WIDTH * 0.45f );
        m_rightMargin[c] = std::max(m_rightMargin[c], TEX_WIDTH * 0.55f );

    }

}


float BitmapFont::GetTexCoordX( unsigned char theChar )
{
    float texX = (theChar % 16) * TEX_WIDTH;   
    
    if( !m_fixedWidth )
    {
        texX += m_leftMargin[theChar];
    }

    return texX;
}


float BitmapFont::GetTexCoordY( unsigned char theChar )
{
    float texY = (theChar / 16) * TEX_HEIGHT; 
    texY = 1.0f - texY - TEX_HEIGHT;
    texY -= 0.002f;                                     // Offset to prevent minor overlap
    return texY;
}


float BitmapFont::GetTexWidth( unsigned char theChar )
{
    float result = TEX_WIDTH;

    if( !m_fixedWidth )
    {
        result = m_rightMargin[theChar] - m_leftMargin[theChar];
    }
    
    return result;
}


void BitmapFont::SetHoriztonalFlip( bool _flip )
{
    m_horizontalFlip = _flip;
}


void BitmapFont::SetFixedWidth( bool _fixedWidth )
{
    m_fixedWidth = _fixedWidth;
}


void BitmapFont::SetSpacing( float _spacing )
{
    m_spacing = _spacing;
}


void BitmapFont::DrawText2DSimple(float _x, float _y, float _size, char const *_text )
{
    bool blending = glIsEnabled( GL_BLEND );

    glEnable        ( GL_BLEND );
    glEnable        ( GL_TEXTURE_2D );    
    glBindTexture   ( GL_TEXTURE_2D, m_textureID );    
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    
	size_t numChars = strlen(_text);
    for( size_t i = 0; i < numChars; ++i )
    {       
        unsigned char thisChar = _text[i];

		float texX = GetTexCoordX( thisChar );
		float texY = GetTexCoordY( thisChar );
        float texW = GetTexWidth( thisChar );
        float texH = TEX_HEIGHT;

        float horiSize = _size * texW/texH;

        if( m_horizontalFlip )
        {
            texY += texH;
            texH *= -1.0f;
        }

		glBegin( GL_QUADS );
			glTexCoord2f(texX, texY + texH);		    glVertex2f( _x,				_y );
			glTexCoord2f(texX + texW, texY + texH);	    glVertex2f( _x + horiSize,	_y );
			glTexCoord2f(texX + texW, texY);			glVertex2f( _x + horiSize,	_y + _size );
			glTexCoord2f(texX, texY);					glVertex2f( _x,				_y + _size );
		glEnd();

    	_x += horiSize;                
        _x += _size * m_spacing;

        if( m_fixedWidth )
        {
            _x -= _size*0.55f;
        }
    }

    glDisable       ( GL_TEXTURE_2D );
}


void BitmapFont::DrawText2D( float _x, float _y, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);
    DrawText2DSimple( _x, _y, _size, buf );
}


void BitmapFont::DrawText2DRight( float _x, float _y, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

    float width = GetTextWidth( buf, _size );
    DrawText2DSimple( _x - width, _y, _size, buf );
}


void BitmapFont::DrawText2DCentre( float _x, float _y, float _size, char const *_text, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

    float width = GetTextWidth( buf, fabs(_size) );
    DrawText2DSimple( _x - width/2, _y, _size, buf );
}


float BitmapFont::GetTextWidth( char const *_string, float _size )
{
    float result = 0.0f;

    size_t stringlen = strlen(_string);

    for( size_t i = 0; i < stringlen; ++i )
    {
        unsigned char thisChar = _string[i];
        float texW = GetTexWidth( thisChar );
        float texH = TEX_HEIGHT;
        float horiSize = _size * texW/texH;

        result += _size * m_spacing;
    	result += horiSize;  
        
        if( m_fixedWidth )
        {
            result -= _size*0.55f;
        }
    }

    return result;
}
