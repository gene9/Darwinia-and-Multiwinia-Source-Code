#include "lib/universal_include.h"

#include <math.h>

#include "lib/input/input.h"

#include "lib/3d_sprite.h"
#include "lib/avi_generator.h"
#include "lib/bitmap.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/input/win32_eventhandler.h"
#include "lib/poster_maker.h"
#include "lib/math_utils.h"
#include "lib/mouse_cursor.h"
#include "lib/ogl_extensions.h"
#include "lib/persisting_debug_render.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h" 
#include "lib/language_table.h"
#include "lib/user_info.h"
#include "lib/resource.h"
#include "lib/debug_render.h"

#include "app.h"
#include "camera.h"
#include "deform.h"
#include "explosion.h"
#include "global_world.h"
#include "helpsystem.h"
#include "landscape_renderer.h"
#include "location.h"
#include "location_editor.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "sepulveda.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "team.h"
#include "unit.h"
#include "user_input.h"
#include "water.h"
#include "water_reflection.h"
#include "gamecursor.h"
#include "startsequence.h"
#include "demoendsequence.h"
#include "tutorial.h"
#include "control_help.h"

#include "sound/soundsystem.h"

#include "eclipse.h"

#include "network/clienttoserver.h"

#include "interface/message_dialog.h"

#include "worldobject/insertion_squad.h"
#include "worldobject/virii.h"
#include "worldobject/engineer.h"


#ifdef USE_DIRECT3D
    #define USE_PIXEL_EFFECT_GRID_OPTIMISATION	1
#else
    #define USE_PIXEL_EFFECT_GRID_OPTIMISATION	1
#endif

enum 
{
	PosterMakerInactive,
	PosterMakerTiling,
	PosterMakerPixelEffect
} PosterMakerState;


Renderer::Renderer()
:   m_fps(60),
	m_displayFPS(false),
	m_displayInputMode(false),
    m_nearPlane(5.0f),
    m_farPlane(150000.0f),
    m_renderDebug(false),
	m_renderingPoster(PosterMakerInactive),
	m_tileIndex(0),
	m_fadedness(0.0f),
	m_fadeRate(0.0f),
	m_fadeDelay(0.0f),
    m_pixelSize(256),
    m_renderDarwinLogo(-1.0f)
{
}


void Renderer::Initialise()
{
	m_screenW = g_prefsManager->GetInt("ScreenWidth", 0);
	m_screenH = g_prefsManager->GetInt("ScreenHeight", 0);
	bool windowed = g_prefsManager->GetInt("ScreenWindowed", 0) ? true : false;
	int colourDepth = g_prefsManager->GetInt("ScreenColourDepth", 32);
	int refreshRate = g_prefsManager->GetInt("ScreenRefresh", 75);
	int zDepth = g_prefsManager->GetInt("ScreenZDepth", 24);
	bool waitVRT = g_prefsManager->GetInt("WaitVerticalRetrace", 1);

    if( m_screenW == 0 || m_screenH == 0 )
    {
        g_windowManager->SuggestDefaultRes( &m_screenW, &m_screenH, &refreshRate, &colourDepth );
        g_prefsManager->SetInt( "ScreenWidth", m_screenW );
        g_prefsManager->SetInt( "ScreenHeight", m_screenH );
        g_prefsManager->SetInt( "ScreenRefresh", refreshRate );
        g_prefsManager->SetInt( "ScreenColourDepth", colourDepth );
    }
    
    bool success = g_windowManager->CreateWin(m_screenW, m_screenH, windowed, colourDepth, refreshRate, zDepth, waitVRT);

    if( !success )
    {
        char caption[512];
        sprintf( caption, "Failed to set requested screen resolution of\n"
                          "%d x %d, %d bit colour, %s\n\n"
                          "Restored to safety settings of\n"
                          "640 x 480, 16 bit colour, windowed",
                          m_screenW, m_screenH, colourDepth, windowed ? "windowed" : "fullscreen" );
        MessageDialog *dialog = new MessageDialog( "Error", caption );
        EclRegisterWindow( dialog );
        dialog->m_x = 100;
        dialog->m_y = 100;


        // Go for safety values
        m_screenW = 640;
        m_screenH = 480;
        windowed = 1;
        colourDepth = 16;
        zDepth = 16;
        refreshRate = 60;

        success = g_windowManager->CreateWin(m_screenW, m_screenH, windowed, colourDepth, refreshRate, zDepth, waitVRT);
		if(!success)
		{
			// next try with 24bit z (colour depth is automatic in windowed mode)
			zDepth = 24;
			success = g_windowManager->CreateWin(m_screenW, m_screenH, windowed, colourDepth, refreshRate, zDepth, waitVRT);
		}
        DarwiniaReleaseAssert( success, "Failed to set screen mode" );

        g_prefsManager->SetInt( "ScreenWidth", m_screenW );
        g_prefsManager->SetInt( "ScreenHeight", m_screenH );
        g_prefsManager->SetInt( "ScreenWindowed", 1 );
        g_prefsManager->SetInt( "ScreenColourDepth", colourDepth );
        g_prefsManager->SetInt( "ScreenRefresh", 60 );
        g_prefsManager->Save();
    }

	InitialiseOGLExtensions();

	BuildOpenGlState();
}

void Renderer::Restart()
{
	BuildOpenGlState();
}


void Renderer::BuildOpenGlState()
{
    glGenTextures( 1, &m_pixelEffectTexId );
}


void Renderer::RenderFlatTexture()
{
	glColor3ubv(g_colourWhite.GetData());
	glEnable(GL_TEXTURE_2D);
	int textureId = g_app->m_resource->GetTexture("textures/privatedemo.bmp", true, true);
    if( textureId == -1 ) return;
    glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

	float size = m_nearPlane * 0.3f;
	Vector3 up = g_app->m_camera->GetUp() * 1.0f * size;
	Vector3 right = g_app->m_camera->GetRight() * 1.0f * size;
	Vector3 pos = g_app->m_camera->GetPos() + g_app->m_camera->GetFront() * m_nearPlane * 1.01f;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_ALPHA_TEST );
    glAlphaFunc     ( GL_GREATER, 0.02f );

	glBegin(GL_QUADS);
		glTexCoord2f(1.0f, 1.0f);	glVertex3fv( (pos + up - right).GetData() );
		glTexCoord2f(0.0f, 1.0f);	glVertex3fv( (pos + up + right).GetData() );
		glTexCoord2f(0.0f, 0.0f);	glVertex3fv( (pos - up + right).GetData() );
		glTexCoord2f(1.0f, 0.0f);	glVertex3fv( (pos - up - right).GetData() );
	glEnd();

	glAlphaFunc		( GL_GREATER, 0.01);
    glDisable( GL_ALPHA_TEST );
    glDisable( GL_BLEND );
    
	glDisable(GL_TEXTURE_2D);

	glLineWidth(1.0f);
	glBegin(GL_LINE_LOOP);
		glVertex3fv( (pos + up - right).GetData() );
		glVertex3fv( (pos + up + right).GetData() );
		glVertex3fv( (pos - up + right).GetData() );
		glVertex3fv( (pos - up - right).GetData() );
	glEnd();
}


