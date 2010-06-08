#ifndef INCLUDED_POSTER_MAKER_H
#define INCLUDED_POSTER_MAKER_H


class BitmapRGBA;


class PosterMaker
{
protected:
	int			m_x;
	int			m_y;
	int			m_screenWidth;
	int			m_screenHeight;
	BitmapRGBA	*m_bitmap;
	unsigned char *m_screenPixels;
    
public:
	PosterMaker(int _screenWidth, int _screenHeight);
	~PosterMaker();

	void AddFrame();
    void SavePoster();

	BitmapRGBA *GetBitmap();
};


#endif
