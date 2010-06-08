#define _CRT_SECURE_NO_DEPRECATE
#include "lib/universal_include.h"

#include <string.h>
#include <math.h>

#include "lib/filesys/binary_stream_readers.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/filesys/file_system.h"
#include "lib/render/colour.h"
#include "lib/debug_utils.h"

#include "bitmap.h"


#define BMP_RGB				0
#define OS2INFOHEADERSIZE  12
#define WININFOHEADERSIZE  40


class BitmapFileHeader
{
public:
   unsigned short bfType;
   unsigned long  bfSize;
   unsigned short bfReserved1;
   unsigned short bfReserved2;
   unsigned long  bfOffBits;
};


// Used for both OS/2 and Windows BMP. 
// Contains only the parameters needed to load the image 
class BitmapInfoHeader
{
public:
   unsigned long  biWidth;
   unsigned long  biHeight;
   unsigned short biBitCount;
   unsigned long  biCompression;
};


class WinBmpInfoHeader  /* size: 40 */
{
public:
   unsigned long  biWidth;
   unsigned long  biHeight;
   unsigned short biPlanes;
   unsigned short biBitCount;
   unsigned long  biCompression;
   unsigned long  biSizeImage;
   unsigned long  biXPelsPerMeter;
   unsigned long  biYPelsPerMeter;
   unsigned long  biClrUsed;
   unsigned long  biClrImportant;
};


class OS2BmpInfoHeader  /* size: 12 */
{
public:
   unsigned short biWidth;
   unsigned short biHeight;
   unsigned short biPlanes;
   unsigned short biBitCount;
};



// ****************************************************************************
// Private Functions
// ****************************************************************************

// Reads a BMP file header and check that it has the BMP magic number.
void Bitmap::ReadBMPFileHeader(BinaryReader *f, BitmapFileHeader *fileheader)
{
	fileheader->bfType = f->ReadS16();
	fileheader->bfSize= f->ReadS32();
	fileheader->bfReserved1= f->ReadS16();
	fileheader->bfReserved2= f->ReadS16();
	fileheader->bfOffBits= f->ReadS32();

	AppAssert(fileheader->bfType == 19778);
}


// Reads information from a BMP file header.
void Bitmap::ReadWinBMPInfoHeader(BinaryReader *f, BitmapInfoHeader *infoheader)
{
	WinBmpInfoHeader win_infoheader;

	win_infoheader.biWidth = f->ReadS32();
	win_infoheader.biHeight = f->ReadS32();
	win_infoheader.biPlanes = f->ReadS16();
	win_infoheader.biBitCount = f->ReadS16();
	win_infoheader.biCompression = f->ReadS32();
	win_infoheader.biSizeImage = f->ReadS32();
	win_infoheader.biXPelsPerMeter = f->ReadS32();
	win_infoheader.biYPelsPerMeter = f->ReadS32();
	win_infoheader.biClrUsed = f->ReadS32();
	win_infoheader.biClrImportant = f->ReadS32();

	infoheader->biWidth = win_infoheader.biWidth;
	infoheader->biHeight = win_infoheader.biHeight;
	infoheader->biBitCount = win_infoheader.biBitCount;
	infoheader->biCompression = win_infoheader.biCompression;
}


// Reads information from an OS/2 format BMP file header.
void Bitmap::ReadOS2BMPInfoHeader(BinaryReader *f, BitmapInfoHeader *infoheader)
{
	OS2BmpInfoHeader os2_infoheader;

	os2_infoheader.biWidth = f->ReadS16();
	os2_infoheader.biHeight = f->ReadS16();
	os2_infoheader.biPlanes = f->ReadS16();
	os2_infoheader.biBitCount = f->ReadS16();

	infoheader->biWidth = os2_infoheader.biWidth;
	infoheader->biHeight = os2_infoheader.biHeight;
	infoheader->biBitCount = os2_infoheader.biBitCount;
	infoheader->biCompression = 0;
}


void Bitmap::ReadBMPPalette(int ncols, Colour pal[256], BinaryReader *f, int win_flag)
{
	for (int i = 0; i < ncols; i++) 
	{
	    pal[i].m_b = f->ReadU8();
	    pal[i].m_g = f->ReadU8();
	    pal[i].m_r = f->ReadU8();
		pal[i].m_a = 255;
	    if (win_flag) f->ReadU8();
	}
}