void Renderer::RenderLogo()
{
	glColor3ubv(g_colourWhite.GetData());
	glEnable(GL_BLEND);
	glDepthMask(false);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4ub(0,0,0, 200);
    float logoW = 200;
    float logoH = 35;
	glBegin(GL_QUADS);
		glVertex2f(m_screenW - logoW - 10, m_screenH - logoH - 10);
		glVertex2f(m_screenW - 10, m_screenH - logoH - 10 );
		glVertex2f(m_screenW - 10, m_screenH - 10 );
		glVertex2f(m_screenW - logoW - 10, m_screenH - 10 );
	glEnd();

	glColor4ub(255, 255, 255, 255);
	glEnable(GL_TEXTURE_2D);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	int textureId = g_app->m_resource->GetTexture("textures/privatedemo.bmp", true, false);
    if( textureId == -1 ) return;
    glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f);	glVertex2f(m_screenW - logoW - 10, m_screenH - logoH - 10);
		glTexCoord2f(1.0f, 1.0f);	glVertex2f(m_screenW - 10, m_screenH - logoH - 10 );
		glTexCoord2f(1.0f, 0.0f);	glVertex2f(m_screenW - 10, m_screenH - 10 );
		glTexCoord2f(0.0f, 0.0f);	glVertex2f(m_screenW - logoW - 10, m_screenH - 10 );
	glEnd();

	glDepthMask(true);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    glColor4f( 1.0f, 0.75f, 0.75f, 1.0f );
	g_gameFont.DrawText2D(20, m_screenH - 70, 25, LANGUAGEPHRASE("privatedemo1") );
    g_gameFont.DrawText2D(20, m_screenH - 40, 25, LANGUAGEPHRASE("privatedemo2") );
    g_gameFont.DrawText2D(20, m_screenH - 10, 10, LANGUAGEPHRASE("privatedemo2") );
}


void Renderer::Render()
{
#ifdef PROFILER_ENABLED
	g_app->m_profiler->RenderStarted();
#endif

    if (g_inputManager->controlEvent( ControlCreateScreenShot ) && EclGetWindows()->Size() == 0 )
	{
#if 0
// something is broken with tile camera -> landscape disappears
// also shader effects are not supported for tiles
// so maybe it's better to use standard non-tiled render for now
// when high res is needed, i can easily add render to big texture

		// Generate the pixel effect overlay for the whole screen, with the depth
		// information already taken account of. In a moment we will use this 
		// overlay a section of this image on each tile.
		m_renderingPoster = PosterMakerPixelEffect;
		RenderFrame();

		m_renderingPoster = PosterMakerTiling;
#endif
		int posterResolution = g_prefsManager->GetInt( "RenderPosterResolution", 1 );
		PosterMaker pm(m_screenW, m_screenH);
		
		for (int y = 0; y < posterResolution; ++y)
		{
			for (int x = 0; x < posterResolution; ++x)
			{
				// renders to backbuffer
				RenderFrame(false);
				// reads from backbuffer
				pm.AddFrame();
				// flips back to front
				g_windowManager->Flip();
				++m_tileIndex;
				if (m_tileIndex == posterResolution * posterResolution)
				{
					m_tileIndex = 0;
				}
			}
		}

		pm.SavePoster();

		m_renderingPoster = PosterMakerInactive;
	}
	else
	{
		RenderFrame();
	}

#ifdef TARGET_OS_VISTA
	if( g_app->m_saveThumbnail )
	{
		g_app->SaveThumbnailScreenshot();
	}
#endif


#ifdef PROFILER_ENABLED
	g_app->m_profiler->RenderEnded();
#endif // PROFILER_ENABLED
}


bool Renderer::IsFadeComplete() const
{
	if (NearlyEquals(m_fadeRate, 0.0f)) return true;

	return false;
}


void Renderer::StartFadeOut()
{
	m_fadeDelay = 0.0f;
	m_fadeRate = 1.0f;
}


void Renderer::StartFadeIn(float _delay)
{
	m_fadedness = 1.0f;
	m_fadeDelay = _delay;
	m_fadeRate = -1.0f;
}


