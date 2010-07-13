#include "lib/universal_include.h"

#include "lib/filesys/binary_stream_readers.h"
#include "lib/bitmap.h"
#include "lib/debug_utils.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/rgb_colour.h"
#include "lib/preferences.h"
#include "loading_screen.h"

#include "app.h"

#include <png.h>

#define BMP_RGB				0
#define OS2INFOHEADERSIZE  12
#define WININFOHEADERSIZE  40

static int gMaxTextureSize = 0;

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
void BitmapRGBA::ReadBMPFileHeader(BinaryReader *f, BitmapFileHeader *fileheader)
{
	fileheader->bfType = f->ReadS16();
	fileheader->bfSize= f->ReadS32();
	fileheader->bfReserved1= f->ReadS16();
	fileheader->bfReserved2= f->ReadS16();
	fileheader->bfOffBits= f->ReadS32();

	AppDebugAssert(fileheader->bfType == 19778);
}


// Reads information from a BMP file header.
void BitmapRGBA::ReadWinBMPInfoHeader(BinaryReader *f, BitmapInfoHeader *infoheader)
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
void BitmapRGBA::ReadOS2BMPInfoHeader(BinaryReader *f, BitmapInfoHeader *infoheader)
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


void BitmapRGBA::ReadBMPPalette(int ncols, RGBAColour pal[256], BinaryReader *f, int win_flag)
{
	for (int i = 0; i < ncols; i++) 
	{
	    pal[i].b = f->ReadU8();
	    pal[i].g = f->ReadU8();
	    pal[i].r = f->ReadU8();
		pal[i].a = 255;
	    if (win_flag) f->ReadU8();
	}
}


// Support function for reading the 4 bit bitmap file format.
void BitmapRGBA::Read4BitLine(int length, BinaryReader *f, RGBAColour pal[256], int y)
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
void BitmapRGBA::Read8BitLine(int length, BinaryReader *f, RGBAColour pal[256], int y)
{
//	y = m_height - line - 1;
	for (int x = 0; x < length; ++x) 
	{
		unsigned char i = f->ReadU8();
		PutPixel(x, y, pal[i]);
	}
}


// Support function for reading the 24 bit bitmap file format
void BitmapRGBA::Read24BitLine(int length, BinaryReader *f, int y)
{
	RGBAColour c;
	int nbytes=0;
//	y = m_height - y - 1;
	c.a = 255;

	for (int i=0; i<length; i++) 
	{
		c.b = f->ReadU8();
		c.g = f->ReadU8();
		c.r = f->ReadU8();
		PutPixel(i, y, c);
		nbytes += 3;
	}

	for (int padding = (4 - nbytes) & 3; padding; --padding) 
	{
		f->ReadU8();
	}
}


void BitmapRGBA::Read32BitLine(int length, BinaryReader *f, int y)
{
    RGBAColour c;
    int nbytes=0;

    for (int i=0; i<length; i++) 
    {
        c.b = f->ReadU8();
        c.g = f->ReadU8();
        c.r = f->ReadU8();
        c.a = f->ReadU8();
        PutPixel(i, y, c);
        nbytes += 4;
    }
}


void BitmapRGBA::LoadBmp(BinaryReader *_in)
{
	BitmapFileHeader fileheader;
	BitmapInfoHeader infoheader;
	RGBAColour palette[256];

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
	    AppDebugAssert(0);
	}

	Initialise(infoheader.biWidth, infoheader.biHeight);
	AppDebugAssert(infoheader.biCompression == BMP_RGB); 
	AppDebugAssert(!_in->m_eof);

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
        case 32:
            Read32BitLine(infoheader.biWidth, _in, i);
            break;

		default:
			AppDebugAssert(0);
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

void BitmapRGBA::WriteBMPFileHeader(FILE *_out)
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


void BitmapRGBA::WriteWinBMPInfoHeader(FILE *_out)
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


void BitmapRGBA::Write24BitLine(FILE *_out, int _y)
{
	int nbytes=0;

	for (int x = 0; x < m_width; ++x) 
	{
		RGBAColour const &c = GetPixel(x, _y);
		fputc(c.b, _out);
		fputc(c.g, _out);
		fputc(c.r, _out);
		nbytes += 3;
	}

	for (int padding = (4 - nbytes) & 3; padding; --padding) 
	{
		fputc(0, _out);
	}
}


