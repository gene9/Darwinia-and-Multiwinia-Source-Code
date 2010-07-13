#define _CRT_SECURE_NO_DEPRECATE
#include "lib/universal_include.h"

#include <time.h>
#include <stdarg.h>
#include <math.h>

#include "lib/eclipse/eclipse.h"
#include "lib/gucci/window_manager.h"
#include "lib/gucci/input.h"
#include "lib/resource/bitmapfont.h"
#include "lib/resource/resource.h"
#include "lib/resource/image.h"
#include "lib/resource/bitmap.h"
#ifndef NO_RENDERER_3D
#  include "lib/math/vector3.h"
#endif // NO_RENDERER_3D
#include "lib/math/math_utils.h"
#include "lib/hi_res_time.h"
#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "lib/string_utils.h"
#include "lib/filesys/filesys_utils.h"


#include "renderer.h"
#include "colour.h"

Renderer *g_renderer = NULL;


static const char *GetFontFilename( const char *_fontName )
{
    static char result[512];
    sprintf( result, "fonts/%s.bmp", _fontName );
    return result;
}


Renderer::Renderer()
:   m_alpha(1.0f),
    m_currentFontFilename(NULL),
    m_defaultFontFilename(NULL),
    m_horizFlip(false),
    m_fixedWidth(false)
{    
}


void Renderer::Set2DViewport ( float l, float r, float b, float t,
                               int x, int y, int w, int h )
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(l - 0.325, r - 0.325, b - 0.325, t - 0.325);    
    glViewport( x, g_windowManager->WindowH() - h - y, w, h );
}

