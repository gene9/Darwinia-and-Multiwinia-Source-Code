#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/rgb_colour.h"
#include "lib/resource.h"
#include "lib/bitmap.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"

#include "app.h"
#include "team.h"
#include "location.h"
#include "global_world.h"
#include "renderer.h"

#include "animatedpanel_renderer.h"



AnimatedPanelRenderer::AnimatedPanelRenderer()
:   AnimatedPanel(),
    m_animTimer(-1.0f),
    m_screenX(0),
    m_screenY(0),
    m_screenW(200),
    m_screenH(200),
    m_borderSize(0),
    m_titleH(0),
    m_captionH(0)
{
}


void AnimatedPanelRenderer::RenderBackground( float alpha )
{
    //
    // Render a fill box

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glColor4f( 0.0f, 0.0f, 0.0f, alpha );

    float boxS = 5;

    glBegin( GL_QUADS );
        glVertex2f( m_screenX -m_borderSize, m_screenY-m_titleH-m_borderSize );
        glVertex2f( m_screenX + m_screenW+m_borderSize, m_screenY-m_titleH-m_borderSize );
        glVertex2f( m_screenX + m_screenW+m_borderSize, m_screenY + m_screenH+m_borderSize+m_captionH );
        glVertex2f( m_screenX -m_borderSize, m_screenY + m_screenH+m_borderSize+m_captionH );
    glEnd();

    //
    // Render an outer line

    glColor4f( 1.0f, 1.0f, 1.0f, alpha*0.5f );
    glBegin( GL_LINE_LOOP );
        glVertex2f( m_screenX -m_borderSize, m_screenY-m_titleH-m_borderSize );
        glVertex2f( m_screenX + m_screenW+m_borderSize, m_screenY-m_titleH-m_borderSize );
        glVertex2f( m_screenX + m_screenW+m_borderSize, m_screenY + m_screenH+m_borderSize+m_captionH );
        glVertex2f( m_screenX -m_borderSize, m_screenY + m_screenH+m_borderSize+m_captionH );
    glEnd();

    glColor4f( 1.0f, 1.0f, 1.0f, alpha*0.25f );
    glBegin( GL_LINE_LOOP );
        glVertex2f( m_screenX, m_screenY-m_titleH );
        glVertex2f( m_screenX + m_screenW, m_screenY-m_titleH);
        glVertex2f( m_screenX + m_screenW, m_screenY + m_screenH+m_captionH);
        glVertex2f( m_screenX, m_screenY + m_screenH+m_captionH);
    glEnd();
}



void AnimatedPanelRenderer::Blit ( int texId, float x, float y, float w, float h, float angle, float texX, float texY, float texW, float texH )
{    
    Vector3 vert1( -w/2.0f, +h/2.0f,0 );
    Vector3 vert2( +w/2.0f, +h/2.0f,0 );
    Vector3 vert3( +w/2.0f, -h/2.0f,0 );
    Vector3 vert4( -w/2.0f, -h/2.0f,0 );

    vert1.RotateAroundZ(angle);
    vert2.RotateAroundZ(angle);
    vert3.RotateAroundZ(angle);
    vert4.RotateAroundZ(angle);

    vert1 += Vector3( x+w/2, y+h/2, 0 );
    vert2 += Vector3( x+w/2, y+h/2, 0 );
    vert3 += Vector3( x+w/2, y+h/2, 0 );
    vert4 += Vector3( x+w/2, y+h/2, 0 );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, texId );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

    glBegin( GL_QUADS );
        glTexCoord2f( texX, texY );                 glVertex2dv( &(vert1.x) );
        glTexCoord2f( texX+texW, texY );            glVertex2dv( &(vert2.x) );
        glTexCoord2f( texX+texW, texY+texH );       glVertex2dv( &(vert3.x) );
        glTexCoord2f( texX, texY+texH );            glVertex2dv( &(vert4.x) );
    glEnd();                

    glDisable       ( GL_TEXTURE_2D );
}


