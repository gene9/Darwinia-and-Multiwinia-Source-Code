#include "lib/universal_include.h"

#include <math.h>

#include "lib/resource.h"
#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "lib/profiler.h"

#include "app.h"
#include "globals.h"
#include "clouds.h"
#include "location.h"
#include "main.h"

#include "lib/math/random_number.h"


Clouds::Clouds()
: m_faceTime(-1.0f)
{
    m_offset.Set(0.0,0.0,0.0);
    m_vel.Set(0.04,0.0,0.0);
}

 
void Clouds::Advance()
{    
    m_vel.Set(0.03,0.0,-0.01);
    m_offset += m_vel * SERVER_ADVANCE_PERIOD;
}


void Clouds::Render(float _predictionTime)
{    
    START_PROFILE( "RenderSky" );
	RenderSky();
    END_PROFILE(  "RenderSky" );
	
    START_PROFILE( "RenderBlobby" );
    RenderBlobby(_predictionTime);
    END_PROFILE(  "RenderBlobby" );

    START_PROFILE( "RenderFlat" );
    RenderFlat(_predictionTime);
    END_PROFILE(  "RenderFlat" );

	if( m_faceTime > 0.0f && m_faceTime + 5.0f < GetHighResTime() )
	{
		RenderFace();
		if( m_faceTime + 30.0f < GetHighResTime() )
		{
			m_faceTime = -1.0f;
		}
	}
}

void Clouds::RenderFace()
{
	//glEnable		(GL_BLEND);
    glBlendFunc		(GL_SRC_ALPHA, GL_ONE);

	float height = 800.0f;
	float gridPlace = g_app->m_location->m_landscape.GetWorldSizeX()/2;
	float faceSize = 800.0f;
	float alpha = min((GetHighResTime() - m_faceTime - 5.0), 1.0);
	if( m_faceTime + 28.0f < GetHighResTime() )
	{
		alpha = min((GetHighResTime() - m_faceTime - 28.0), 1.0);
		AppDebugOut("alpha2 = %f\n", alpha);
	}
	else
	{
		AppDebugOut("alpha = %f\n", alpha);
	}
    
    char filename[256];
    static int previousPicIndex = 1;
    static int picIndex = 1;
    static float picTimer = 0.0f;
    static float zoomFactor = 0.0f;
    static float currentZoomFactor = 0.0f;
    static float offsetX = 0.0f;
    static float currentOffsetX = 0.0f;
    float frameTime = 0.4f;

    float timeNow = GetHighResTime();

    //
    // Time to chose a new pic?

    if( timeNow > picTimer + frameTime && alpha > 0.9f )
    {
        previousPicIndex = picIndex;
        
        if( frand(6.0f) < 1.0f )        zoomFactor = frand(0.1f);        
        if( frand(6.0f) < 1.0f )        offsetX = sfrand(0.1f);
        
        if      ( frand(10.0f) < 1.0f ) picIndex = 5;           // mouth open
        else if ( frand(10.0f) < 1.0f ) picIndex = 4;           // eyes shut
        else                            picIndex = 1 + ( AppRandom() % 3 );       
            
        picTimer = timeNow;            
    }

    glEnable        ( GL_TEXTURE_2D );
    glDepthMask     ( false );


    float texX = 0;
    float texY = 0;
    float texW = 1;
    float texH = 1;

    currentZoomFactor = currentZoomFactor * ( 1.0f-(g_advanceTime*4) ) + (zoomFactor*g_advanceTime*4);
    currentOffsetX = currentOffsetX * ( 1.0f-(g_advanceTime*4) ) + (offsetX*g_advanceTime*4);
    currentOffsetX = min( currentOffsetX, currentZoomFactor );
    currentOffsetX = max( currentOffsetX, -currentZoomFactor );

    texX += currentZoomFactor;
    texX += currentOffsetX;
    texY += currentZoomFactor;
    texW -= currentZoomFactor * 2;
    texH -= currentZoomFactor * 2;
    
    //
    // Render current pic

    sprintf( filename, "sepulveda/sepulveda%d.bmp", picIndex );   
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( filename, true, false ) );
    
    glColor4f( 1.0f, 1.0f, 1.0f, alpha );
    glBegin( GL_QUADS );                
        glTexCoord2f( texX, texY+texH );        glVertex3f(gridPlace+faceSize, height, gridPlace-faceSize);
        glTexCoord2f( texX+texW, texY+texH );   glVertex3f(gridPlace+faceSize, height, gridPlace+faceSize);
        glTexCoord2f( texX+texW, texY );        glVertex3f(gridPlace-faceSize, height, gridPlace+faceSize);
        glTexCoord2f( texX, texY );             glVertex3f(gridPlace-faceSize, height, gridPlace-faceSize);                     
    glEnd();


    //
    // Render previous pic if we are fading

    if( timeNow < picTimer + frameTime && alpha > 0.75f )
    {
        float previousAlpha = alpha;
        previousAlpha *= ( 1.0f - ( timeNow - picTimer ) / frameTime );        
        sprintf( filename, "sepulveda/sepulveda%d.bmp", previousPicIndex );   
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( filename ) );

        previousAlpha *= 0.75f;
        
        glColor4f( 1.0f, 1.0f, 1.0f, previousAlpha );
        glBegin( GL_QUADS );                
            glTexCoord2f( texX, texY+texH );        glVertex3f(gridPlace+faceSize, height, gridPlace-faceSize);
            glTexCoord2f( texX+texW, texY+texH );   glVertex3f(gridPlace+faceSize, height, gridPlace+faceSize);
            glTexCoord2f( texX+texW, texY );        glVertex3f(gridPlace-faceSize, height, gridPlace+faceSize);
            glTexCoord2f( texX, texY );             glVertex3f(gridPlace-faceSize, height, gridPlace-faceSize);                    
        glEnd();
    }

    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
	glDisable		( GL_BLEND);
	glBlendFunc		( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	/*float height = 800.0f;
	float gridPlace = g_app->m_location->m_landscape.GetWorldSizeX()/2;
	float faceSize = 800.0f;
	float alpha = min((GetHighResTime() - m_faceTime - 5.0f), 1.0f);
	if( m_faceTime + 28.0f < GetHighResTime() )
	{
		alpha = min((GetHighResTime() - m_faceTime - 28.0f), 1.0f);
		AppDebugOut("alpha2 = %f\n", alpha);
	}
	else
	{
		AppDebugOut("alpha = %f\n", alpha);
	}
	char temp[256];

	sprintf(temp, "sepulveda/sepulveda%d.bmp", ((int)(GetHighResTime()))%4 + 1);

	glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( temp ) );	
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ); 

	glColor4f(1.0f,1.0f,1.0f, alpha);

	glBegin(GL_QUADS);
		glTexCoord2f(1, 0);
		glVertex3f(gridPlace+faceSize, height, gridPlace-faceSize);

		glTexCoord2f(1, 1);
		glVertex3f(gridPlace+faceSize, height, gridPlace+faceSize);

		glTexCoord2f(0, 1);
		glVertex3f(gridPlace-faceSize, height, gridPlace+faceSize);

		glTexCoord2f(0, 0);
		glVertex3f(gridPlace-faceSize, height, gridPlace-faceSize);
	glEnd();*/
}