void Renderer::BeginScene()
{
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable    ( GL_BLEND );

    glDisable   ( GL_POLYGON_SMOOTH );

    bool smoothLines = g_preferences->GetInt( PREFS_GRAPHICS_SMOOTHLINES );

    if( smoothLines )
    {
        glHint      ( GL_LINE_SMOOTH_HINT, GL_NICEST );
        glHint      ( GL_POINT_SMOOTH_HINT, GL_NICEST );
        glHint      ( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

        glEnable    ( GL_LINE_SMOOTH );
        glEnable    ( GL_POINT_SMOOTH );
    }
    else
    {
        glDisable   ( GL_LINE_SMOOTH );
        glDisable   ( GL_POINT_SMOOTH );
    }
}


void Renderer::ClearScreen( bool _colour, bool _depth )
{
    if( _colour ) glClear( GL_COLOR_BUFFER_BIT );
    if( _depth ) glClear( GL_DEPTH_BUFFER_BIT );
}


void Renderer::Reset2DViewport()
{
    Set2DViewport( 0, 
                   g_windowManager->WindowW(), 
                   g_windowManager->WindowH(), 
                   0, 0, 0, 
                   g_windowManager->WindowW(), 
                   g_windowManager->WindowH() );
}


void Renderer::Shutdown()
{
}


void Renderer::SetBlendMode( int _blendMode )
{
    m_blendMode = _blendMode;
    
    switch( _blendMode )
    {
        case BlendModeDisabled:
            glDisable   ( GL_BLEND );
            break;

        case BlendModeNormal:
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glEnable    ( GL_BLEND );
            break;

        case BlendModeAdditive:
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
            glEnable    ( GL_BLEND );
            break;

        case BlendModeSubtractive:
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
            glEnable    ( GL_BLEND );
            break;

        case BlendModeSubtractiveSpecial:
            glBlendFunc ( GL_ONE_MINUS_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR );
            glEnable    ( GL_BLEND );
            break;
    }
}


void Renderer::SetDepthBuffer( bool _enabled, bool _clearNow )
{
    if( _enabled )
    {
        glEnable ( GL_DEPTH_TEST );
        glDepthMask( true );
    }
    else
    {
        glDisable( GL_DEPTH_TEST );
        glDepthMask( false );
    }
 
    if( _clearNow )     
    {
        glClear( GL_DEPTH_BUFFER_BIT );
    }
}


void Renderer::SetDefaultFont( const char *font )
{
    const char *fullFilename = GetFontFilename(font);

    m_defaultFontFilename = newStr( fullFilename );    
    m_currentFontFilename = newStr( fullFilename );
}


void Renderer::SetFontSpacing( const char *font, float _spacing )
{
    BitmapFont *theFont = g_resource->GetBitmapFont( GetFontFilename(font) );
    if( theFont )
    {
        theFont->SetSpacing( _spacing );
    }
}


void Renderer::SetFont( const char *font, bool horizFlip, bool negative, bool fixedWidth )
{
    if( m_currentFontFilename )
    {
        delete [] m_currentFontFilename;
        m_currentFontFilename = NULL;
    }

    if( font )
    {
        const char *fontFilename = GetFontFilename(font);
        m_currentFontFilename = newStr( fontFilename );
    }

    m_horizFlip = horizFlip;
    m_fixedWidth = fixedWidth;

    if( negative )
    {
        SetBlendMode( BlendModeSubtractive );
    }
    else
    {
        SetBlendMode( BlendModeNormal );
    }
}


void Renderer::Text( float x, float y, Colour const &col, float size, const char *text, ... )
{   
    char buf[1024];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    TextSimple( x, y, col, size, buf );
}


void Renderer::TextCentre ( float x, float y, Colour const &col, float size, const char *text, ... )
{
    char buf[1024];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    float actualX = x - TextWidth( buf, size )/2.0f;
    TextSimple( actualX, y, col, size, buf );
}


void Renderer::TextRight( float x, float y, Colour const &col, float size, const char *text, ...)
{
    char buf[1024];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    float actualX = x - TextWidth( buf, size );
    TextSimple( actualX, y, col, size, buf );
}

void Renderer::TextSimple( float x, float y, Colour const &col, float size, const char *text )
{
    char safeText[1024];
    strcpy( safeText, text );
    StringReplace("%", "%%", safeText );
    
    BitmapFont *font = g_resource->GetBitmapFont( m_currentFontFilename );
    if( !font ) font = g_resource->GetBitmapFont( m_defaultFontFilename );

    if( font )
    {    
        glColor4ub( col.m_r, col.m_g, col.m_b, col.m_a );    
        font->SetHoriztonalFlip( m_horizFlip );
        font->SetFixedWidth( m_fixedWidth );
                
        GLint blendSrc, blendDst;
        glGetIntegerv( GL_BLEND_SRC, &blendSrc );
        glGetIntegerv( GL_BLEND_DST, &blendDst );
   
        if( m_blendMode != BlendModeSubtractive && m_blendMode != BlendModeSubtractiveSpecial )
        {
            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        }
        
        font->DrawText2D( x, y, size, safeText );

        glBlendFunc( blendSrc, blendDst );
    }
}

float Renderer::TextWidth ( const char *text, float size )
{
    BitmapFont *font = g_resource->GetBitmapFont( m_currentFontFilename );
    if( !font ) font = g_resource->GetBitmapFont( m_defaultFontFilename );

    if( !font ) return -1;
    return font->GetTextWidth( text, size );
}

void Renderer::Rect ( float x, float y, float w, float h, Colour const &col, float lineWidth )
{
    glLineWidth(lineWidth);
    glColor4ub( col.m_r, col.m_g, col.m_b, col.m_a );
    glBegin( GL_LINE_LOOP );
        glVertex2f( x, y );
        glVertex2f( x + w, y );
        glVertex2f( x + w, y + h );
        glVertex2f( x, y + h );
    glEnd();
}

void Renderer::RectFill ( float x, float y, float w, float h, Colour const &col )
{
    RectFill( x, y, w, h, col, col, col, col );
}


void Renderer::RectFill( float x, float y, float w, float h, Colour const &col1, Colour const &col2, bool horizontal )
{
    if( horizontal )
    {
        RectFill( x, y, w, h, col1, col1, col2, col2 );
    }
    else
    {
        RectFill( x, y, w, h, col1, col2, col2, col1 );
    }
}


void Renderer::RectFill( float x, float y, float w, float h, Colour const &colTL, Colour const &colTR, Colour const &colBR, Colour const &colBL )
{
    glBegin( GL_QUADS );
        glColor4ub( colTL.m_r, colTL.m_g, colTL.m_b, colTL.m_a );           glVertex2f( x, y );
        glColor4ub( colTR.m_r, colTR.m_g, colTR.m_b, colTR.m_a );           glVertex2f( x + w, y );
        glColor4ub( colBR.m_r, colBR.m_g, colBR.m_b, colBR.m_a );           glVertex2f( x + w, y + h );
        glColor4ub( colBL.m_r, colBL.m_g, colBL.m_b, colBL.m_a );           glVertex2f( x, y + h );
    glEnd();
}


void Renderer::Line ( float x1, float y1, float x2, float y2, Colour const &col, float lineWidth )
{
    glLineWidth( lineWidth );
    glColor4ub( col.m_r, col.m_g, col.m_b, col.m_a );
    glBegin( GL_LINES );
        glVertex2f( x1, y1 );
        glVertex2f( x2, y2 );
    glEnd();
}

void Renderer::Circle( float x, float y, float radius, int numPoints, Colour const &col, float lineWidth )
{
    glLineWidth( lineWidth );
    glColor4ub( col.m_r, col.m_g, col.m_b, col.m_a );

    glBegin( GL_LINE_LOOP );
    for( int i = 0; i < numPoints; ++i )
    {
        float angle = 2.0f * M_PI * (float) i / (float) numPoints;
        float thisX = x + cosf(angle) * radius;
        float thisY = y + sinf(angle) * radius;

        glVertex2f( thisX, thisY );
    }
    glEnd();
}

void Renderer::CircleFill ( float x, float y, float radius, int numPoints, Colour const &col )
{
    glColor4ub( col.m_r, col.m_g, col.m_b, col.m_a );

    glBegin( GL_TRIANGLE_FAN );
    glVertex2f( x, y );

    for( int i = 0; i < numPoints; ++i )
    {
        float angle = 2.0f * M_PI * (float) i / (float) numPoints;
        float thisX = x + cosf(angle) * radius;
        float thisY = y + sinf(angle) * radius;
        glVertex2f( thisX, thisY );
    }

    glVertex2f( x + radius, y );

    glEnd();
}

Renderer & Renderer::BeginLines ( Colour const &col, float lineWidth )
{
    glColor4ub( col.m_r, col.m_g, col.m_b, col.m_a );
    glLineWidth( lineWidth );
    glBegin( GL_LINE_LOOP );
	return *this;
}

Renderer & Renderer::Line( float x, float y )
{
    glVertex2f( x, y );
	return *this;
}

Renderer & Renderer::EndLines()
{
    glEnd();
	return *this;
}

void Renderer::SetClip( int x, int y, int w, int h )
{
    glScissor( x, g_windowManager->WindowH() - h - y, w, h );
    glEnable( GL_SCISSOR_TEST );
}

void Renderer::ResetClip()
{
    glDisable( GL_SCISSOR_TEST );
}

void Renderer::Blit( Image *src, float x, float y, float w, float h, Colour const &col )
{    
    glColor4ub      ( col.m_r, col.m_g, col.m_b, col.m_a );    

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, src->m_textureID );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if( src->m_mipmapping ) glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    else                    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );                
            
    float onePixelW = 1.0f / (float) src->Width();
    float onePixelH = 1.0f / (float) src->Height();

    glBegin( GL_QUADS );
        glTexCoord2f( onePixelW, 1.0f-onePixelH );          glVertex2f( x, y );
        glTexCoord2f( 1.0f-onePixelW, 1-onePixelH );        glVertex2f( x+w, y );
        glTexCoord2f( 1.0f-onePixelW, onePixelH );          glVertex2f( x+w, y+h );
        glTexCoord2f( onePixelW, onePixelH );               glVertex2f( x, y+h );
    glEnd();                

    glDisable       ( GL_TEXTURE_2D );
}