void AnimatedPanelRenderer::RenderObjects()
{
    double timeNow = GetHighResTime();

    //
    // First time rendering this animation?

    if( m_animTimer < 0.0f )
    {
        m_time = 0;
        m_animTimer = GetHighResTime();
    }

    //
    // Advance time

    double timePassed = timeNow - m_animTimer;
    m_time += timePassed * 1.0f;
    if( m_time > m_stop )
    {
        m_time = m_start;
    }
    m_animTimer = timeNow;

    g_app->m_renderer->Clip( m_screenX+1, 
                            m_screenY - m_titleH+1, 
                            m_screenW-1, 
                            m_screenH + m_titleH + m_captionH -1 );


    //
    // Render all objects

    float scale = m_screenW / m_width;

    for( int i = 0; i < m_objects.Size(); ++i )
    {
        if( m_objects.ValidIndex(i) )
        {
            AnimatedPanelKeyframe keyframe;
            bool success = CalculateObjectProperties( i, m_time, keyframe );

            if( success )
            {
                float size = 10;

                RGBAColour baseColour(255,255,255,255);

                switch( keyframe.m_colour )
                {
                    case AnimatedPanelKeyframe::ColourFriendly: 
                    {
                        Team *myTeam = g_app->m_location->GetMyTeam();
                        baseColour = myTeam->m_colour;
                        break;
                    }
                    case AnimatedPanelKeyframe::ColourEnemy:
                    {
                        for( int i = 0; i < NUM_TEAMS; ++i )
                        {
                            Team *team = g_app->m_location->m_teams[i];
                            if( team->m_teamType != TeamTypeUnused &&
                                !g_app->m_location->IsFriend( g_app->m_globalWorld->m_myTeamId, i ) )
                            {
                                baseColour = team->m_colour;
                                break;
                            }
                        }
                        break;
                    }
                    case AnimatedPanelKeyframe::ColourInvisible: 
                    {
                        baseColour.Set( 255, 255, 255, 0 );         
                        break;
                    }
                }

                if( keyframe.m_colour != AnimatedPanelKeyframe::ColourInvisible )
                {
                    baseColour.a = keyframe.m_alpha;
                }

                char fullFilename[512];
                sprintf( fullFilename, "animations/%s", keyframe.m_imageName );
                const BitmapRGBA *image = g_app->m_resource->GetBitmap( fullFilename );
                int texId = g_app->m_resource->GetTexture( fullFilename );

                if( image )
                {
                    float x = m_screenX + keyframe.m_x * scale;
                    float y = m_screenY + keyframe.m_y * scale;
                    float w = image->m_width * keyframe.m_scale * scale;
                    float h = image->m_height * keyframe.m_scale * scale;

                    float texX = 0;
                    float texY = 0; 
                    float texW = 1;
                    float texH = 1;

                    if( keyframe.m_flipHoriz )
                    {
                        texX = 1;
                        texW = -1;
                    }

                    float angle = 2.0f * M_PI * keyframe.m_angle/360.0f;

                    glColor4ubv( baseColour.GetData() );   
                    
                    Blit( texId, x - w/2.0f, y - h/2.0f, w, h, angle, texX, texY, texW, texH );             
                }
            }
        }
    }

    g_app->m_renderer->EndClip();

}


void AnimatedPanelRenderer::RenderTitle( UnicodeString &title )
{
    float fontSize = 30;

    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_titleFont.SetRenderOutline(true);
    g_titleFont.DrawText2DCentre( m_screenX + m_screenW/2.0f, m_screenY-m_titleH+10, fontSize, title );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_titleFont.SetRenderOutline(false);
    g_titleFont.DrawText2DCentre( m_screenX + m_screenW/2.0f, m_screenY-m_titleH+10, fontSize, title  );
}


void AnimatedPanelRenderer::RenderCaption( UnicodeString &caption )
{
    float fontSize = 18;

	// 1.9 too big
	// 1.7 is good
	// 1.5 too small

	LList <UnicodeString *> *wrappedLines = WordWrapText( caption, m_screenW, fontSize / 1.7f );

	for( int i = 0; i < wrappedLines->Size(); i++ )
	{
		UnicodeString *line = wrappedLines->GetData( i );

		float x = m_screenX + m_screenW/2.0f;
		float y = m_screenY + m_screenH + m_captionH - 12;

		y -= fontSize * ( wrappedLines->Size() - 1 - i );

		glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
		g_editorFont.SetRenderOutline(true);
		g_editorFont.DrawText2DCentre( x, y, fontSize, *line );

		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		g_editorFont.SetRenderOutline(false);
		g_editorFont.DrawText2DCentre( x, y, fontSize, *line );
	}

	wrappedLines->EmptyAndDelete();
	delete wrappedLines;
}