void BitmapRGBA::SaveBmp(char const *_filename)
{
	FILE *_out = fopen(_filename, "wb");
	AppReleaseAssert(_out, "Couldn't create image file %s", _filename);
	WriteBmp( _out );
	fclose(_out);
}


void BitmapRGBA::SavePng(char const *_filename, bool _saveAlpha)
{
	FILE *_out = fopen(_filename, "wb");
	AppReleaseAssert(_out, "Couldn't create image file %s", _filename);
	WritePng( _out );
	fclose(_out);
}

void BitmapRGBA::WritePng(FILE *_out, bool _saveAlpha)
{
	png_structp png_ptr = png_create_write_struct
       (PNG_LIBPNG_VER_STRING, (png_voidp) NULL /*user_error_ptr*/,
        NULL /*user_error_fn*/, NULL /*user_warning_fn*/);
    
	if( !png_ptr )
       return; // failed 

	png_infop info_ptr = png_create_info_struct( png_ptr );
	if( !info_ptr )
	{
		png_destroy_write_struct( &png_ptr, (png_infopp) NULL );
		return; // failed
	}
	
    if( setjmp( png_jmpbuf(png_ptr) ) )
    {
       png_destroy_write_struct(&png_ptr, &info_ptr);
       return; // failed
    }

    png_init_io( png_ptr, _out );
	png_set_compression_level( png_ptr, Z_BEST_SPEED );
    png_set_compression_mem_level( png_ptr, 8 );
    png_set_compression_strategy( png_ptr, Z_DEFAULT_STRATEGY );
    png_set_compression_window_bits( png_ptr, 15 );
    png_set_compression_method( png_ptr, 8 );
    png_set_compression_buffer_size( png_ptr, 8192 );

	png_set_IHDR( png_ptr, info_ptr, m_width, m_height, 8, 
		(_saveAlpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB),
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_bytepp rows = new png_bytep[m_height];
	png_bytep chunk = NULL;
	if (!_saveAlpha) {
		// Strip out the alpha component
		chunk = new png_byte[m_width * m_height * 3];
		png_bytep p = chunk;

		for (int y = 0; y < m_height; y++) {
			png_bytep row = (png_bytep) m_lines[m_height - y - 1];
			rows[y] = p;
			for (int x = 0; x < m_width; x++) {
				*p++ = *row++;
				*p++ = *row++;
				*p++ = *row++;
				row++;
			}
		}
	}
	else {
		for (int y = 0; y < m_height; y++)
			rows[y] = (png_bytep) m_lines[m_height - y - 1];
	}
	png_set_rows( png_ptr, info_ptr, rows );
	png_write_png( png_ptr, info_ptr, 0 /* transforms */, NULL );
	png_write_end( png_ptr, info_ptr );
	png_destroy_write_struct( &png_ptr, &info_ptr );
	delete [] rows;
	delete [] chunk;
}


void BitmapRGBA::WriteBmp(FILE *_out)
{
	AppReleaseAssert(_out, "Couldn't write image file");

	WriteBMPFileHeader(_out);

	unsigned long nextHeaderSize = 40;
	writeInt(nextHeaderSize, _out);

	WriteWinBMPInfoHeader(_out);

	for (int y = 0; y < m_height; ++y)
	{
		Write24BitLine(_out, y);
	}
}


// ****************************************************************************
// Public Functions
// ****************************************************************************


BitmapRGBA::BitmapRGBA()
:	m_width(-1),
	m_height(-1),
	m_pixels(NULL),
	m_lines(NULL)
{
}


BitmapRGBA::BitmapRGBA(BitmapRGBA const &_other)
{
	Initialise(_other.m_width, _other.m_height);
	memcpy(m_pixels, _other.m_pixels, m_width * m_height * sizeof(RGBAColour));
}


BitmapRGBA::BitmapRGBA(int width, int height)
:	m_width(-1),
	m_height(-1),
	m_pixels(NULL),
	m_lines(NULL)
{
	Initialise(width, height);
}


// *** Constructor: Load from file
BitmapRGBA::BitmapRGBA(char const *_filename)
:	m_width(-1),
	m_height(-1),
	m_pixels(NULL),
	m_lines(NULL)
{
	Initialise(_filename);
}


// *** Constructor: Load from a BinaryReader
BitmapRGBA::BitmapRGBA(BinaryReader *_reader, char const *_type)
:	m_width(-1),
	m_height(-1),
	m_pixels(NULL),
	m_lines(NULL)
{
	Initialise(_reader, _type);
}


// *** Destructor
BitmapRGBA::~BitmapRGBA()
{
	m_width = -1;
	m_height = -1;
	delete [] m_pixels;
	delete [] m_lines;
	m_pixels = NULL;
	m_lines = NULL;
}


// *** Initialise
void BitmapRGBA::Initialise(int width, int height)
{
	m_width = width;
	m_height = height;
	m_pixels = new RGBAColour[width * height];
	m_lines = new RGBAColour *[height];

	AppDebugAssert(m_pixels && m_lines);

	for (int y = 0; y < height; ++y)
	{
		m_lines[y] = &m_pixels[y * width];
	}
}
	

void BitmapRGBA::Initialise(char const *_filename)
{
	BinaryFileReader in(_filename);

	char const *extension = GetExtensionPart(_filename);
	AppDebugAssert(stricmp(extension, "bmp") == 0);
	LoadBmp(&in);
}


void BitmapRGBA::Initialise(BinaryReader *_reader, char const *_type)
{
	AppDebugAssert(stricmp(_type, "bmp") == 0);
	LoadBmp(_reader);
}


// *** PutPixel
void BitmapRGBA::PutPixel(int x, int y, RGBAColour const &colour)
{
	m_lines[y][x] = colour;
}

// *** PutPixelOr
void BitmapRGBA::PutPixelOr(int x, int y, RGBAColour const &colour)
{
	m_lines[y][x] |= colour;
}


// *** GetPixel
RGBAColour const &BitmapRGBA::GetPixel(int x, int y) const
{
	return m_lines[y][x];
}


// *** PutPixelClipped
void BitmapRGBA::PutPixelClipped(int x, int y, RGBAColour const &colour)
{
	if (x < 0 || x >= m_width ||
		y < 0 || y >= m_height)
	{
		return;
	}

	m_lines[y][x] = colour;
}


// *** GetPixelClipped
RGBAColour const &BitmapRGBA::GetPixelClipped(int x, int y) const
{
	if (x < 0 || x >= m_width ||
		y < 0 || y >= m_height)
	{
		return g_colourBlack;
	}

	return m_lines[y][x]; 
}


void BitmapRGBA::DrawLine(int x1, int y1, int x2, int y2, RGBAColour const &colour)
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
RGBAColour BitmapRGBA::GetInterpolatedPixel(float _x, float _y) const
{
	int x1 = floorf(_x);
	int y1 = floorf(_y);
	int x2 = ceilf(_x);
	int y2 = ceilf(_y);

	if (x2 == m_width) 
		x2 = m_width - 1;

	if (y2 == m_height)
		y2 = m_height - 1;

	float fractionalX = _x - x1;
	float fractionalY = _y - y1;

	float weight11 = (1.0f - fractionalX) * (1.0f - fractionalY);
	float weight12 = (1.0f - fractionalX) * (fractionalY);
	float weight21 = (fractionalX) * (1.0f - fractionalY);
	float weight22 = (fractionalX) * (fractionalY);

    RGBAColour const &c11 = GetPixelClipped(x1, y1); 
    RGBAColour const &c12 = GetPixelClipped(x1, y2); 
    RGBAColour const &c21 = GetPixelClipped(x2, y1); 
    RGBAColour const &c22 = GetPixelClipped(x2, y2); 

	RGBAColour returnVal((float)c11.r * weight11 + (float)c12.r * weight12 + (float)c21.r * weight21 + (float)c22.r * weight22,
						 (float)c11.g * weight11 + (float)c12.g * weight12 + (float)c21.g * weight21 + (float)c22.g * weight22,
						 (float)c11.b * weight11 + (float)c12.b * weight12 + (float)c21.b * weight21 + (float)c22.b * weight22,
						 (float)c11.a * weight11 + (float)c12.a * weight12 + (float)c21.a * weight21 + (float)c22.a * weight22);

	return returnVal;
}


void BitmapRGBA::ConvertToGreyScale()
{
	RGBAColour *c = m_pixels;
	unsigned numPixels = m_width * m_height;

	while (numPixels--) {
		int average = (c->r + c->b + c->g) / 3;
		c->r = average;
		c->b = average;
		c->g = average;
		c++;
	}
}

void BitmapRGBA::ConvertToColour( RGBAColour _col, int _threshold )
{
    RGBAColour *c = m_pixels;
	unsigned numPixels = m_width * m_height;

	while (numPixels--) 
    {
        float average = (c->r + c->b + c->g) / 3.0f;
        if( average < _threshold )
        {
            int r, g, b;
            r = ((255.0f - average) / 255.0f) * _col.r;
            g = ((255.0f - average) / 255.0f) * _col.g;
            b = ((255.0f - average) / 255.0f) * _col.b;

		    c->r = r;
		    c->b = b;
		    c->g = g;		    
        }
        c++;
    }
}


void BitmapRGBA::ConvertColour( RGBAColour oldColour, RGBAColour newColour )
{
    RGBAColour *c = m_pixels;
    unsigned numPixels = m_width * m_height;

    while (numPixels--) 
    {        
        int rDif = (int)oldColour.r - (int)c->r;
        int gDif = (int)oldColour.g - (int)c->g;
        int bDif = (int)oldColour.b - (int)c->b;

        float diff = sqrtf( rDif * rDif + gDif * gDif + bDif * bDif );

        if( diff < 10 )
        {
            c->Set( newColour.r, newColour.g, newColour.b );
        }

        c++;
    }
}


void BitmapRGBA::ConvertRedChannel( RGBAColour newColour )
{
    RGBAColour *c = m_pixels;
    unsigned numPixels = m_width * m_height;

    while (numPixels--) 
    {        
        if( c->r > 0 && c->g == 0 && c->b == 0 )
        {
            float scale = c->r / 255.0f;
            RGBAColour scaledCol = newColour * scale;

            c->Set( scaledCol.r, scaledCol.g, scaledCol.b );
        }

        c++;
    }
}



// *** Blit
void BitmapRGBA::Blit(int _srcX, int _srcY, int _srcW, int _srcH, const BitmapRGBA *_srcBmp, 
					  int _destX, int _destY, int _destW, int _destH, bool _bilinear)
{
	AppDebugAssert(_srcX + _srcW <= _srcBmp->m_width);
	AppDebugAssert(_srcY + _srcH <= _srcBmp->m_height);
	AppDebugAssert(_destX + _destW <= m_width);
	AppDebugAssert(_destY + _destH <= m_height);

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
				RGBAColour const &c = _srcBmp->GetInterpolatedPixel(sx, sy);
				PutPixel(dx, dy, c);
				sx += sxPitch;
			}
		}
		else
		{
			for (int dx = _destX; dx < _destX + _destW; ++dx)
			{
				RGBAColour const &c = _srcBmp->GetPixelClipped(sx, sy);
				PutPixel(dx, dy, c);
				sx += sxPitch;
			}
		}
		sy += syPitch;
	}
}