void Renderer::Blit( Image *image, float x, float y, Colour const &col )
{
    float w = image->Width();
    float h = image->Height();
    
    Blit( image, x, y, w, h, col );
}

#ifndef NO_RENDERER_3D
void Renderer::Blit ( Image *src, float x, float y, float w, float h, Colour const &col, float angle)
{    
    Vector3 vert1( -w, +h,0 );
    Vector3 vert2( +w, +h,0 );
    Vector3 vert3( +w, -h,0 );
    Vector3 vert4( -w, -h,0 );

    vert1.RotateAroundZ(angle);
    vert2.RotateAroundZ(angle);
    vert3.RotateAroundZ(angle);
    vert4.RotateAroundZ(angle);

    vert1 += Vector3( x, y, 0 );
    vert2 += Vector3( x, y, 0 );
    vert3 += Vector3( x, y, 0 );
    vert4 += Vector3( x, y, 0 );

    glColor4ub      ( col.m_r, col.m_g, col.m_b, col.m_a );   
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, src->m_textureID );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    if( src->m_mipmapping ) glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    else                    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );                
    
    float onePixelW = 1.0f / (float) src->Width();
    float onePixelH = 1.0f / (float) src->Height();
        
    glBegin( GL_QUADS );
        glTexCoord2f( onePixelW, 1-onePixelH );         glVertex2dv( &(vert1.x) );
        glTexCoord2f( 1-onePixelW, 1-onePixelH );       glVertex2dv( &(vert2.x) );
        glTexCoord2f( 1-onePixelW, onePixelH );         glVertex2dv( &(vert3.x) );
        glTexCoord2f( onePixelW, onePixelH );           glVertex2dv( &(vert4.x) );
    glEnd();                

    glDisable       ( GL_TEXTURE_2D );
}
#endif // NO_RENDERER_3D


