#ifndef INCLUDED_BITMAP_H
#define INCLUDED_BITMAP_H

#include <stdio.h>

class Colour;
class BitmapFileHeader;
class BitmapInfoHeader;
class BinaryReader;


class Bitmap
{
private:
	void ReadBMPFileHeader      (BinaryReader *f, BitmapFileHeader *fileheader);
	void ReadWinBMPInfoHeader   (BinaryReader *f, BitmapInfoHeader *infoheader);
	void ReadOS2BMPInfoHeader   (BinaryReader *f, BitmapInfoHeader *infoheader);

	void ReadBMPPalette         (int ncols, Colour pal[256], BinaryReader *f, int win_flag);
	void Read4BitLine           (int length, BinaryReader *f, Colour *pal, int line);
	void Read8BitLine           (int length, BinaryReader *f, Colour *pal, int line);
	void Read24BitLine          (int length, BinaryReader *f, int line);
	
    void LoadBmp                (BinaryReader *_in);
	
	void WriteBMPFileHeader     (FILE *_out);
	void WriteWinBMPInfoHeader  (FILE *_out);
	void Write24BitLine         (FILE *_out, int _y);

public:
	int         m_width;
	int         m_height;
	Colour      *m_pixels;
	Colour      **m_lines;

public:
	Bitmap      ();
	Bitmap      (Bitmap &_other);
	Bitmap      (int _width, int _height);
	Bitmap      (BinaryReader *_reader, char *_type);
	~Bitmap     ();

	void        Initialise              (int _width, int _height);
	void        Initialise              (BinaryReader *_reader, char *_type);
    
    void        Clear                   ( Colour const &colour );

	void        PutPixel                (int x, int y, Colour const &colour);
    void        PutPixelClipped         (int x, int y, Colour const &colour);
    Colour      &GetPixel               (int x, int y);
	Colour      &GetPixelClipped        (int x, int y);
	Colour      GetInterpolatedPixel    (float x, float y);

    void        DrawLine                (int x1, int y1, int x2, int y2, Colour const &colour);

	void        Blit                    (int srcX,  int srcY,  int srcW,  int srcH, Bitmap *_srcBmp, 
			                             int destX, int destY, int destW, int destH, bool _bilinear);
    
    void        ApplyBlurFilter         (float _scale);
    void        ApplyDilateFilter       ();

	void        ConvertPinkToTransparent    ();
	void        ConvertColourToAlpha        ();	                                    // Luminance of rgb data is copied into the alpha channel and the rgb data is set to 255,255,255
	int         ConvertToTexture            (bool _mipmapping = true);

	void        SaveBmp                     (char *_filename);
};


#endif
