#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/filesys_utils.h"
#include "lib/preferences.h"

#include "interface/demoend_window.h"

#include "app.h"
#include "main.h"
#include "renderer.h"
#include "demoendsequence.h"
#include "user_input.h"
#include "location.h"
#include "level_file.h"
#include "global_world.h"


DemoEndSequence::DemoEndSequence()
{
    m_timer = GetHighResTime();
    m_newDarwinianTimer = 20.0f;
    m_endDialogCreated = false;


    //
    // Delete the saved mission file

    char *missionFilename = g_app->m_location->m_levelFile->m_missionFilename;
    char saveFilename[256];
    sprintf( saveFilename, "%susers/%s/%s", g_app->GetProfileDirectory(), g_app->m_userProfileName, missionFilename );    
    DeleteThisFile( saveFilename );

    //
    // Delete the game file 

    sprintf( saveFilename, "%susers/%s/game_demo2.txt", g_app->GetProfileDirectory(), g_app->m_userProfileName );
    DeleteThisFile( saveFilename );

    g_app->m_levelReset = true;

    g_prefsManager->SetInt("PrologueComplete", 1 );
	g_prefsManager->SetInt("CurrentGameMode", 1 );
    g_prefsManager->Save();
    
}

DemoEndSequence::~DemoEndSequence()
{
    m_darwinians.EmptyAndDelete();
}

void DemoEndSequence::Render()
{
    float screenRatio = (float) g_app->m_renderer->ScreenH() / (float) g_app->m_renderer->ScreenW();
    int screenH = 800 * screenRatio;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 800, screenH, 0);
	glMatrixMode(GL_MODELVIEW);


    float timeSinceStart = GetHighResTime() - m_timer;
    float alpha = timeSinceStart * 0.2f;
    alpha = min( alpha, 1.0f );

    glColor4f( 1.0f, 1.0f, 1.0f, alpha );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );


    //
    // White out the background
    
    glBegin( GL_QUADS );
        glVertex2i( 0, 0 );
        glVertex2i( 800, 0 );
        glVertex2i( 800, screenH );
        glVertex2i( 0, screenH );
    glEnd();


    //
    // Create new Darwinians

    m_newDarwinianTimer -= g_advanceTime;
    if( m_newDarwinianTimer < 0.0f )
    {
        DemoEndDarwinian *darwinian = new DemoEndDarwinian();
        darwinian->m_pos.Set( 850, 0, frand(screenH) );
        darwinian->m_up.Set( 0, 0, 1 );
        darwinian->m_vel.Set( -10-frand(10), 0, sfrand(10) );
        darwinian->m_turnRate = 0.3f+frand(0.3f);
        if( sfrand() > 0.0f ) darwinian->m_turnRate *= -1.0f;
        darwinian->m_size = 50 + sfrand(25);
        m_darwinians.PutData( darwinian );
        
        m_newDarwinianTimer = 3.0f;
    }


    //
    // Advance and Render Darwinians

    glEnable( GL_TEXTURE_2D );
        
    for( int i = 0; i < m_darwinians.Size(); ++i )
    {
        DemoEndDarwinian *darwinian = m_darwinians[i];
        darwinian->m_pos += darwinian->m_vel * g_advanceTime;
        if( darwinian->m_pos.x < -100 )
        {
            m_darwinians.RemoveData(i);
            delete darwinian;
            --i;
            continue;
        }
        
        darwinian->m_up.RotateAroundY( darwinian->m_turnRate * g_advanceTime );
        
        float size = darwinian->m_size;
        Vector3 right = darwinian->m_up ^ Vector3(0,1,0);
        Vector3 topLeft = darwinian->m_pos - darwinian->m_up * size/2 - right * size/2;
        Vector3 topRight = darwinian->m_pos - darwinian->m_up * size/2 + right * size/2;
        Vector3 bottomRight = darwinian->m_pos + darwinian->m_up * size/2 + right * size/2;
        Vector3 bottomLeft = darwinian->m_pos + darwinian->m_up * size/2 - right * size/2;
       
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/ghost.bmp" ) );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );  
        glColor4f( 0.8f, 0.3f, 0.8f, 0.0f );
       
        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2f( topLeft.x, topLeft.z );
            glTexCoord2i(1,1);      glVertex2f( topRight.x, topRight.z );
            glTexCoord2i(1,0);      glVertex2f( bottomRight.x, bottomRight.z );
            glTexCoord2i(0,0);      glVertex2f( bottomLeft.x, bottomLeft.z );
        glEnd();

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glColor4f( 0.3f, 1.0f, 0.3f, alpha );

        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glColor4f( 0.3f, 1.0f, 0.3f, 0.1f );
       
        size *= 0.7f;
        topLeft = darwinian->m_pos - darwinian->m_up * size/2 - right * size/2;
        topRight = darwinian->m_pos - darwinian->m_up * size/2 + right * size/2;
        bottomRight = darwinian->m_pos + darwinian->m_up * size/2 + right * size/2;
        bottomLeft = darwinian->m_pos + darwinian->m_up * size/2 - right * size/2;

        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2f( topLeft.x, topLeft.z );
            glTexCoord2i(1,1);      glVertex2f( topRight.x, topRight.z );
            glTexCoord2i(1,0);      glVertex2f( bottomRight.x, bottomRight.z );
            glTexCoord2i(0,0);      glVertex2f( bottomLeft.x, bottomLeft.z );
        glEnd();

    }

    glDisable( GL_TEXTURE_2D );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    //
    // Bring up end dialog

	if (g_app->HasBoughtGame()) 
	{
		if( timeSinceStart > 30 )
		{
			g_app->LoadCampaign();
		}
	}
	else {
		if( timeSinceStart > 40 && !m_endDialogCreated )
		{
			EclRegisterWindow( new DemoEndWindow(15.0f, false) );
			m_endDialogCreated = true;        
		}
	}

    g_app->m_renderer->SetupMatricesFor3D();
}