// Renders a textured quad by spliting it up into a regular grid of smaller quads.
// This is needed to prevent fog artifacts on Radeon cards.
void Clouds::RenderQuad(float posNorth, float posSouth, float posEast, float posWest, float height,
						float texNorth, float texSouth, float texEast, float texWest)
{
	int const steps = 4;

    float sizeX = posWest - posEast;
    float sizeZ = posSouth - posNorth;
	float texSizeX = texWest - texEast;
	float texSizeZ = texSouth - texNorth;
	float posStepX = sizeX / (float)steps;
	float posStepZ = sizeZ / (float)steps;
	float texStepX = texSizeX / (float)steps;
	float texStepZ = texSizeZ / (float)steps;

    glBegin(GL_QUADS);
		for (int j = 0; j < steps; ++j)
		{
			float pz = posNorth + j * posStepZ;
			float tz = texNorth + j * texStepZ;

			for (int i = 0; i < steps; ++i)
			{
				float px = posEast + i * posStepX;
				float tx = texEast + i * texStepX;

				glTexCoord2f(tx + texStepX, tz);
				glVertex3f(px + posStepX, height, pz);

				glTexCoord2f(tx + texStepX, tz + texStepZ);
				glVertex3f(px + posStepX, height, pz + posStepZ);
				
				glTexCoord2f(tx, tz + texStepZ);
				glVertex3f(px, height, pz + posStepZ);

				glTexCoord2f(tx, tz);
				glVertex3f(px, height, pz);
			}
		}
	glEnd();
}


void Clouds::RenderFlat( float _predictionTime )
{
    float r         = 8.0;
    float height    = 1200.0;
    float xStart    = -1000.0*r;
    float xEnd      = 1000.0 + 1000.0*r;
    float zStart    = -1000.0*r;
    float zEnd      = 1000.0 + 1000.0*r;
    float detail    = 9.0;

    int cloudDetail = g_prefsManager->GetInt( "RenderCloudDetail", 1 );
    
    Vector3 offset  = m_offset + m_vel * _predictionTime;

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/clouds.bmp" ) );	
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );    
    
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_BLEND );
    glDepthMask     ( false );
    glDisable		( GL_DEPTH_TEST );
    
    float fogColor  [4] = { 0.0, 0.0, 0.0, 0.0 };
    glFogfv         ( GL_FOG_COLOR, fogColor );
    glFogf          ( GL_FOG_START, 2000.0 );
    glFogf          ( GL_FOG_END, 5000.0 );
    glEnable        ( GL_FOG );

    glColor4f       ( 0.7, 0.7, 0.9, 0.3 );

    if( cloudDetail == 3 )
    {
        glColor4f( 0.7, 0.7, 0.9, 0.5 );
    }

    RenderQuad(zStart, zEnd, xStart, xEnd, height,
			   offset.z, offset.z + detail, offset.x, offset.x + detail);
    
    detail          *= 0.5;
    height          -= 200.0;

    if( cloudDetail == 1 )
    {
	    RenderQuad(zStart, zEnd, xStart, xEnd, height,
			       offset.x/2, offset.x/2 + detail, offset.x/2, offset.x/2 + detail);
    }
    
    if( g_app->m_location ) g_app->m_location->SetupFog();
    glDepthMask     ( true );
    glEnable		( GL_DEPTH_TEST );
    glDisable       ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_TEXTURE_2D );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );    

}