void Renderer::RenderFadeOut()
{
	static double lastTime = GetHighResTime();
	double timeNow = GetHighResTime();
	double timeIncrement = timeNow - lastTime;
	if (timeIncrement > 0.05f) timeIncrement = 0.05f;
	lastTime = timeNow;

	if (m_fadeDelay > 0.0f) 
	{
		m_fadeDelay -= timeIncrement;
	}
	else
	{
		m_fadedness += m_fadeRate * timeIncrement;
		if (m_fadedness < 0.0f)
		{
			m_fadedness = 0.0f;
			m_fadeRate = 0.0f;
		}
		else if (m_fadedness > 1.0f)
		{
			m_fadeRate = 0.0f;
			m_fadedness = 1.0f;
		}
	}
		
	if (m_fadedness > 0.0001f)
	{
		glEnable(GL_BLEND);
		glDepthMask(false);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		glColor4ub(0, 0, 0, (int)(m_fadedness * 255.0f)); 
		glBegin(GL_QUADS);
			glVertex2i(-1, -1);
			glVertex2i(m_screenW, -1);
			glVertex2i(m_screenW, m_screenH);
			glVertex2i(-1, m_screenH);
		glEnd();
		
		glDisable(GL_BLEND);
		glDepthMask(true);
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}


void Renderer::RenderPaused()
{
	const char *msg = "PAUSED";
	int x = g_app->m_renderer->ScreenW()/2;
	int y = g_app->m_renderer->ScreenH()/2;	
	TextRenderer &font = g_gameFont;
	
	font.BeginText2D();
	
	// Black Background
	g_gameFont.SetRenderShadow( true );
    glColor4f(0.3f,0.3f,0.3f,0.0f);
	font.DrawText2DCentre( x, y, 80, msg );
	
	// White Foreground
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	font.SetRenderShadow(false);
	font.DrawText2DCentre( x, y, 80, msg );
	
	font.EndText2D();
}

void Renderer::RenderFrame(bool withFlip)
{
	int renderPixelShaderPref = g_prefsManager->GetInt("RenderPixelShader");

    SetOpenGLState();

	if (g_app->m_locationId == -1)
	{
		m_nearPlane = 50.0f;
		m_farPlane = 10000000.0f;
	}
	else
	{
		m_nearPlane = 5.0f;
		m_farPlane = 15000.0f;
	}

	FPSMeterAdvance();
	SetupMatricesFor3D();

    START_PROFILE(g_app->m_profiler, "Render Clear");
		RGBAColour *col = &g_app->m_backgroundColour;
		if( g_app->m_location )
        {
            glClearColor(col->r/255.0f, col->g/255.0f, col->b/255.0f, col->a/255.0f);
        }
        else
        {
            glClearColor(0.05f, 0.0f, 0.05f, 0.1f);
        }
		glClear			(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	END_PROFILE(g_app->m_profiler, "Render Clear");

	bool deformStarted = false;

    if (g_app->m_editing)
	{
		if (g_app->m_locationId != -1)
		{
#ifdef LOCATION_EDITOR
			SetupMatricesFor3D();
			g_app->m_location->Render();
			g_app->m_locationEditor->Render();
#endif // LOCATION_EDITOR
		}
		else
		{
			g_app->m_globalWorld->Render();
		}
	}
	else
	{
		if (g_app->m_locationId != -1)
		{
			if( renderPixelShaderPref > 0 )
            {
				switch (m_renderingPoster)
				{
					case PosterMakerInactive:
						PreRenderPixelEffect();

#ifdef USE_DIRECT3D
						if( renderPixelShaderPref == 1 ) 
						{
							if (g_waterReflectionEffect)
							{
								g_waterReflectionEffect->PreRenderWaterReflection();
							}
							if(g_deformEffect)
							{
								deformStarted = true;
								g_deformEffect->Start();
							}
						}
#endif
						START_PROFILE(g_app->m_profiler, "Render Clear");
						glClear	(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
						END_PROFILE(g_app->m_profiler, "Render Clear");
    					g_app->m_location->Render();

						ApplyPixelEffect();

						SetupMatricesFor3D();
						break;
					case PosterMakerTiling:
						g_app->m_location->Render();
						ApplyPixelEffect();
						SetupMatricesFor3D();
						break;
					case PosterMakerPixelEffect:
						PreRenderPixelEffect();
						break;
				}
            }
            else
            {
    			g_app->m_location->Render();
            }
		}
		else
		{
			g_app->m_globalWorld->Render();
		}
	}

	CHECK_OPENGL_STATE();
    g_app->m_helpSystem->Render();
    g_app->m_controlHelpSystem->Render();
    if( g_app->m_tutorial ) g_app->m_tutorial->Render();
	g_explosionManager.Render();
    g_app->m_particleSystem->Render();
#ifdef USE_DIRECT3D
	if( renderPixelShaderPref == 1 )
	{
		if( g_deformEffect && deformStarted ) 
			g_deformEffect->Stop();
	}
#endif

    if( g_app->m_demoEndSequence ) 
    {
        g_editorFont.BeginText2D();
        g_app->m_demoEndSequence->Render();
        SetupMatricesFor3D();
        glEnable( GL_DEPTH_TEST );
    }

    g_app->m_userInput->Render();
	g_app->m_gameCursor->Render();	
	g_app->m_taskManagerInterface->Render();
    g_app->m_camera->Render();

#ifdef DEBUG_RENDER_ENABLED
	g_debugRenderer.Render();
#endif
	CHECK_OPENGL_STATE();
    
//	RenderFlatTexture();

	//if (m_renderingPoster == PosterMakerInactive) 
	{
	    g_app->m_sepulveda->Render();  
	}

    g_editorFont.BeginText2D();

	RenderFadeOut();

	if (m_displayFPS)
	{
		glColor4f(0,0,0,0.6);
		glBegin(GL_QUADS);
			glVertex2f(8.0, 1.0f);
			glVertex2f(70.0, 1.0f);
			glVertex2f(70.0, 15.0f);
			glVertex2f(8.0, 15.0f);
		glEnd();
	
		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		g_editorFont.DrawText2D( 12, 10, DEF_FONT_SIZE, "FPS: %d", m_fps);
//		g_editorFont.DrawText2D( 150, 10, DEF_FONT_SIZE, "TFPS: %2.0f", g_targetFrameRate);
//		Vector3 const camPos = g_app->m_camera->GetPos();
//		g_editorFont.DrawText2D( 150, 10, DEF_FONT_SIZE, "cam: %.1f, %.1f, %.1f", camPos.x, camPos.y, camPos.z);
	}


	if (m_displayInputMode) {

		glColor4f(0,0,0,0.6);
		glBegin(GL_QUADS);
			glVertex2f(80.0, 1.0f);
			glVertex2f(230.0, 1.0f);
			glVertex2f(230.0, 18.0f);
			glVertex2f(80.0, 18.0f);
		glEnd();

		std::string inmode;
		switch ( g_inputManager->getInputMode() ) {
			case INPUT_MODE_KEYBOARD:
				inmode = "keyboard"; break;
			case INPUT_MODE_GAMEPAD:
				inmode = "gamepad"; break;
			default:
				inmode = "unknown";
		}

		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		g_editorFont.DrawText2D( 84, 10, DEF_FONT_SIZE, "InputMode: %s", inmode.c_str() );

	}


	if (g_app->m_editing )
	{
        g_gameFont.DrawText2DCentre( m_screenW / 2, 10, 17, "= EDITOR ENABLED =" );
        g_gameFont.DrawText2DCentre( m_screenW /2, 27, 11, "BaseDir : %s", g_app->m_resource->GetBaseDirectory() );

        if( g_app->m_locationId != -1 )
        {
		    g_editorFont.DrawText2D( m_screenW - 300, m_screenH - 40, DEF_FONT_SIZE, "Triangles : %d", g_app->m_location->m_landscape.m_renderer->m_numTriangles);
		    g_editorFont.DrawText2D( m_screenW - 300, m_screenH - 25, DEF_FONT_SIZE, "Mission   : %s", g_app->m_requestedMission);
            g_editorFont.DrawText2D( m_screenW - 300, m_screenH - 10, DEF_FONT_SIZE, "Map       : %s", g_app->m_requestedMap);
        }
	}
    
    if( g_app->m_server )
    {
        int latency = g_app->m_server->m_sequenceId - g_lastProcessedSequenceId;
        if( latency > 10)
		{
#ifdef AVI_GENERATOR
			if (!g_app->m_aviGenerator)
#endif
			{
				glColor4f( 0.0f, 0.0f, 0.0f, 0.8f );
				glBegin( GL_QUADS );
					glVertex2f( m_screenW/2 - 200, 120 );
					glVertex2f( m_screenW/2 + 200, 120 );
					glVertex2f( m_screenW/2 + 200, 80 );
					glVertex2f( m_screenW/2 - 200, 80 );
				glEnd();    
				glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
				g_editorFont.DrawText2DCentre( m_screenW/2, 100, 20, "Client LAG %dms behind Server ", latency*100 );
			}
        }
    }

    //
    // Personalisation information

	if( !g_app->m_taskManagerInterface->m_visible &&
        !g_app->m_sepulveda->IsVisible() &&
        m_renderingPoster == PosterMakerInactive &&
        !g_app->m_camera->IsInMode( Camera::ModeSphereWorldIntro ) &&
        !g_app->m_camera->IsInMode( Camera::ModeSphereWorldOutro ) )
    {
#ifdef PROMOTIONAL_BUILD
        RenderLogo();
#endif
    }

    if( g_app->m_startSequence ) g_app->m_startSequence->Render();
        
    if( m_renderDarwinLogo >= 0.0f )
    {
        int textureId = g_app->m_resource->GetTexture( "icons/darwin_research_associates.bmp" );

        glBindTexture( GL_TEXTURE_2D, textureId );
        glEnable( GL_TEXTURE_2D );

        float y = m_screenH * 0.05f;
        float h = m_screenH * 0.7f;

        float w = h;
        float x = m_screenW/2 - w/2;

        float alpha = 0.0f;

        float timeNow = GetHighResTime();
        if( timeNow > m_renderDarwinLogo + 10 )
        {
            m_renderDarwinLogo = -1.0f;
        }
        else if( timeNow < m_renderDarwinLogo + 3 )
        {
            alpha = (timeNow - m_renderDarwinLogo) / 3.0f;
        }
        else if( timeNow > m_renderDarwinLogo + 8 )
        {
            alpha = 1.0f - (timeNow - m_renderDarwinLogo - 8) / 2.0f;
        }
        else 
        {
            alpha = 1.0f;
        }
        
        alpha = max( alpha, 0.0f );
        alpha = min( alpha, 1.0f );

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
        glEnable( GL_BLEND );
        glColor4f( alpha, alpha, alpha, 0.0f );

        for( float dx = -4; dx <= 4; dx += 4 )
        {
            for( float dy = -4; dy <= 4; dy += 4 )
            {
                glBegin( GL_QUADS );
			        glTexCoord2i(0,1);      glVertex2f( x+dx, y+dy );
			        glTexCoord2i(1,1);      glVertex2f( x+w+dx, y+dy );
			        glTexCoord2i(1,0);      glVertex2f( x+w+dx, y+h+dy );
			        glTexCoord2i(0,0);      glVertex2f( x+dx, y+h+dy );
		        glEnd();
            }
        }

        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glColor4f( 1.0f, 1.0f, 1.0f, alpha );

        glBegin( GL_QUADS );
			glTexCoord2i(0,1);      glVertex2f( x, y );
			glTexCoord2i(1,1);      glVertex2f( x+w, y );
			glTexCoord2i(1,0);      glVertex2f( x+w, y+h );
			glTexCoord2i(0,0);      glVertex2f( x, y+h );
		glEnd();

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable( GL_TEXTURE_2D );

        float textSize = m_screenH / 9.0f;
        g_gameFont.SetRenderOutline( true );
        glColor4f( alpha, alpha, alpha, 0.0f );
        g_gameFont.DrawText2DCentre( m_screenW/2, m_screenH * 0.8f, textSize, "DARWINIA" );
        g_gameFont.SetRenderOutline( false );
        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        g_gameFont.DrawText2DCentre( m_screenW/2, m_screenH * 0.8f, textSize, "DARWINIA" );
    }

    g_editorFont.EndText2D();

	if ( !g_eventHandler->WindowHasFocus() ||
        g_app->m_paused )
		RenderPaused();	

    START_PROFILE(g_app->m_profiler, "GL Flip");
	
    if(withFlip)
		g_windowManager->Flip();

    END_PROFILE(g_app->m_profiler, "GL Flip");
    
	CHECK_OPENGL_STATE();

}


int Renderer::ScreenW() const
{
    return m_screenW;
}


int Renderer::ScreenH() const
{
    return m_screenH;
}


void Renderer::SetupProjMatrixFor3D() const
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (m_renderingPoster == PosterMakerTiling)
	{
		int const screenH = ScreenH();
		int const screenW = ScreenW();
        int posterResolution = g_prefsManager->GetInt( "RenderPosterResolution", 1 );

		float scale = g_app->m_camera->GetFov() / 78.0f;
		float left = -scale * m_nearPlane;
		float top = -0.75f * scale * m_nearPlane;
		float tileWidth = (-left * 2.0f) / (float)posterResolution;
		float tileHeight = (-top * 2.0f) / (float)posterResolution;

		float y = m_tileIndex / posterResolution;
		float x = m_tileIndex % posterResolution;
		float x1 = left + x * tileWidth;
		float x2 = x1 + tileWidth;
		float y1 = top + y * tileHeight;
		float y2 = y1 + tileHeight;
		glFrustum(x1, x2, y1, y2, m_nearPlane, m_farPlane);
	}
	else
	{
	    gluPerspective(g_app->m_camera->GetFov(),
				   (float)m_screenW / (float)m_screenH, // Aspect ratio
				   m_nearPlane, m_farPlane);
	}
}


void Renderer::SetupMatricesFor3D() const
{
	Camera *camera = g_app->m_camera;

	SetupProjMatrixFor3D();
	camera->SetupModelviewMatrix();
}


void Renderer::SetupMatricesFor2D() const
{
	int v[4];
	glGetIntegerv(GL_VIEWPORT, &v[0]);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

    float left = v[0];
	float right = v[0] + v[2];
	float bottom = v[1] + v[3];
	float top = v[1];
//	glOrtho(left, right, bottom, top, -m_nearPlane, -m_farPlane);
	gluOrtho2D(left, right, bottom, top);
    glMatrixMode(GL_MODELVIEW);
}

	
void Renderer::FPSMeterAdvance()
{
    static int framesThisSecond = 0;
    static double endOfSecond = GetHighResTime() + 1.0;

    framesThisSecond++;

    double currentTime = GetHighResTime();
    if (currentTime > endOfSecond)
    {
        if (currentTime > endOfSecond + 2.0)
        {
            endOfSecond = currentTime + 1.0;
        }
        else
        {
            endOfSecond += 1.0;
        }
        m_fps = framesThisSecond;
        framesThisSecond = 0;
    }
}


float Renderer::GetNearPlane() const
{
    return m_nearPlane;
}


float Renderer::GetFarPlane() const
{
    return m_farPlane;
}


void Renderer::SetNearAndFar(float _nearPlane, float _farPlane)
{
	DarwiniaDebugAssert(_nearPlane < _farPlane);
	DarwiniaDebugAssert(_nearPlane > 0.0f);
	m_nearPlane = _nearPlane;
	m_farPlane = _farPlane;
}


int Renderer::GetGLStateInt(int pname) const
{
	int returnVal;
	glGetIntegerv(pname, &returnVal);
	return returnVal;
}


float Renderer::GetGLStateFloat(int pname) const
{
	float returnVal;
	glGetFloatv(pname, &returnVal);
	return returnVal;
}


void Renderer::CheckOpenGLState() const
{   
    return;
	int results[10];
	float resultsf[10];

	DarwiniaDebugAssert(glGetError() == GL_NO_ERROR);

	// Geometry
//	DarwiniaDebugAssert(glIsEnabled(GL_CULL_FACE));
	DarwiniaDebugAssert(GetGLStateInt(GL_FRONT_FACE) == GL_CCW);
	glGetIntegerv(GL_POLYGON_MODE, results);
	DarwiniaDebugAssert(results[0] == GL_FILL);
	DarwiniaDebugAssert(results[1] == GL_FILL);
	DarwiniaDebugAssert(GetGLStateInt(GL_SHADE_MODEL) == GL_FLAT);
	DarwiniaDebugAssert(!glIsEnabled(GL_NORMALIZE));

	// Colour
	DarwiniaDebugAssert(!glIsEnabled(GL_COLOR_MATERIAL));
	DarwiniaDebugAssert(GetGLStateInt(GL_COLOR_MATERIAL_FACE) == GL_FRONT_AND_BACK);
	DarwiniaDebugAssert(GetGLStateInt(GL_COLOR_MATERIAL_PARAMETER) == GL_AMBIENT_AND_DIFFUSE);

	// Lighting
	DarwiniaDebugAssert(!glIsEnabled(GL_LIGHTING));

    DarwiniaDebugAssert(!glIsEnabled(GL_LIGHT0));
	DarwiniaDebugAssert(!glIsEnabled(GL_LIGHT1));
	DarwiniaDebugAssert(!glIsEnabled(GL_LIGHT2));
	DarwiniaDebugAssert(!glIsEnabled(GL_LIGHT3));
	DarwiniaDebugAssert(!glIsEnabled(GL_LIGHT4));
	DarwiniaDebugAssert(!glIsEnabled(GL_LIGHT5));
	DarwiniaDebugAssert(!glIsEnabled(GL_LIGHT6));
	DarwiniaDebugAssert(!glIsEnabled(GL_LIGHT7));

	glGetFloatv(GL_LIGHT_MODEL_AMBIENT, resultsf);
	DarwiniaDebugAssert(resultsf[0] < 0.001f &&
				resultsf[1] < 0.001f &&
				resultsf[2] < 0.001f &&
				resultsf[3] < 0.001f);
	
	if (g_app->m_location)
	{
		for (int i = 0; i < g_app->m_location->m_lights.Size(); i++)
		{
			Light *light = g_app->m_location->m_lights.GetData(i);

			float amb = 0.0f;
			GLfloat ambCol1[] = { amb, amb, amb, 1.0f };
            
			GLfloat pos1_actual[4];
			GLfloat ambient1_actual[4];
			GLfloat diffuse1_actual[4];
			GLfloat specular1_actual[4];
    
			glGetLightfv(GL_LIGHT0 + i, GL_POSITION, pos1_actual);
			glGetLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse1_actual);
			glGetLightfv(GL_LIGHT0 + i, GL_SPECULAR, specular1_actual);
			glGetLightfv(GL_LIGHT0 + i, GL_AMBIENT, ambient1_actual);

			for (int i = 0; i < 4; i++)
			{
	//			DarwiniaDebugAssert(fabsf(lightPos1[i] - pos1_actual[i]) < 0.001f);
	//			DarwiniaDebugAssert(fabsf(light->m_colour[i] - diffuse1_actual[i]) < 0.001f);
	//			DarwiniaDebugAssert(fabsf(light->m_colour[i] - specular1_actual[i]) < 0.0001f);
	//			DarwiniaDebugAssert(fabsf(ambCol1[i] - ambient1_actual[i]) < 0.001f);
			}
		}
	}

	// Blending, Anti-aliasing, Fog and Polygon Offset
//	DarwiniaDebugAssert(!glIsEnabled(GL_BLEND));
	DarwiniaDebugAssert(GetGLStateInt(GL_BLEND_DST) == GL_ONE_MINUS_SRC_ALPHA);
	DarwiniaDebugAssert(GetGLStateInt(GL_BLEND_SRC) == GL_SRC_ALPHA);
	DarwiniaDebugAssert(!glIsEnabled(GL_ALPHA_TEST));
	DarwiniaDebugAssert(GetGLStateInt(GL_ALPHA_TEST_FUNC) == GL_GREATER);
	DarwiniaDebugAssert(GetGLStateFloat(GL_ALPHA_TEST_REF) == 0.01f);
	DarwiniaDebugAssert(!glIsEnabled(GL_FOG));
	DarwiniaDebugAssert(GetGLStateFloat(GL_FOG_DENSITY) == 1.0f);
	DarwiniaDebugAssert(GetGLStateFloat(GL_FOG_END) >= 4000.0f);
	//DarwiniaDebugAssert(GetGLStateFloat(GL_FOG_START) >= 1000.0f);
	glGetFloatv(GL_FOG_COLOR, resultsf);
//	DarwiniaDebugAssert(fabsf(resultsf[0] - g_app->m_location->m_backgroundColour.r/255.0f) < 0.001f);
//	DarwiniaDebugAssert(fabsf(resultsf[1] - g_app->m_location->m_backgroundColour.g/255.0f) < 0.001f);
//	DarwiniaDebugAssert(fabsf(resultsf[2] - g_app->m_location->m_backgroundColour.b/255.0f) < 0.001f);
//	DarwiniaDebugAssert(fabsf(resultsf[3] - g_app->m_location->m_backgroundColour.a/255.0f) < 0.001f);
	DarwiniaDebugAssert(GetGLStateInt(GL_FOG_MODE) == GL_LINEAR);
	DarwiniaDebugAssert(!glIsEnabled(GL_LINE_SMOOTH));
	DarwiniaDebugAssert(!glIsEnabled(GL_POINT_SMOOTH));
	
	// Texture Mapping
	DarwiniaDebugAssert(!glIsEnabled(GL_TEXTURE_2D));
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, results);
	DarwiniaDebugAssert(results[0] == GL_CLAMP);
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, results);
	DarwiniaDebugAssert(results[0] == GL_CLAMP);
	glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, results);
	DarwiniaDebugAssert(results[0] == GL_MODULATE);
	glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, results);
	DarwiniaDebugAssert(results[0] == 0);
	DarwiniaDebugAssert(results[1] == 0);
	DarwiniaDebugAssert(results[2] == 0);
	DarwiniaDebugAssert(results[3] == 0);

	// Frame Buffer
	DarwiniaDebugAssert(glIsEnabled(GL_DEPTH_TEST));
	DarwiniaDebugAssert(GetGLStateInt(GL_DEPTH_WRITEMASK) != 0);
	DarwiniaDebugAssert(GetGLStateInt(GL_DEPTH_FUNC) == GL_LEQUAL);
	DarwiniaDebugAssert(glIsEnabled(GL_SCISSOR_TEST) == 0);

	// Hints
	DarwiniaDebugAssert(GetGLStateInt(GL_FOG_HINT) == GL_DONT_CARE);
	DarwiniaDebugAssert(GetGLStateInt(GL_POLYGON_SMOOTH_HINT) == GL_DONT_CARE);
}