// *** BlitOr
void BitmapRGBA::BlitOr(int _srcX, int _srcY, int _srcW, int _srcH, const BitmapRGBA *_srcBmp, 
						int _destX, int _destY, int _destW, int _destH, bool _bilinear)
{
	AppDebugAssert(_srcX + _srcW <= _srcBmp->m_width);
	AppDebugAssert(_srcY + _srcH <= _srcBmp->m_height);
	AppDebugAssert(_destX + _destW <= m_width);
	AppDebugAssert(_destY + _destH <= m_height);

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
				RGBAColour const &c = _srcBmp->GetInterpolatedPixel(sx, sy);
				PutPixelOr(dx, dy, c);
				sx += sxPitch;
			}
		}
		else
		{
			for (int dx = _destX; dx < _destX + _destW; ++dx)
			{
				RGBAColour const &c = _srcBmp->GetPixelClipped(sx, sy);
				PutPixelOr(dx, dy, c);
				sx += sxPitch;
			}
		}
		sy += syPitch;
	}
}


void BitmapRGBA::ApplyDilateFilter()
{
    RGBAColour *temp = new RGBAColour[ m_width * m_height ];
	memcpy( temp, m_pixels, sizeof(RGBAColour) * m_width * m_height );

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
                        RGBAColour col = temp[ (y+j) * m_width + (x+i) ];
                        adjacentRed += col.r;
                        adjacentGreen += col.g;
                        adjacentBlue += col.b;
                    }
                }
            }
            adjacentRed /= 8.0f;
            adjacentGreen /= 8.0f;
            adjacentBlue /= 8.0f;

            m_pixels[ y * m_width + x ].Set( adjacentRed, adjacentGreen, adjacentBlue );
        }
    }
	delete[] temp;
}