// Support function for reading the 4 bit bitmap file format.
void Bitmap::Read4BitLine(int length, BinaryReader *f, Colour pal[256], int y)
{
	for (int x = 0; x < length; x += 2) 
	{
		unsigned char i = f->ReadU8();
		unsigned idx1 = i & 15;
		unsigned idx2 = (i >> 4) & 15;
		PutPixel(x+1, y, pal[idx1]);
		PutPixel(x, y, pal[idx2]);
	}
}


// Support function for reading the 8 bit bitmap file format.
void Bitmap::Read8BitLine(int length, BinaryReader *f, Colour pal[256], int y)
{
//	y = m_height - line - 1;
	for (int x = 0; x < length; ++x) 
	{
		unsigned char i = f->ReadU8();
		PutPixel(x, y, pal[i]);
	}
}


// Support function for reading the 24 bit bitmap file format
void Bitmap::Read24BitLine(int length, BinaryReader *f, int y)
{
	Colour c;
	int nbytes=0;
//	y = m_height - y - 1;
	c.m_a = 255;

	for (int i=0; i<length; i++) 
	{
		c.m_b = f->ReadU8();
		c.m_g = f->ReadU8();
		c.m_r = f->ReadU8();
		PutPixel(i, y, c);
		nbytes += 3;
	}

	for (int padding = (4 - nbytes) & 3; padding; --padding) 
	{
		f->ReadU8();
	}
}


void Bitmap::LoadBmp(BinaryReader *_in)
{
	BitmapFileHeader fileheader;
	BitmapInfoHeader infoheader;
	Colour palette[256];

	ReadBMPFileHeader(_in, &fileheader);

	unsigned long biSize = _in->ReadS32();

	if (biSize == WININFOHEADERSIZE) 
	{
		ReadWinBMPInfoHeader(_in, &infoheader);
		
		int ncol = (fileheader.bfOffBits - 54) / 4; // compute number of colors recorded
		ReadBMPPalette(ncol, palette, _in, 1);
	}
	else if (biSize == OS2INFOHEADERSIZE) 
	{
	    ReadOS2BMPInfoHeader(_in, &infoheader);
    
	    int ncol = (fileheader.bfOffBits - 26) / 3; // compute number of colors recorded
	    ReadBMPPalette(ncol, palette, _in, 0);
	}
	else 
	{
	    AppAbort( "Error reading bitmap" );
	}

	Initialise(infoheader.biWidth, infoheader.biHeight);
	AppAssert(infoheader.biCompression == BMP_RGB); 
	AppAssert(!_in->m_eof);

	// Read the image
	for (int i = 0; i < (int)infoheader.biHeight; ++i) 
	{
		switch (infoheader.biBitCount)
		{
		case 4:
			Read4BitLine(infoheader.biWidth, _in, palette, i);
			break;
		case 8:
			Read8BitLine(infoheader.biWidth, _in, palette, i);
			break;
		case 24:
			Read24BitLine(infoheader.biWidth, _in, i);
			break;
		default:
			AppAbort("Error reading bitmap");
			break;
		}
	}
}

// Little endian output functions
static void writeShort(unsigned short _x, FILE *_out)
{
	fputc( _x & 0xFF, _out);
	_x >>= 8;
	fputc( _x & 0xFF, _out);
}

static void writeInt(unsigned _x, FILE *_out)
{
	fputc( _x & 0xFF, _out);
	_x >>= 8;
	fputc( _x & 0xFF, _out);
	_x >>= 8;
	fputc( _x & 0xFF, _out);
	_x >>= 8;
	fputc( _x & 0xFF, _out);
}

void Bitmap::WriteBMPFileHeader(FILE *_out)
{
	BitmapFileHeader fileheader;
	fileheader.bfType = 19778;
	fileheader.bfSize = m_width * m_height * 3 + 54;
	fileheader.bfReserved1 = 0;
	fileheader.bfReserved2 = 0;
	fileheader.bfOffBits = 54;

	writeShort(fileheader.bfType, _out);
	writeInt(fileheader.bfSize, _out);
	writeShort(fileheader.bfReserved1, _out);
	writeShort(fileheader.bfReserved2, _out);
	writeInt(fileheader.bfOffBits, _out);
}