void Renderer::SetOpenGLState() const
{
	// Geometry
	glEnable		(GL_CULL_FACE);
	glFrontFace		(GL_CCW);
	glPolygonMode	(GL_FRONT, GL_FILL);
	glPolygonMode	(GL_BACK, GL_FILL);
	glShadeModel	(GL_FLAT);
	glDisable		(GL_NORMALIZE);

	// Colour
	glDisable		(GL_COLOR_MATERIAL);
	glColorMaterial	(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// Lighting
	glDisable		(GL_LIGHTING);
	glDisable		(GL_LIGHT0);
	glDisable		(GL_LIGHT1);
	glDisable		(GL_LIGHT2);
	glDisable		(GL_LIGHT3);
	glDisable		(GL_LIGHT4);
	glDisable		(GL_LIGHT5);
	glDisable		(GL_LIGHT6);
	glDisable		(GL_LIGHT7);
	if (g_app->m_location)		g_app->m_location->SetupLights();
	else						g_app->m_globalWorld->SetupLights();
	float ambient[] = { 0, 0, 0, 0 };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

	// Blending, Anti-aliasing, Fog and Polygon Offset
	glDisable		(GL_BLEND);
	glBlendFunc		(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable		(GL_ALPHA_TEST);
	glAlphaFunc		(GL_GREATER, 0.01);
	if (g_app->m_location)		g_app->m_location->SetupFog();
	else						g_app->m_globalWorld->SetupFog();
	glDisable		(GL_LINE_SMOOTH);
	glDisable		(GL_POINT_SMOOTH);
	
	// Texture Mapping
	glDisable		(GL_TEXTURE_2D);
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexEnvi		(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	int colour[4] = { 0, 0, 0, 0 };
	glTexEnviv		(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, colour);

	// Frame Buffer
	glEnable		(GL_DEPTH_TEST);
	glDepthMask		(true);
	glDepthFunc		(GL_LEQUAL);
//	glStencilMask	(0x00);
//	glScissor		(400, 100, 400, 400);
	glDisable		(GL_SCISSOR_TEST);

	// Hints
	glHint			(GL_FOG_HINT, GL_DONT_CARE);
	glHint			(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);    
}


void Renderer::SetObjectLighting() const
{
	float spec = 0.0f;
	float diffuse = 1.0f;
    float amb = 0.0f;
	GLfloat materialShininess[] = { 127.0f };
	GLfloat materialSpecular[] = { spec, spec, spec, 0.0f };
   	GLfloat materialDiffuse[] = { diffuse, diffuse, diffuse, 1.0f };
	GLfloat ambCol[] = { amb, amb, amb, 1.0f };
   
	glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
	glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambCol);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
}


void Renderer::UnsetObjectLighting() const
{
	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHT1);
}


void Renderer::PreRenderPixelEffect()
{
    START_PROFILE(g_app->m_profiler, "Pixel Pre-render");

	UpdateTotalMatrix();


	// 
	// Reset pixel effect grid cell distances to infinity

	for (int y = 0; y < PIXEL_EFFECT_GRID_RES; ++y)
	{
		for (int x = 0; x < PIXEL_EFFECT_GRID_RES; ++x)
		{
			m_pixelEffectGrid[y][x] = 1e9;
		}
	}
	// memset(m_pixelEffectGrid, 0, sizeof(m_pixelEffectGrid));

    float timeSinceAdvance = g_predictionTime;
   
    //
    // Blend our old glow texture into place

    START_PROFILE(g_app->m_profiler, "blend old");
    glEnable            (GL_TEXTURE_2D);
    glBindTexture       (GL_TEXTURE_2D, m_pixelEffectTexId );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

    glEnable            (GL_BLEND);
    glDepthMask         (false);
        
    g_editorFont.BeginText2D();
    
    float upSpeed = 2.0f;
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    glDisable           (GL_TEXTURE_2D); // *
    glEnable( GL_BLEND );
    //glDisable( GL_BLEND ); // *
    glDisable( GL_CULL_FACE ); // *
    //glBegin( GL_QUADS );
    //    glTexCoord2f( 0.0f, 1.0f );     glVertex2f( 0, m_screenH-upSpeed );
    //    glTexCoord2f( 1.0f, 1.0f );     glVertex2f( m_pixelSize, m_screenH-upSpeed);
    //    glTexCoord2f( 1.0f, 0.0f );     glVertex2f( m_pixelSize, m_screenH - m_pixelSize -upSpeed );
    //    glTexCoord2f( 0.0f, 0.0f );     glVertex2f( 0, m_screenH - m_pixelSize - upSpeed);
    //glEnd();
    g_editorFont.EndText2D();
    glEnable           (GL_TEXTURE_2D); // *
    END_PROFILE(g_app->m_profiler, "blend old");
           
    //glDisable           (GL_TEXTURE_2D);


    //
    // Draw all pixelated objects to the screen
    // Find the nearest pixelated object and update m_pixelSize at the end

    START_PROFILE(g_app->m_profiler, "Draw pixelated");
    glViewport( 0, 0, m_pixelSize, m_pixelSize );
    float nearest = 99999.9f;

    float cutoff = 1000.0f;
    Vector3 camPos = g_app->m_camera->GetPos();

    for( int t = 0; t < NUM_TEAMS; ++t )    
    {
        if( g_app->m_location->m_teams[t].m_teamType != Team::TeamTypeUnused )
        {
            for( int i = 0; i < g_app->m_location->m_teams[t].m_units.Size(); ++i )
            {
                if( g_app->m_location->m_teams[t].m_units.ValidIndex(i) )
                {
                    Unit *unit = g_app->m_location->m_teams[t].m_units[i];
                    if( unit->m_troopType == Entity::TypeInsertionSquadie ||
                        unit->m_troopType == Entity::TypeCentipede )
                    {
                        if( unit->IsInView() )
                        {
                            float distance = ( unit->m_centrePos - camPos ).Mag();
                            if( distance < cutoff )
                            {
                                for( int j = 0; j < unit->m_entities.Size(); ++j )
                                {
                                    if( unit->m_entities.ValidIndex(j) )
                                    {
                                        Entity *entity = unit->m_entities[j];  
                                        bool rendered = false;
                                        if( j <= unit->m_entities.GetLastUpdated() )
                                        {
                                            rendered = entity->RenderPixelEffect( g_predictionTime );
                                        }
                                        else
                                        {
                                            rendered = entity->RenderPixelEffect( g_predictionTime+SERVER_ADVANCE_PERIOD );
                                        }      
                                        if( rendered )
                                        {
                                            float distance = (entity->m_pos - g_app->m_camera->GetPos()).Mag();
                                            if( distance < nearest ) nearest = distance;
                                        }                                    
                                    }
                                }
                            }
                        }
                    }
                }
            }
    
            for( int i = 0; i < g_app->m_location->m_teams[t].m_others.Size(); ++i )
            {
                if( g_app->m_location->m_teams[t].m_others.ValidIndex(i) )
                {
                    Entity *entity = g_app->m_location->m_teams[t].m_others[i];
                    if( entity->IsInView() )
                    {
                        float distance = ( entity->m_pos - camPos ).Mag();
                        if( distance < cutoff )
                        {
                            bool rendered = false;
                            if( i <= g_app->m_location->m_teams[t].m_others.GetLastUpdated() )
                            {
                                rendered = entity->RenderPixelEffect( g_predictionTime );
                            }
                            else
                            {
                                rendered = entity->RenderPixelEffect( g_predictionTime+SERVER_ADVANCE_PERIOD );
                            }
                            if( rendered )
                            {
                                float distance = (entity->m_pos - g_app->m_camera->GetPos()).Mag();
                                if( distance < nearest ) nearest = distance;
                            }
                        }
                    }
                }
            }
        }
    }
    

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            float distance = ( building->m_centrePos - camPos ).Mag();
            if( distance < cutoff )
            {
                bool rendered = building->RenderPixelEffect( g_predictionTime );
                if( rendered )
                {
                    float distance = (building->m_pos - g_app->m_camera->GetPos()).Mag();
                    if( distance < nearest ) nearest = distance;
                }
            }
        }
    }
    
    END_PROFILE(g_app->m_profiler, "Draw pixelated");
    glViewport( 0, 0, m_screenW, m_screenH );


    //
    // Copy the screen to a texture

    START_PROFILE(g_app->m_profiler, "Gen new texture");
    glEnable            (GL_TEXTURE_2D);
	glBindTexture	    (GL_TEXTURE_2D, m_pixelEffectTexId);
    
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glCopyTexImage2D    (GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, m_pixelSize, m_pixelSize, 0 );

    glDisable           (GL_TEXTURE_2D);
    glDisable           (GL_BLEND);
    glEnable            (GL_CULL_FACE);
    END_PROFILE(g_app->m_profiler, "Gen new texture");

    glDepthMask( true );

	CHECK_OPENGL_STATE();


    //
    // Update pixel size

    //if      ( nearest < 30 )        m_pixelSize = 128;    
    //else if ( nearest < 200 )       m_pixelSize = 256;
    //else                            m_pixelSize = 512;

    END_PROFILE(g_app->m_profiler, "Pixel Pre-render");
}

