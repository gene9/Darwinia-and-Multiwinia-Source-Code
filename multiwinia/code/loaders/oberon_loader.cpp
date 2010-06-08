#include "lib/universal_include.h"

#ifdef OBERONBUILD
#include "lib/input/input.h"
#include "lib/window_manager.h"
#include "lib/text_renderer.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/language_table.h"

#include "loaders/oberon_loader.h"

#include "app.h"


OberonLoader::OberonLoader()
:   Loader()
{
}


OberonLoader::~OberonLoader()
{
}


void OberonLoader::Run()
{
	const char *oberonTextureName = "textures/msn_oberon_combosplash.bmp";
	const char *ivTextureName = "textures/ivlogo.bmp";

	if (!g_app->m_resource->DoesTextureExist( oberonTextureName ) ||
		!g_app->m_resource->DoesTextureExist( ivTextureName ))		
		return;

    int firstLogo = g_app->m_resource->GetTexture( oberonTextureName );
	int secondLogo = g_app->m_resource->GetTexture( ivTextureName );


	glEnable        ( GL_TEXTURE_2D );
	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable        ( GL_BLEND );

	SetupFor2D( 800 );

	double startTime = GetHighResTime();
    
    while( true )
    {
        if( g_app->m_requestQuit ) break;

		float timePassed = GetHighResTime() - startTime;

		float alpha1, alpha2;

		if( timePassed < 1.0f ) {
			// Fade up
			alpha1 = timePassed;
			alpha2 = 0.0f;
		}
		else if( 1.0f <= timePassed && timePassed < 2.5f ) {
			// Display First Logo
			alpha1 = 1.0f;
			alpha2 = 0.0f;
		}
		else if ( 2.5f <= timePassed && timePassed < 3.5f ) {
			// Alpha transition
			alpha2 = timePassed - 2.5f;
			alpha1 = 1.0f - alpha2;
		}
		else if ( 3.5f <= timePassed && timePassed < 5.0f ) {
			// Display Second Logo
			alpha1 = 0.0f;
			alpha2 = 1.0f;
		}
		else if ( 5.0f <= timePassed && timePassed < 6.5f ) {
			// Fade out
			alpha1 = 0.0f;
			alpha2 = 1.0f - (timePassed - 5.0f) / 1.5f;
		}
		else {
			// Finish up
			break;
		}

        //
        // Bitmap

        float bitmapW = 800;
        float bitmapH = 600;
        float bitmapX = 0;
        float bitmapY = 0;

        glColor4f		( 1.0f, 1.0f, 1.0f, alpha1 );
		glBindTexture   ( GL_TEXTURE_2D, firstLogo );
        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2i(bitmapX, bitmapY);
            glTexCoord2i(1,1);      glVertex2i(bitmapX+bitmapW, bitmapY);
            glTexCoord2i(1,0);      glVertex2i(bitmapX+bitmapW, bitmapY+bitmapH);
            glTexCoord2i(0,0);      glVertex2i(bitmapX, bitmapY+bitmapH);		 
        glEnd();

		bitmapW = 400;
		bitmapH = 400;
		bitmapX = 200;
		bitmapY = 100;

		glColor4f		( 1.0f, 1.0f, 1.0f, alpha2 );
		glDisable		( GL_TEXTURE_2D );
		glBegin( GL_QUADS );
            glVertex2i(0, 0);
            glVertex2i(800, 0);
            glVertex2i(800, bitmapY);
            glVertex2i(0, bitmapY);		 

            glVertex2i(0, bitmapY + bitmapH);
            glVertex2i(800, bitmapY + bitmapH);
            glVertex2i(800, 600);
            glVertex2i(0, 600);		 

            glVertex2i(0, bitmapY);
            glVertex2i(bitmapX, bitmapY);
            glVertex2i(bitmapX, bitmapY+bitmapH);
            glVertex2i(0, bitmapY+bitmapH);		 

            glVertex2i(bitmapX+bitmapW, bitmapY);
            glVertex2i(800, bitmapY);
            glVertex2i(800, bitmapY+bitmapH);
            glVertex2i(bitmapX+bitmapW, bitmapY+bitmapH);		 
        glEnd();
		
		glBindTexture	( GL_TEXTURE_2D, secondLogo );
		glEnable		( GL_TEXTURE_2D );
		glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2i(bitmapX, bitmapY);
            glTexCoord2i(1,1);      glVertex2i(bitmapX+bitmapW, bitmapY);
            glTexCoord2i(1,0);      glVertex2i(bitmapX+bitmapW, bitmapY+bitmapH);
            glTexCoord2i(0,0);      glVertex2i(bitmapX, bitmapY+bitmapH);		 
        glEnd();

        //
        // All done

        FlipBuffers();
    }

    glDisable       ( GL_TEXTURE_2D );
}
#endif