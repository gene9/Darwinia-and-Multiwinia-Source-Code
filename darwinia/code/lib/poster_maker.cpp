#include "lib/universal_include.h"
#include "lib/bitmap.h"
#include "lib/debug_utils.h"
#include "lib/poster_maker.h"
#include "lib/rgb_colour.h"
#include "lib/preferences.h"
#include "lib/filesys_utils.h"
#include "app.h"

#define COMPONENTS 3

PosterMaker::PosterMaker(int _screenWidth, int _screenHeight)
:	m_screenWidth(_screenWidth),
	m_screenHeight(_screenHeight),
	m_x(0),
	m_y(0)
{
    int posterResolution = g_prefsManager->GetInt( "RenderPosterResolution", 1 );

	m_bitmap = new BitmapRGBA(_screenWidth * posterResolution,
							  _screenHeight * posterResolution);
	m_bitmap->Clear(g_colourBlack);

	m_screenPixels = new unsigned char[_screenWidth * _screenHeight * COMPONENTS];
}


PosterMaker::~PosterMaker()
{
	delete m_bitmap;		m_bitmap = NULL;
	delete m_screenPixels;	m_screenPixels = NULL;
}


void PosterMaker::AddFrame()
{
    int posterResolution = g_prefsManager->GetInt( "RenderPosterResolution", 1 );

    DarwiniaDebugAssert(m_x < posterResolution && m_y < posterResolution);

	//
	// Copy of the frame buffer into system memory

    glReadPixels(0, 0, m_screenWidth, m_screenHeight, GL_RGB, GL_UNSIGNED_BYTE, m_screenPixels);
	

	//
	// Work out where the current tile goes

	int tx = m_x * m_screenWidth;
	int ty = m_y * m_screenHeight;


	//
	// Copy tile data into big bitmap

	for (int y = 0; y < m_screenHeight; ++y)
	{
		unsigned char *line = &m_screenPixels[y * m_screenWidth * COMPONENTS];
		for (int x = 0; x < m_screenWidth; ++x)
		{
			unsigned char *p = &line[x * COMPONENTS];
			m_bitmap->PutPixel(tx + x, ty + y, RGBAColour(p[0], p[1], p[2]));
		}
	}

	
	//
	// Move on to the next tile

	++m_x;
	if (m_x >= posterResolution)
	{
		m_x = 0;
		++m_y;
	}
}

void PosterMaker::SavePoster()
{
    char filename[256];
    int i = 1;
    const char *path = g_app->GetScreenshotDirectory();

	while( true )
    {
        sprintf( filename, "%sscreenshot%d.png", path, i );
        if( !DoesFileExist(filename) )
        {
            m_bitmap->SavePng(filename);
            break;
        }
        ++i;
    }
}

BitmapRGBA *PosterMaker::GetBitmap()
{
	return m_bitmap;
}