#ifdef USE_DIRECT3D
inline float d3dOneMinus( float _x )
{
	return 1.0f - _x;
}
#else
#define d3dOneMinus( _x ) _x
#endif

void Renderer::PaintPixels()
{
#if USE_PIXEL_EFFECT_GRID_OPTIMISATION
	double const aspectRatio = (double)m_screenW / (double)m_screenH;
	double zoomCorrection = 0.000037 * (double)g_app->m_camera->GetFov();
	double scale = (0.017 + zoomCorrection) * (double)g_app->m_camera->GetFov();
	
	double const step = scale * aspectRatio / (double)PIXEL_EFFECT_GRID_RES;
	double const xOffset = scale * (-0.5 * aspectRatio);
	double const yOffset = scale * -0.5;
	float gridToTexture = 1.0f / (float)PIXEL_EFFECT_GRID_RES;
	float gridToTextureY = gridToTexture * ((float)m_screenW / (float)m_screenH);
	float distance = 1.0f;
	double const cellsUsed = (double)PIXEL_EFFECT_GRID_RES / aspectRatio;
	int const cellsToSkip = PIXEL_EFFECT_GRID_RES - ceil(cellsUsed);

	int const yGridRes = PIXEL_EFFECT_GRID_RES - cellsToSkip;
    glBegin(GL_QUADS);
		for (int y = 0; y < yGridRes; ++y)
		{
			double y1 = (double)y * step + yOffset;
			double y2 = y1 + step;
			float ty = gridToTextureY * (float)y;

			for (int x = 0; x < PIXEL_EFFECT_GRID_RES; ++x)
			{
				if (m_pixelEffectGrid[x][y] < 1e9)
				{
					distance = m_pixelEffectGrid[x][y];
					if (distance < m_nearPlane) distance = m_nearPlane + 0.1;
					double x1 = (double)x * step + xOffset;
					double x2 = x1 + step; 
					x1 *= distance;
					x2 *= distance;
					double y1a = y1 * distance;
					double y2a = y2 * distance;
					float tx = (float)x * gridToTexture;

					// Direct3D renders the texture upside down for some reason, so we flip the 
					// texture coordinates here.

					glTexCoord2f(tx, d3dOneMinus(ty));
					glVertex3d(x1, y1a, -distance);
					
					glTexCoord2f(tx + gridToTexture, d3dOneMinus(ty));
					glVertex3d(x2, y1a, -distance);
					
					glTexCoord2f(tx + gridToTexture, d3dOneMinus(ty + gridToTextureY));
					glVertex3d(x2, y2a, -distance);
					
					glTexCoord2f(tx, d3dOneMinus(ty + gridToTextureY));
					glVertex3d(x1, y2a, -distance);
				}
			}
		}
	glEnd();
#else
    glBegin( GL_QUADS );
        glTexCoord2i(0,0);          glVertex2i( 0, 0 );
        glTexCoord2i(1,0);          glVertex2i( m_screenW, 0 );
        glTexCoord2i(1,1);          glVertex2i( m_screenW, m_screenH );
        glTexCoord2i(0,1);          glVertex2i( 0, m_screenH );
    glEnd();
#endif
}