void BitmapRGBA::ApplyBlurFilter(float _scale)
{
	START_PROFILE( "ApplyBlur");

	AppDebugAssert(m_width > 0 && m_width <= 1024);
	AppDebugAssert(m_height > 0 && m_height <= 1024);

    RGBAColour *temp = new RGBAColour[ m_width * m_height ];
	memset(temp, 0, sizeof(RGBAColour) * m_width * m_height);


	int const blurSize = 5;
	int const halfBlurSize = 2;
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
			RGBAColour const &src = m_lines[y][x];
			if (src.r == 0 && src.g == 0 && src.b == 0) continue;

			for (int i = 0; i < blurSize; ++i)
			{
				int a = x + i - halfBlurSize;
				if (a < 0 || a >= m_width) continue;

				RGBAColour const &dest = temp[y * m_width + a];
				int r = dest.r + Round((float)src.r * m[i]);
				int g = dest.g + Round((float)src.g * m[i]);
				int b = dest.b + Round((float)src.b * m[i]);

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
	
	Clear(RGBAColour(0,0,0,0));
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			RGBAColour const &src = temp[y * m_width + x];
			if (src.r == 0 && src.g == 0 && src.b == 0) continue;

			for (int i = 0; i < blurSize; ++i)
			{
				int a = y + i - halfBlurSize;
				if (a < 0 || a >= m_height) continue;

				RGBAColour const &dest = m_lines[a][x];
				int r = dest.r + Round((float)src.r * m[i]);
				int g = dest.g + Round((float)src.g * m[i]);
				int b = dest.b + Round((float)src.b * m[i]);

				if (r > 255) r = 255;
				if (g > 255) g = 255;
				if (b > 255) b = 255;

				m_lines[a][x].Set(r, g, b);
			}
		}
	}

	delete[] temp;

	END_PROFILE( "ApplyBlur");
}


