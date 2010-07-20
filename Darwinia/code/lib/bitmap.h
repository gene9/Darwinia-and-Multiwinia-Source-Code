#ifndef INCLUDED_BITMAP_H
#define INCLUDED_BITMAP_H


class RGBAColour;
class jpeg_decoder;
class BitmapFileHeader;
class BitmapInfoHeader;
class BinaryReader;


class BitmapRGBA
{
private:
	void ReadBMPFileHeader(BinaryReader *f, BitmapFileHeader *fileheader);
	void ReadWinBMPInfoHeader(BinaryReader *f, BitmapInfoHeader *infoheader);
	void ReadOS2BMPInfoHeader(BinaryReader *f, BitmapInfoHeader *infoheader);

	void ReadBMPPalette(int ncols, RGBAColour pal[256], BinaryReader *f, int win_flag);
	void Read4BitLine(int length, BinaryReader *f, RGBAColour *pal, int line);
	void Read8BitLine(int length, BinaryReader *f, RGBAColour *pal, int line);
	void Read24BitLine(int length, BinaryReader *f, int line);
	void LoadBmp(BinaryReader *_in);

	void WriteBMPFileHeader(FILE *_out);
	void WriteWinBMPInfoHeader(FILE *_out);
	void Write24BitLine(FILE *_out, int _y);

public:
	int m_width;
	int m_height;
	RGBAColour *m_pixels;
	RGBAColour **m_lines;

	BitmapRGBA();
	BitmapRGBA(BitmapRGBA const &_other);
	BitmapRGBA(int _width, int _height);
	BitmapRGBA(char const *_filename);
	BitmapRGBA(BinaryReader *_reader, char const *_type);
	~BitmapRGBA();

	void Initialise(int _width, int _height);
	void Initialise(char const *_filename);
	void Initialise(BinaryReader *_reader, char const *_type);

	void SavePng(const char *_filename, bool _saveAlpha = false);
	void WritePng(FILE *_out, bool _saveAlpha = false);

	void SaveBmp(char const *_filename);
	void WriteBmp(FILE *_out);

    void Clear( RGBAColour const &colour );

	void PutPixel(int x, int y, RGBAColour const &colour);
	RGBAColour const &GetPixel(int x, int y) const;

	void PutPixelClipped(int x, int y, RGBAColour const &colour);
	RGBAColour const &GetPixelClipped(int x, int y) const;

    void DrawLine(int x1, int y1, int x2, int y2, RGBAColour const &colour);

	RGBAColour GetInterpolatedPixel(float x, float y) const;

	void Blit(int srcX,  int srcY,  int srcW,  int srcH, const BitmapRGBA *_srcBmp,
			  int destX, int destY, int destW, int destH, bool _bilinear);

    void ApplyBlurFilter(float _scale);
    void ApplyDilateFilter();

	void ConvertPinkToTransparent();
	void ConvertColourToAlpha();	// Luminance of rgb data is copied into the alpha channel and the rgb data is set to 255,255,255
	void ConvertToGreyScale();		// Colour is averaged out
	int ConvertToTexture(bool _mipmapping = true) const;
};


#endif