void Renderer::ApplyPixelEffect()
{
    //SetupMatricesFor2D	();
    //glDisable			(GL_DEPTH_TEST);
    //glDisable           (GL_CULL_FACE);
    //glEnable            (GL_BLEND);

    //glEnable            (GL_TEXTURE_2D);
    //glBindTexture       (GL_TEXTURE_2D, m_pixelEffectTexId );
    //glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    //glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    //glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    //glBegin( GL_QUADS );
    //    glBegin( GL_QUADS );
    //    glTexCoord2i(0,0);          glVertex2i( 0, 0 );
    //    glTexCoord2i(1,0);          glVertex2i( m_screenW/4, 0 );
    //    glTexCoord2i(1,1);          glVertex2i( m_screenW/4, m_screenH/4 );
    //    glTexCoord2i(0,1);          glVertex2i( 0, m_screenH/4 );
    //    glEnd();
    //glEnd();

    //glDisable           (GL_TEXTURE_2D);
    //glEnable            (GL_DEPTH_TEST);


    //return;

    START_PROFILE(g_app->m_profiler, "Pixel Apply");

	CHECK_OPENGL_STATE();

    glEnable            (GL_BLEND);
    glDisable           (GL_CULL_FACE);
    glDepthMask         (false);

#if USE_PIXEL_EFFECT_GRID_OPTIMISATION
	glEnable			(GL_DEPTH_TEST);
	SetupProjMatrixFor3D();
	glMatrixMode		(GL_MODELVIEW);
	glLoadIdentity		();
#else
	SetupMatricesFor2D	();
	glDisable			(GL_DEPTH_TEST);
#endif
    
	// Render debug information showing which cells are "dirty"
	if (0)
	{
	    glColor4f           ( 1.0f, 0.0f, 1.0f, 0.5f );
		PaintPixels();
	}

    glEnable            (GL_TEXTURE_2D);
    glBindTexture       (GL_TEXTURE_2D, m_pixelEffectTexId );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

	// Additive blocky
    START_PROFILE(g_app->m_profiler, "pass 1");
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glBlendFunc         ( GL_SRC_ALPHA, GL_ONE );
    glColor4f           ( 1.0f, 1.0f, 1.0f, 1.0f );
	PaintPixels();
    END_PROFILE(g_app->m_profiler, "pass 1");

    // Subtractive smooth
    START_PROFILE(g_app->m_profiler, "pass 2");
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glColor4f           ( 1.0f, 1.0f, 1.0f, 0.0f );
	PaintPixels();
    END_PROFILE(g_app->m_profiler, "pass 2");

    // Subtractive smooth
    START_PROFILE(g_app->m_profiler, "pass 3");
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glColor4f           ( 1.0f, 1.0f, 1.0f, 0.2f );
	PaintPixels();
    END_PROFILE(g_app->m_profiler, "pass 3");

    // Additive smooth
    START_PROFILE(g_app->m_profiler, "pass 4");
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glBlendFunc         ( GL_SRC_ALPHA, GL_ONE );
    glColor4f           ( 1.0f, 1.0f, 1.0f, 0.9f );
	PaintPixels();
    END_PROFILE(g_app->m_profiler, "pass 4");

    glDisable           (GL_TEXTURE_2D);
    glBlendFunc         (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    
	glEnable			(GL_DEPTH_TEST);//FIXME
    glDepthMask         (true);
    glEnable            (GL_CULL_FACE);
	glDisable	        (GL_BLEND);

	if (0)
	{
		g_editorFont.BeginText2D();
		double aspectRatio = (double)m_screenW / (double)m_screenH;
		int const yCellsUsed = (int)((double)PIXEL_EFFECT_GRID_RES / aspectRatio);
		float const scaleX = m_screenW / (float)PIXEL_EFFECT_GRID_RES;
		float const scaleY = m_screenH / yCellsUsed;
		float const offsetX = scaleX / 2.0f;
		float const offsetY = scaleY / 2.0f;
		for (int y = 0; y < yCellsUsed; ++y)
		{
			for (int x = 0; x < PIXEL_EFFECT_GRID_RES; ++x)
			{
				int blah = yCellsUsed - y - 1;
				const float dist = m_pixelEffectGrid[x][blah];
				if (dist < 1e9)
				{
					g_editorFont.DrawText2DCentre((float)x * scaleX + offsetX, 
												  (float)y * scaleY + offsetY,
												  12, "%.0f", dist);
				}
			}
		}
		g_editorFont.EndText2D();
	}

    CHECK_OPENGL_STATE();

    END_PROFILE(g_app->m_profiler, "Pixel Apply");
}


void Renderer::UpdateTotalMatrix()
{
    double m[16];
    double p[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, m);
    glGetDoublev(GL_PROJECTION_MATRIX, p);

	DarwiniaDebugAssert(m[3] == 0.0);
	DarwiniaDebugAssert(m[7] == 0.0);
	DarwiniaDebugAssert(m[11] == 0.0);
	DarwiniaDebugAssert(NearlyEquals(m[15], 1.0));

	DarwiniaDebugAssert(p[1] == 0.0);
	DarwiniaDebugAssert(p[2] == 0.0);
	DarwiniaDebugAssert(p[3] == 0.0);
	DarwiniaDebugAssert(p[4] == 0.0);
	DarwiniaDebugAssert(p[6] == 0.0);
	DarwiniaDebugAssert(p[7] == 0.0);
	if (m_renderingPoster == PosterMakerInactive)
	{
		DarwiniaDebugAssert(p[8] == 0.0);
		DarwiniaDebugAssert(p[9] == 0.0);
	}
	DarwiniaDebugAssert(p[12] == 0.0);
	DarwiniaDebugAssert(p[13] == 0.0);
	DarwiniaDebugAssert(p[15] == 0.0);

	m_totalMatrix[0] = m[0]*p[0] + m[1]*p[4] + m[2]*p[8] + m[3]*p[12];
	m_totalMatrix[1] = m[0]*p[1] + m[1]*p[5] + m[2]*p[9] + m[3]*p[13];
	m_totalMatrix[2] = m[0]*p[2] + m[1]*p[6] + m[2]*p[10] + m[3]*p[14];
	m_totalMatrix[3] = m[0]*p[3] + m[1]*p[7] + m[2]*p[11] + m[3]*p[15];

	m_totalMatrix[4] = m[4]*p[0] + m[5]*p[4] + m[6]*p[8] + m[7]*p[12];
	m_totalMatrix[5] = m[4]*p[1] + m[5]*p[5] + m[6]*p[9] + m[7]*p[13];
	m_totalMatrix[6] = m[4]*p[2] + m[5]*p[6] + m[6]*p[10] + m[7]*p[14];
	m_totalMatrix[7] = m[4]*p[3] + m[5]*p[7] + m[6]*p[11] + m[7]*p[15];

	m_totalMatrix[8]  = m[8]*p[0] + m[9]*p[4] + m[10]*p[8] + m[11]*p[12];
	m_totalMatrix[9]  = m[8]*p[1] + m[9]*p[5] + m[10]*p[9] + m[11]*p[13];
	m_totalMatrix[10] = m[8]*p[2] + m[9]*p[6] + m[10]*p[10] + m[11]*p[14];
	m_totalMatrix[11] = m[8]*p[3] + m[9]*p[7] + m[10]*p[11] + m[11]*p[15];

	m_totalMatrix[12] = m[12]*p[0] + m[13]*p[4] + m[14]*p[8] + m[15]*p[12];
	m_totalMatrix[13] = m[12]*p[1] + m[13]*p[5] + m[14]*p[9] + m[15]*p[13];
	m_totalMatrix[14] = m[12]*p[2] + m[13]*p[6] + m[14]*p[10] + m[15]*p[14];
	m_totalMatrix[15] = m[12]*p[3] + m[13]*p[7] + m[14]*p[11] + m[15]*p[15];
}


void Renderer::Get2DScreenPos(Vector3 const &v, Vector3 *_out)
{
    double out[4];

	#define m m_totalMatrix
	out[0] = v.x*m[0] + v.y*m[4] + v.z*m[8]  + m[12];
	out[1] = v.x*m[1] + v.y*m[5] + v.z*m[9]  + m[13];
	out[2] = v.x*m[2] + v.y*m[6] + v.z*m[10] + m[14];
	out[3] = v.x*m[3] + v.y*m[7] + v.z*m[11] + m[15];
	#undef m

	if (out[3] <= 0.0f) return;
    
	double multiplier = 0.5f / out[3];
	out[0] *= multiplier;
    out[1] *= multiplier;

    // Map x, y and z to range 0-1
    out[0] += 0.5;
    out[1] += 0.5;

    // Map x, y to viewport
    _out->x = out[0] * m_screenW;
    _out->y = out[1] * m_screenH;
	_out->z = out[3];
}

const double* Renderer::GetTotalMatrix()
{
	return m_totalMatrix;
}


void Renderer::RasteriseSphere(Vector3 const &_pos, float _radius)
{
	float const screenToGridFactor = (float)PIXEL_EFFECT_GRID_RES / (float)m_screenW;
	Camera *cam = g_app->m_camera;
	Vector3 centre;
	Vector3 topLeft;
	Vector3 bottomRight;
	Vector3 const camUpRight = (cam->GetRight() + cam->GetUp()) * _radius;
	Get2DScreenPos(_pos, &centre);
	Get2DScreenPos(_pos + camUpRight, &topLeft);
	Get2DScreenPos(_pos - camUpRight, &bottomRight);

	int x1 = floorf(topLeft.x * screenToGridFactor);
	int x2 = ceilf(bottomRight.x * screenToGridFactor);
	int y1 = floorf(bottomRight.y * screenToGridFactor);
	int y2 = ceilf(topLeft.y * screenToGridFactor);

	clamp(x1, 0, PIXEL_EFFECT_GRID_RES);
	clamp(x2, 0, PIXEL_EFFECT_GRID_RES);
	clamp(y1, 0, PIXEL_EFFECT_GRID_RES);
	clamp(y2, 0, PIXEL_EFFECT_GRID_RES);

	float const nearestZ = centre.z - _radius;

	for (int y = y1; y < y2; ++y)
	{
		for (int x = x1; x < x2; ++x)
		{
			if (nearestZ < m_pixelEffectGrid[x][y])
			{
				m_pixelEffectGrid[x][y] = nearestZ;
			}
		}
	}
}


void Renderer::MarkUsedCells(ShapeFragment const *_frag, Matrix34 const &_transform)
{
#if USE_PIXEL_EFFECT_GRID_OPTIMISATION
	Matrix34 total = _frag->m_transform * _transform;
	Vector3 worldPos = _frag->m_centre * total;

	// Return early if this shape fragment isn't on the screen
	{
		if (!g_app->m_camera->SphereInViewFrustum(worldPos, _frag->m_radius))
			return;
	}

    if (_frag->m_radius > 0.0f)
	{
		RasteriseSphere(worldPos, _frag->m_radius);
	}

	// Recurse into all child fragments
	int numChildren = _frag->m_childFragments.Size();
	for (int i = 0; i < numChildren; ++i)
	{
		ShapeFragment const *child = _frag->m_childFragments.GetData(i);
		MarkUsedCells(child, total);
	}
#endif // USE_PIXEL_EFFECT_GRID_OPTIMISATION
}


void Renderer::MarkUsedCells(Shape const *_shape, Matrix34 const &_transform)
{
    START_PROFILE( g_app->m_profiler, "MarkUsedCells" );
	MarkUsedCells(_shape->m_rootFragment, _transform);
    END_PROFILE( g_app->m_profiler, "MarkUsedCells" );
}