void BitmapRGBA::ConvertColourToAlpha()
{
	RGBAColour col;
    RGBAColour newCol( 255, 255, 255, 0 );

    for( int y = 0; y < m_height; ++y )
    {
	    for( int x = 0; x < m_width; ++x )
        {
            col = GetPixel( x, y );
            newCol.a = col.g;
            PutPixel( x, y, newCol );
        }
    }
}

		
void BitmapRGBA::ConvertPinkToTransparent() 
{
	const RGBAColour pink(255, 0, 255);
	const RGBAColour trans(128,128,128,0);

	for (int y = 0; y < m_height; ++y) 
	{
		for (int x = 0; x < m_width; ++x) 
		{
			RGBAColour const c(GetPixel(x, y));
			if (c == pink)
			{
				PutPixel(x, y, trans);
			}
		}
	}
}


void BitmapRGBA::Clear( RGBAColour const &colour )
{
	for (int x = 0; x < m_width; ++x) 
	{
        PutPixel( x, 0, colour );
    }

	int const size = sizeof(RGBAColour) * m_width;
 	for (int y = 0; y < m_height; ++y) 
	{
		memcpy(m_lines[y], m_lines[0], size);
    }
}

void BitmapRGBA::SwapRB()
{
	for (int y = 0; y < m_height; ++y) 
	{
		for (int x = 0; x < m_width; ++x) 
		{
			RGBAColour const c(GetPixel(x, y));
			PutPixel(x, y, RGBAColour(c.b,c.g,c.r,c.a));
		}
	}
}


class TextureToConvert : public Job
{
public:
	TextureToConvert( const BitmapRGBA &_bitmap, bool _mipmapping, bool _compressed )
		:	m_bitmap( _bitmap ), 
			m_mipmapping( _mipmapping ),
			m_texId( -1 ),
			m_compressed(_compressed)
	{
	}