void Bitmap::WriteWinBMPInfoHeader(FILE *_out)
{
	WinBmpInfoHeader win_infoheader;

	win_infoheader.biWidth = m_width;
	win_infoheader.biHeight = m_height;
	win_infoheader.biPlanes = 1;
	win_infoheader.biBitCount = 24;
	win_infoheader.biCompression = 0;
	win_infoheader.biSizeImage = m_width * m_height * 3;
	win_infoheader.biXPelsPerMeter = 2835;
	win_infoheader.biYPelsPerMeter = 2835;
	win_infoheader.biClrUsed = 0;
	win_infoheader.biClrImportant = 0;

	writeInt(win_infoheader.biWidth, _out);
	writeInt(win_infoheader.biHeight, _out);
	writeShort(win_infoheader.biPlanes, _out);
	writeShort(win_infoheader.biBitCount, _out);
	writeInt(win_infoheader.biCompression, _out);
	writeInt(win_infoheader.biSizeImage, _out);
	writeInt(win_infoheader.biXPelsPerMeter, _out);
	writeInt(win_infoheader.biYPelsPerMeter, _out);
	writeInt(win_infoheader.biClrUsed, _out);
	writeInt(win_infoheader.biClrImportant, _out);
}


void Bitmap::Write24BitLine(FILE *_out, int _y)
{
	int nbytes=0;

	for (int x = 0; x < m_width; ++x) 
	{
		Colour const &c = GetPixel(x, _y);
		fputc(c.m_b, _out);
		fputc(c.m_g, _out);
		fputc(c.m_r, _out);
		nbytes += 3;
	}

	for (int padding = (4 - nbytes) & 3; padding; --padding) 
	{
		fputc(0, _out);
	}
}


void Bitmap::SaveBmp(char *_filename)
{
	FILE *_out = fopen(_filename, "wb");
	AppAssert( _out );

	WriteBMPFileHeader(_out);

	unsigned long nextHeaderSize = 40;
	writeInt(nextHeaderSize, _out);

	WriteWinBMPInfoHeader(_out);

	for (int y = 0; y < m_height; ++y)
	{
		Write24BitLine(_out, y);
	}

	fclose(_out);
}


// ****************************************************************************
// Public Functions
// ****************************************************************************


Bitmap::Bitmap()
:	m_width(-1),
	m_height(-1),
	m_pixels(NULL),
	m_lines(NULL)
{
}


Bitmap::Bitmap(Bitmap &_other)
{
	Initialise(_other.m_width, _other.m_height);
	memcpy(m_pixels, _other.m_pixels, m_width * m_height * sizeof(Colour));
}


Bitmap::Bitmap(int width, int height)
:	m_width(-1),
	m_height(-1),
	m_pixels(NULL),
	m_lines(NULL)
{
	Initialise(width, height);
}


// *** Constructor: Load from a BinaryReader
Bitmap::Bitmap(BinaryReader *_reader, char *_type)
:	m_width(-1),
	m_height(-1),
	m_pixels(NULL),
	m_lines(NULL)
{
	Initialise(_reader, _type);
}


// *** Destructor
Bitmap::~Bitmap()
{
	m_width = -1;
	m_height = -1;
	delete [] m_pixels;
	delete [] m_lines;
	m_pixels = NULL;
	m_lines = NULL;
}


// *** Initialise
void Bitmap::Initialise(int width, int height)
{
	m_width = width;
	m_height = height;
	m_pixels = new Colour[width * height];
	m_lines = new Colour *[height];

	AppAssert(m_pixels && m_lines);

	for (int y = 0; y < height; ++y)
	{
		m_lines[y] = &m_pixels[y * width];
	}
}


void Bitmap::Initialise(BinaryReader *_reader, char *_type)
{
	AppAssert(stricmp(_type, "bmp") == 0);
	LoadBmp(_reader);
}


// *** PutPixel
void Bitmap::PutPixel(int x, int y, Colour const &colour)
{
	m_lines[y][x] = colour;
}


// *** GetPixel
Colour &Bitmap::GetPixel(int x, int y) 
{
	return m_lines[y][x];
}


// *** PutPixelClipped
void Bitmap::PutPixelClipped(int x, int y, Colour const &colour)
{
	if (x < 0 || x >= m_width ||
		y < 0 || y >= m_height)
	{
		return;
	}

	m_lines[y][x] = colour;
}