void Clouds::RenderBlobby( float _predictionTime )
{
    float r         = 8.0;
    float height    = 1200.0;
    float xStart    = -1000.0*r;
    float xEnd      = 1000.0 + 1000.0*r;
    float zStart    = -1000.0*r;
    float zEnd      = 1000.0 + 1000.0*r;
    float detail    = 9.0;

    Vector3 offset  = m_offset + m_vel * _predictionTime;

    int cloudDetail = g_prefsManager->GetInt( "RenderCloudDetail", 1 );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/clouds.bmp", false ) );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );    
        
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_BLEND );
    glDisable		( GL_DEPTH_TEST );
	glDepthMask     ( false );

    float fogColor  [4] = { 0.0, 0.0, 0.0, 0.0 };
    glFogfv         ( GL_FOG_COLOR, fogColor );
    glFogf          ( GL_FOG_START, 2000.0 );
    glFogf          ( GL_FOG_END, 5000.0 );
    glEnable        ( GL_FOG );

    glColor4f       ( 0.7, 0.7, 0.9, 0.6 );

    if( cloudDetail == 1 || cloudDetail == 2 )
    {
        RenderQuad		(zStart, zEnd, xStart, xEnd, height,
	    				 offset.z, detail + offset.z, offset.x, detail + offset.x);
    }
    
    detail          *= 0.5;
    height          -= 200.0;
	
    RenderQuad		(zStart, zEnd, xStart, xEnd, height,
					 offset.x/2, detail + offset.x/2, offset.x/2, detail + offset.x/2);
    
    detail          *= 0.4;
    height          -= 200.0;

    if( cloudDetail == 1 )
    {
	    RenderQuad		(zStart, zEnd, xStart, xEnd, height,
					     offset.x, detail + offset.x, offset.z, detail + offset.z);
    }

    if( g_app->m_location ) g_app->m_location->SetupFog();
    glEnable		( GL_DEPTH_TEST );
    glDepthMask     ( true );
    glDisable       ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_TEXTURE_2D );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );    
}

void Clouds::ShowDrSFace()
{
	m_faceTime = GetHighResTime();
}

// *** RenderSky
void Clouds::RenderSky()
{
    float r = 2.0;
    float height = 1200.0;
    float gridSize = 80.0;
    float lineWidth = 8.0;

    float xStart = -2000.0*r;
    float xEnd = 2000.0 + 2000.0*r;
    float zStart = -2000.0*r;
    float zEnd = 2000.0 + 2000.0*r;

    float fogColor[4] = { 0.0, 0.0, 0.0, 0.0 };

    glEnable		(GL_BLEND);
    glBlendFunc		(GL_SRC_ALPHA, GL_ONE);
    glDepthMask		(false);
    glFogfv         (GL_FOG_COLOR, fogColor);
    glFogf          (GL_FOG_START, 2000.0 );
    glFogf          (GL_FOG_END, 4000.0 );
    glEnable        (GL_FOG);
//    glDisable       (GL_CULL_FACE);
    glLineWidth		(1.0);
    glColor4f		(0.5, 0.5, 1.0, 0.3);

    glEnable        (GL_TEXTURE_2D);
    glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );

    glBegin( GL_QUADS );
    for( int x = xStart; x < xEnd; x += gridSize )
    {
        glTexCoord2i(0,0);      glVertex3f( x-lineWidth, height, zStart );
        glTexCoord2i(0,1);      glVertex3f( x+lineWidth, height, zStart );
        glTexCoord2i(1,1);      glVertex3f( x+lineWidth, height, zEnd );
        glTexCoord2i(1,0);      glVertex3f( x-lineWidth, height, zEnd );

        glTexCoord2i(1,0);      glVertex3f( xEnd, height, x-lineWidth );
        glTexCoord2i(1,1);      glVertex3f( xEnd, height, x+lineWidth );
        glTexCoord2i(0,1);      glVertex3f( xStart, height, x+lineWidth );
        glTexCoord2i(0,0);      glVertex3f( xStart, height, x-lineWidth );
    }
    glEnd();

	glDisable       ( GL_TEXTURE_2D);
//    glEnable        ( GL_CULL_FACE);
	glDisable		( GL_BLEND);
	glBlendFunc		( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask		( true);
	
    if( g_app->m_location ) g_app->m_location->SetupFog();
}