	int WaitTexId()
	{
		Wait();
		return m_texId;
	}

protected:
	void Run()
	{
		//AppDebugOut( "Converting Texture in Loading Thread\n" );
		m_texId = m_bitmap.ConvertToTexture( m_mipmapping, m_compressed );
	}

private:
	const BitmapRGBA	&m_bitmap;
	bool				m_mipmapping;
	int					m_texId;
	bool				m_compressed;
};

int BitmapRGBA::ConvertToTextureAsync(bool _mipmapping, bool _compressed) const
{
	if( NetGetCurrentThreadId() == g_app->m_mainThreadId )
		return ConvertToTexture( _mipmapping, _compressed );
	else 
	{
		TextureToConvert t( *this, _mipmapping, _compressed );
		g_loadingScreen->QueueJob( &t );
		return t.WaitTexId();
	}
}

int BitmapRGBA::ConvertToTexture(bool _mipmapping, bool _compressed) const
{
	bool requireScale;
	bool requirePowerOfTwo;
	bool requireSquare;
	
#if defined(USE_DIRECT3D) 
	requirePowerOfTwo = true;
	requireSquare = false;
#else
	// Could add in some additional check here to check for ARB_texture_rectangle support
	requirePowerOfTwo = RequirePowerOfTwoTextures() || g_prefsManager->GetInt("ManuallyScaleTextures", 0);
	requireSquare = RequireSquareTextures();
#endif
	int maxTextureSize = GetMaxTextureSize();
	if(maxTextureSize != 0)
		requireScale = true;
	
	if( requireScale )
	{
		int newWidth = m_width; 
		int newHeight = m_height; 

		if(maxTextureSize != 0)
		{
			float scaleRatio = 1.0;
			float widthRatio = 1.0;
			float heightRatio = 1.0;
			
			// determine which axis controls the scale ratio
			if(m_width > maxTextureSize)
				widthRatio = (float)maxTextureSize / (float) m_width;
			if(m_height > maxTextureSize)
				heightRatio = (float)maxTextureSize / (float) m_height;
			
			// determine if we should scale according to the width or the height
			if(widthRatio < heightRatio)
				scaleRatio = widthRatio;
			else
				scaleRatio = heightRatio;
			
			// if the scale ratio is over 1.0 then we don't want to scale at all
			// we never scale up, only down
			if(scaleRatio < 1.0)
			{
				newWidth = scaleRatio * (float) m_width;
				newHeight = scaleRatio * (float) m_height;
			}
		}
		
		if(requirePowerOfTwo)
		{
			newWidth = nearestPowerOfTwo( m_width ); 
			newHeight = nearestPowerOfTwo( m_height );
		}
		
		if(requireSquare)
			newWidth = newHeight = max(newWidth, newHeight);
		
		if( m_width != newWidth || m_height != newHeight )
		{
			BitmapRGBA scaled(newWidth, newHeight);
			scaled.Blit(0, 0, m_width, m_height, this, 
						0, 0, newWidth, newHeight, true);

			return scaled.ConvertToTexture( _mipmapping, _compressed );
		}
	}

	GLuint texId;
	glGenTextures	(1, &texId);
	glBindTexture	(GL_TEXTURE_2D, texId);

    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

#ifdef USE_DIRECT3D
#define PASS_COMPRESSED , _compressed
#else
#define PASS_COMPRESSED 
#endif

    if (_mipmapping)
	{
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, 
			m_width, m_height, 
			GL_RGBA, GL_UNSIGNED_BYTE, 
			m_pixels PASS_COMPRESSED );
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, 4, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels PASS_COMPRESSED );
	}

	this->CheckGLError();
	return (int) texId;
}

void BitmapRGBA::SetMaxTextureSize(int size)
{	gMaxTextureSize = size;		}
int BitmapRGBA::GetMaxTextureSize(void)
{	return gMaxTextureSize;		}

void BitmapRGBA::CheckGLError() const
{
	GLenum err = glGetError();
	if( err != GL_NO_ERROR )
	{
		AppDebugOut( "GL error: %d\n", err );
	}
}