// *** GetPixelClipped
Colour &Bitmap::GetPixelClipped(int x, int y)
{
    static Colour s_black(0,0,0);

	if (x < 0 || x >= m_width ||
		y < 0 || y >= m_height)
	{
		return s_black;
	}

	return m_lines[y][x]; 
}


void Bitmap::DrawLine(int x1, int y1, int x2, int y2, Colour const &colour)
{
    int numSteps = sqrtf( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
    float dx = float(x2-x1) / (float) numSteps;
    float dy = float(y2-y1) / (float) numSteps;

    float currentX = x1;
    float currentY = y1;

    for( int i = 0; i < numSteps; ++i )
    {
        PutPixelClipped( currentX, currentY, colour );

        currentX += dx;
        currentY += dy;
    }
}


// *** GetInterpolatedPixel
Colour Bitmap::GetInterpolatedPixel(float _x, float _y) 
{
	int x1 = floorf(_x);
	int y1 = floorf(_y);
	int x2 = ceilf(_x);
	int y2 = ceilf(_y);

	float fractionalX = _x - x1;
	float fractionalY = _y - y1;

	if (x2 == m_width) 
		x2 = m_width - 1;

	if (y2 == m_height)
		y2 = m_height - 1;
		
	float weight11 = (1.0f - fractionalX) * (1.0f - fractionalY);
	float weight12 = (1.0f - fractionalX) * (fractionalY);
	float weight21 = (fractionalX) * (1.0f - fractionalY);
	float weight22 = (fractionalX) * (fractionalY);

    Colour &c11 = GetPixelClipped(x1, y1); 
    Colour &c12 = GetPixelClipped(x1, y2); 
    Colour &c21 = GetPixelClipped(x2, y1); 
    Colour &c22 = GetPixelClipped(x2, y2); 

	Colour returnVal((float)c11.m_r * weight11 + (float)c12.m_r * weight12 + (float)c21.m_r * weight21 + (float)c22.m_r * weight22,
						 (float)c11.m_g * weight11 + (float)c12.m_g * weight12 + (float)c21.m_g * weight21 + (float)c22.m_g * weight22,
						 (float)c11.m_b * weight11 + (float)c12.m_b * weight12 + (float)c21.m_b * weight21 + (float)c22.m_b * weight22);

	return returnVal;
}


// *** Blit
void Bitmap::Blit(int _srcX, int _srcY, int _srcW, int _srcH, Bitmap *_srcBmp, 
					  int _destX, int _destY, int _destW, int _destH, bool _bilinear)
{
	AppAssert(_srcX + _srcW <= _srcBmp->m_width);
	AppAssert(_srcY + _srcH <= _srcBmp->m_height);
	AppAssert(_destX + _destW <= m_width);
	AppAssert(_destY + _destH <= m_height);

	float sxPitch = (float)_srcW / (float)_destW;
	float syPitch = (float)_srcH / (float)_destH;

	float sy = _srcY;
	for (int dy = _destY; dy < _destY + _destH; ++dy)
	{
		float sx = _srcX;
		if (_bilinear)
		{
			for (int dx = _destX; dx < _destX + _destW; ++dx)
			{
				Colour const &c = _srcBmp->GetInterpolatedPixel(sx, sy);
				PutPixel(dx, dy, c);
				sx += sxPitch;
			}
		}
		else
		{
			for (int dx = _destX; dx < _destX + _destW; ++dx)
			{
				Colour const &c = _srcBmp->GetPixelClipped(sx, sy);
				PutPixel(dx, dy, c);
				sx += sxPitch;
			}
		}
		sy += syPitch;
	}
}


void Bitmap::ApplyDilateFilter()
{
    static Colour *temp = new Colour[ m_width * m_height ];
	memcpy( temp, m_pixels, sizeof(Colour) * m_width * m_height );

    for( int x = 1; x < m_width-1; ++x )
    {
        for( int y = 1; y < m_height-1; ++y )
        {
            float adjacentRed = 0.0f;
            float adjacentGreen = 0.0f;
            float adjacentBlue = 0.0f;
            for( int i = -1; i <= 1; ++i )
            {
                for( int j = -1; j <= 1; ++j )
                {
                    if( i != 0 || j != 0 )
                    {
                        Colour col = temp[ (y+j) * m_width + (x+i) ];
                        adjacentRed += col.m_r;
                        adjacentGreen += col.m_g;
                        adjacentBlue += col.m_b;
                    }
                }
            }
            adjacentRed /= 8.0f;
            adjacentGreen /= 8.0f;
            adjacentBlue /= 8.0f;

            m_pixels[ y * m_width + x ].Set( adjacentRed, adjacentGreen, adjacentBlue );
        }
    }
}


void Bitmap::ApplyBlurFilter(float _scale)
{
	AppAssert(m_width > 0 && m_width <= 1024);
	AppAssert(m_height > 0 && m_height <= 1024);

    static Colour *temp = new Colour[ m_width * m_height ];
	memset(temp, 0, sizeof(Colour) * m_width * m_height);


	int const blurSize = 5;
	int halfBlurSize = 2;
	float m[blurSize] = { 2, 4, 7, 4, 2 };
	for (int i = 0; i < blurSize; ++i)
	{
		m[i] *= _scale * 0.0526f;
	}


	//
	// Horizontal blur
	
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			Colour &src = m_lines[y][x];
			if (src.m_r == 0 && src.m_g == 0 && src.m_b == 0) continue;

			for (int i = 0; i < blurSize; ++i)
			{
				int a = x + i - halfBlurSize;
				if (a < 0 || a >= m_width) continue;

				Colour &dest = temp[y * m_width + a];
				int r = dest.m_r + int((float)src.m_r * m[i]);
				int g = dest.m_g + int((float)src.m_g * m[i]);
				int b = dest.m_b + int((float)src.m_b * m[i]);

				if (r > 255) r = 255;
				if (g > 255) g = 255;
				if (b > 255) b = 255;

				temp[y * m_width + a].Set(r, g, b);
			}
		}
	}


	for (int i = 0; i < blurSize; ++i)
	{
		m[i] *= 2.0f;
	}


	//
	// Vertical blur
	
	Clear(Colour(0,0,0,0));
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			Colour &src = temp[y * m_width + x];
			if (src.m_r == 0 && src.m_g == 0 && src.m_b == 0) continue;

			for (int i = 0; i < blurSize; ++i)
			{
				int a = y + i - halfBlurSize;
				if (a < 0 || a >= m_height) continue;

				Colour &dest = m_lines[a][x];
				int r = dest.m_r + int((float)src.m_r * m[i]);
				int g = dest.m_g + int((float)src.m_g * m[i]);
				int b = dest.m_b + int((float)src.m_b * m[i]);

				if (r > 255) r = 255;
				if (g > 255) g = 255;
				if (b > 255) b = 255;

				m_lines[a][x].Set(r, g, b);
			}
		}
	}
}