void Renderer::SaveScreenshot()
{
    float timeNow = GetHighResTime();
	char *screenshotsDir = ScreenshotsDirectory();

    int screenW = g_windowManager->WindowW();
    int screenH = g_windowManager->WindowH();    
    
    unsigned char *screenBuffer = new unsigned char[screenW * screenH * 3];

    glReadPixels(0, 0, screenW, screenH, GL_RGB, GL_UNSIGNED_BYTE, screenBuffer);


	//
	// Copy tile data into big bitmap

    Bitmap bitmap( screenW, screenH );
    bitmap.Clear(Black);
    
	for (int y = 0; y < screenH; ++y)
	{
		unsigned const char *line = &screenBuffer[y * screenW * 3];
		for (int x = 0; x < screenW; ++x)
		{
			unsigned const char *p = &line[x * 3];
			bitmap.PutPixel(x, y, Colour(p[0], p[1], p[2]));
		}
	}

    //
    // Save the bitmap to a file

    int screenshotIndex = 1;
    while( true )
    {
        time_t theTimeT = time(NULL);
        tm *theTime = localtime(&theTimeT);
        char filename[2048];
		
        snprintf( filename, sizeof(filename), "%s/screenshot %02d-%02d-%02d %02d.bmp", 
								screenshotsDir,
								1900+theTime->tm_year, 
                                theTime->tm_mon+1,
                                theTime->tm_mday,
                                screenshotIndex );
        if( !DoesFileExist(filename) )
        {
            bitmap.SaveBmp( filename );
            break;
        }
        ++screenshotIndex;
    }

    //
    // Clear up

    delete[] screenBuffer;
	delete[] screenshotsDir;

	float timeTaken = GetHighResTime() - timeNow;
    AppDebugOut( "Screenshot %dms\n", int(timeTaken*1000) );

}

char *Renderer::ScreenshotsDirectory()
{
#ifdef TARGET_OS_MACOSX
	if ( getenv("HOME") )
		return ConcatPaths( getenv("HOME"), "Pictures", NULL );
#endif
	return newStr(".");
}