void Bitmap::ConvertColourToAlpha()
{
	Colour col;
    Colour newCol( 255, 255, 255, 0 );

    for( int y = 0; y < m_height; ++y )
    {
	    for( int x = 0; x < m_width; ++x )
        {
            col = GetPixel( x, y );
            newCol.m_a = col.m_g;
            PutPixel( x, y, newCol );
        }
    }
}

		
void Bitmap::ConvertPinkToTransparent() 
{
	Colour pink(255, 0, 255);
	Colour trans(128,128,128,0);

	for (int y = 0; y < m_height; ++y) 
	{
		for (int x = 0; x < m_width; ++x) 
		{
			Colour c(GetPixel(x, y));
			if (c == pink)
			{
				PutPixel(x, y, trans);
			}
		}
	}
}


void Bitmap::Clear( Colour const &colour )
{
	for (int x = 0; x < m_width; ++x) 
	{
        PutPixel( x, 0, colour );
    }

	int size = sizeof(Colour) * m_width;
 	for (int y = 0; y < m_height; ++y) 
	{
		memcpy(m_lines[y], m_lines[0], size);
    }
}


int Bitmap::ConvertToTexture(bool _mipmapping) 
{
	GLuint texId;

    glEnable        (GL_TEXTURE_2D);
	glGenTextures	(1, &texId);
	glBindTexture	(GL_TEXTURE_2D, texId);
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	
    if (_mipmapping)
	{
		int result = gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, 
					                   m_width, m_height, 
						               GL_RGBA, GL_UNSIGNED_BYTE, 
						               m_pixels);
        
        AppAssert( result == 0 );
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, 4, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels);
	}

    glDisable( GL_TEXTURE_2D );

	return (int) texId;
}

