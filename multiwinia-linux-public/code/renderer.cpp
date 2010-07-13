#include "lib/universal_include.h"

#include <math.h>

#include "lib/input/input.h"

#include "lib/3d_sprite.h"
#include "lib/avi_generator.h"
#include "lib/bitmap.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/input/eventhandler.h"
#ifdef TARGET_MSVC
#include "lib/input/win32_eventhandler.h"
#endif
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
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "helpsystem.h"
#endif
#include "landscape_renderer.h"
#include "location.h"
#include "location_editor.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "team.h"
#include "unit.h"
#include "user_input.h"
#include "water.h"
#include "water_reflection.h"
#include "gamecursor.h"
#include "startsequence.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "demoendsequence.h"
    #include "tutorial.h"
#endif
#include "control_help.h"
#include "game_menu.h"
#include "multiwinia.h"
#include "shaman_interface.h"
#include "markersystem.h"
#include "multiwiniahelp.h"

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
	m_displayFPS(true),
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
    m_renderDarwinLogo(-1.0f),
    m_gamma(1.2f),
	m_lastFrameBufferTexture(-1),
    m_menuTransitionState(MenuTransitionFinished),
    m_menuTransitionDirection(0),
	m_texCoordW(0),
	m_texCoordH(0)
{
#ifdef TARGET_PC_FULLGAME          
	m_displayFPS = false;
#endif
}

#ifdef USE_DIRECT3D
#include "lib/opengl_directx_internals.h"

void Renderer::SetScreenGamma( float _gamma )
{
	AppDebugOut("Setting Screen Gamma to %f\n", _gamma );
	m_gamma = _gamma;

	float g = 2.0 - _gamma;

	D3DGAMMARAMP gamma;
	for( int i=0; i < 256; i++ )
		gamma.red[i] = gamma.green[i] = gamma.blue[i] = iv_pow( i/255.0f, g ) * 65535;

	OpenGLD3D::g_pd3dDevice->SetGammaRamp( 0, 0, &gamma );    
}
#endif

void Renderer::Initialise()
{
	m_screenW = g_prefsManager->GetInt("ScreenWidth", 0);
	m_screenH = g_prefsManager->GetInt("ScreenHeight", 0);
	bool windowed = (bool) g_prefsManager->GetInt("ScreenWindowed", 0);
	int colourDepth = g_prefsManager->GetInt("ScreenColourDepth", 32);
	int refreshRate = g_prefsManager->GetInt("ScreenRefresh", 75);
	int zDepth = g_prefsManager->GetInt("ScreenZDepth", 24);
	bool waitVRT = g_prefsManager->GetInt("WaitVerticalRetrace", 1);

    int overscan = 0.0f;//g_prefsManager->GetInt( "ScreenOverscan", 0 );
    if( m_screenW == 0 && m_screenH == 0 )
    {
        g_windowManager->SuggestDefaultRes( &m_screenW, &m_screenH, &refreshRate, &colourDepth );

        g_prefsManager->SetInt( "ScreenWidth", m_screenW );
        g_prefsManager->SetInt( "ScreenHeight", m_screenH );
        g_prefsManager->SetInt( "ScreenRefresh", refreshRate );
        g_prefsManager->SetInt( "ScreenColourDepth", colourDepth );
    }

    g_windowManager->GetClosestResolution( &m_screenW, &m_screenH, &refreshRate );

    bool success = g_windowManager->CreateWin(m_screenW, m_screenH, windowed, colourDepth, refreshRate,
											  zDepth, true, true, APP_NAME_W);

    if( !success )
    {
        char caption[512];
        snprintf( caption, sizeof(caption),
                  "Failed to set requested screen resolution of\n"
                  "%d x %d, %d bit colour, %s\n"
                  "%d refresh rate, %d bit z depth",
                  m_screenW, m_screenH, colourDepth, windowed ? "windowed" : "fullscreen",
                  refreshRate, zDepth );
        AppDebugOut( "Failed to set requested screen resolution of %d x %d, %d bit colour, %s, %d refresh rate, %d bit z depth",
                     m_screenW, m_screenH, colourDepth, windowed ? "windowed" : "fullscreen", refreshRate, zDepth );
        MessageDialog *dialog = new MessageDialog( "Error", caption );
        EclRegisterWindow( dialog );
        dialog->m_x = 100;
        dialog->m_y = 100;

        // Try suggested default resolution
        g_windowManager->SuggestDefaultRes( &m_screenW, &m_screenH, &refreshRate, &colourDepth );
        zDepth = 24;
        waitVRT = 0;

        success = g_windowManager->CreateWin(m_screenW, m_screenH, windowed, colourDepth, refreshRate,
											 zDepth, true, true, APP_NAME_W);

        if( !success )
        {
            AppDebugOut( "Failed to set requested screen resolution of %d x %d, %d bit colour, %s, %d refresh rate, %d bit z depth",
                         m_screenW, m_screenH, colourDepth, windowed ? "windowed" : "fullscreen", refreshRate, zDepth );

            // Go for safety values
            m_screenW = 800;
            m_screenH = 600;
            windowed = 1;
            colourDepth = 16;
            zDepth = 16;
            refreshRate = 60;

			success = g_windowManager->CreateWin( m_screenW, m_screenH, windowed, colourDepth, refreshRate, zDepth, waitVRT, true, APP_NAME_W );
            if( !success )
            {
                AppDebugOut( "Failed to set requested screen resolution of %d x %d, %d bit colour, %s, %d refresh rate, %d bit z depth",
                             m_screenW, m_screenH, colourDepth, windowed ? "windowed" : "fullscreen", refreshRate, zDepth );

                // next try with 24bit z (colour depth is automatic in windowed mode)
                zDepth = 24;
                success = g_windowManager->CreateWin( m_screenW, m_screenH, windowed, colourDepth, refreshRate, zDepth, waitVRT, true, APP_NAME_W );
                AppReleaseAssert( success, "Failed to set screen mode" );
            }
        }
    }

    g_prefsManager->SetInt( "ScreenWidth", m_screenW );
    g_prefsManager->SetInt( "ScreenHeight", m_screenH );
    g_prefsManager->SetInt( "ScreenWindowed", windowed? 1 : 0 );
    g_prefsManager->SetInt( "ScreenColourDepth", colourDepth );
    g_prefsManager->SetInt( "ScreenZDepth", zDepth );
    g_prefsManager->SetInt( "ScreenRefresh", refreshRate );
    g_prefsManager->SetInt( "WaitVerticalRetrace", waitVRT? 1 : 0 );
    g_prefsManager->SetInt( "ScreenOverscan", overscan );
    g_prefsManager->Save();

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
		glTexCoord2f(1.0f, 1.0f);	glVertex3dv( (pos + up - right).GetData() );
		glTexCoord2f(0.0f, 1.0f);	glVertex3dv( (pos + up + right).GetData() );
		glTexCoord2f(0.0f, 0.0f);	glVertex3dv( (pos - up + right).GetData() );
		glTexCoord2f(1.0f, 0.0f);	glVertex3dv( (pos - up - right).GetData() );
	glEnd();

	glAlphaFunc		( GL_GREATER, 0.01);
    glDisable( GL_ALPHA_TEST );
    glDisable( GL_BLEND );
    
	glDisable(GL_TEXTURE_2D);

	glLineWidth(1.0f);
	glBegin(GL_LINE_LOOP);
		glVertex3dv( (pos + up - right).GetData() );
		glVertex3dv( (pos + up + right).GetData() );
		glVertex3dv( (pos - up + right).GetData() );
		glVertex3dv( (pos - up - right).GetData() );
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
	
    START_PROFILE( "Render" );
	CHECK_OPENGL_STATE();
	if( g_profiler ) g_profiler->RenderStarted();

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
        if( posterResolution > 1 )
        {
            m_renderingPoster = PosterMakerTiling;
        }
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


	if( g_profiler ) g_profiler->RenderEnded();
	CHECK_OPENGL_STATE();
    END_PROFILE( "Render" );
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
        float overscan = 0.0f;
		float left = -1 - overscan;
		float right = m_screenW + overscan;
		float bottom = m_screenH + overscan;
		float top = -1 - overscan;

		glEnable(GL_BLEND);
		glDepthMask(false);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		glColor4ub(0, 0, 0, (int)(m_fadedness * 255.0f)); 
		glBegin(GL_QUADS);
			glVertex2i(left, top);
			glVertex2i(right, top);
			glVertex2i(right, bottom);
			glVertex2i(left, bottom);
		glEnd();
		
		glDisable(GL_BLEND);
		glDepthMask(true);
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}


void Renderer::RenderPaused()
{
    if( g_app->m_hideInterface ) return;

	const char *msg = "PAUSED";
	int x = g_app->m_renderer->ScreenW()/2;
	int y = g_app->m_renderer->ScreenH()*0.4f;	
	TextRenderer &font = g_gameFont;
	
	font.BeginText2D();
	
	// Black Background
	font.SetRenderOutline( true );
    glColor4f(1.0f,1.0f,1.0f,0.0f);
	font.DrawText2DCentre( x, y, 80, msg );
	
	// White Foreground
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	font.SetRenderOutline(false);
	font.DrawText2DCentre( x, y, 80, msg );
	
	font.EndText2D();
}

void Renderer::RenderFrame(bool withFlip)
{
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
	
    bool renderChat = false;

    START_PROFILE( "Render Clear");
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
	END_PROFILE( "Render Clear");
	
	bool deformStarted = false;

	if( g_app->m_atMainMenu &&
        g_app->m_gameMenu->m_menuCreated)
    {
        g_app->m_gameMenu->Render();
    }
    else if (g_app->m_editing)
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
            if( g_prefsManager->GetInt("RenderPixelShader") > 0 && !g_app->Multiplayer() )
            {
				switch (m_renderingPoster)
				{
					case PosterMakerInactive:
						{

						PreRenderPixelEffect();

#ifdef USE_DIRECT3D
						if (g_prefsManager->GetInt("RenderWaterEffect", 1) && g_waterReflectionEffect)
						{
						    g_waterReflectionEffect->PreRenderWaterReflection();
						}

						if(g_deformEffect)
						{
							deformStarted = true;
							g_deformEffect->Start();
						}
#endif // USE_DIRECT3D
						START_PROFILE( "Render Clear");
						glClear	(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
						END_PROFILE( "Render Clear");

						g_app->m_location->Render();
                        renderChat = true;
						ApplyPixelEffect();

						SetupMatricesFor3D();
						break;}
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
                renderChat = true;
            }
		}
		else
		{
			g_app->m_globalWorld->Render();
		}
	}

    
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    g_app->m_helpSystem->Render();
#endif
    //g_app->m_controlHelpSystem->Render();
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    if( g_app->m_tutorial ) g_app->m_tutorial->Render();
#endif
	g_explosionManager.Render();
    g_app->m_particleSystem->Render();    
#ifdef USE_DIRECT3D
	if(g_deformEffect && deformStarted) g_deformEffect->Stop();
#endif


#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    if( g_app->m_demoEndSequence ) 
    {
        g_editorFont.BeginText2D();
        g_app->m_demoEndSequence->Render();
        SetupMatricesFor3D();
        glEnable( GL_DEPTH_TEST );
    }
#endif

    g_app->m_markerSystem->Render();
    g_app->m_multiwiniaHelp->Render();
	g_app->m_shamanInterface->Render();
    g_app->m_userInput->Render();
    g_app->m_gameCursor->Render();	
    g_app->m_gameCursor->RenderBasicCursorAlwaysOnTop();
    if( renderChat ) g_app->m_location->RenderChatMessages();
    g_app->m_camera->Render();
    g_app->m_taskManagerInterface->Render();

#ifdef DEBUG_RENDER_ENABLED
	g_debugRenderer.Render();
#endif
    
//	RenderFlatTexture();

	//if (m_renderingPoster == PosterMakerInactive) 
	{
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
	    g_app->m_sepulveda->Render();  
#endif
	}
	g_app->m_controlHelpSystem->Render();

    g_editorFont.BeginText2D();

	if (!IsFirsttimeSequencing()) RenderFadeOut();

    if( g_app->m_location &&
        !g_app->m_editing &&
        !g_app->m_atMainMenu &&
        g_app->m_multiwinia &&
        g_app->m_gameMode == App::GameModeMultiwinia ) g_app->m_multiwinia->Render();

    if (m_displayFPS && !g_app->m_hideInterface)
    {	
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        if ( g_app->m_location )
        {
             g_editorFont.DrawText2DRight( m_screenW - 10, m_screenH - 40, DEF_FONT_SIZE, "FPS: %d   DWs: %d", m_fps, Entity::s_entityPopulation );
        }
        else
        {
             g_editorFont.DrawText2DRight( m_screenW - 10, m_screenH - 40, DEF_FONT_SIZE, "FPS: %d", m_fps );
        }
    }

    if( !g_app->m_hideInterface )
    {
		bool showVersion = true;

#ifdef TARGET_PC_FULLGAME
		// In the full game don't show the version number in the location
		if( g_app->m_location )
			showVersion = false;
#endif

		if( showVersion )	
	        g_editorFont.DrawText2DRight( m_screenW - 10, m_screenH - 20, DEF_FONT_SIZE, DARWINIA_VERSION_STRING );
    }

    g_gameTimer.DebugRender();

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

    //
    // Network stuff


	if( g_app->m_clientToServer->m_outOfSyncClients.Size() && !g_app->m_hideInterface )
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2DCentre( m_screenW/2, 130, 30, LANGUAGEPHRASE("multiwinia_error_syncerror") );
        
        glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2DCentre( m_screenW/2, 130, 30, LANGUAGEPHRASE("multiwinia_error_syncerror") );
    }
    
	if( ( g_app->m_location || g_app->m_atLobby )&& !g_app->m_editing &&
		!g_app->IsSinglePlayer() &&
		g_app->m_clientToServer->m_lastServerLetterReceivedTime > 0.0f && 
        GetHighResTime() > g_app->m_clientToServer->m_lastServerLetterReceivedTime + 5.0f &&
		!g_app->m_hideInterface )
    {
        double timeLag = GetHighResTime() - g_app->m_clientToServer->m_lastServerLetterReceivedTime;

        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2DCentre( m_screenW/2, 170, 30, LANGUAGEPHRASE("multiwinia_error_lostconnection") );
        
        glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2DCentre( m_screenW/2, 170, 30, LANGUAGEPHRASE("multiwinia_error_lostconnection") );
    }


    //
    // Personalisation information

	if( !g_app->m_taskManagerInterface->IsVisible()  &&
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
        !g_app->m_sepulveda->IsVisible() &&
#endif
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

        double timeNow = GetHighResTime();
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

		glBegin( GL_QUADS );

		for( float dx = -4; dx <= 4; dx += 4 )
        {
            for( float dy = -4; dy <= 4; dy += 4 )
            {
		        glTexCoord2i(0,1);      glVertex2f( x+dx, y+dy );
		        glTexCoord2i(1,1);      glVertex2f( x+w+dx, y+dy );
		        glTexCoord2i(1,0);      glVertex2f( x+w+dx, y+h+dy );
		        glTexCoord2i(0,0);      glVertex2f( x+dx, y+h+dy );
            }
        }

		glEnd();

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

        g_gameFont.DrawText2DCentre( m_screenW/2, m_screenH * 0.8f, textSize, LANGUAGEPHRASE("launchpad_logo_1") );
        g_gameFont.SetRenderOutline( false );
        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        g_gameFont.DrawText2DCentre( m_screenW/2, m_screenH * 0.8f, textSize, LANGUAGEPHRASE("launchpad_logo_1") );
    }

    g_editorFont.EndText2D();

    if( EclGetWindows()->Size() == 0 )
    {
	    if ( (g_app->m_gameMode != App::GameModeMultiwinia && !g_eventHandler->WindowHasFocus()) ||
              g_app->m_userRequestsPause )
        {
		    RenderPaused();	
        }
    }

    START_PROFILE( "GL Flip");
	
    if(withFlip)
	{
		// Menu transition was here
		AdvanceMenuTransition();
		g_windowManager->Flip();
	}

    END_PROFILE( "GL Flip");

}


int Renderer::ScreenW() const
{
    return m_screenW;
}


int Renderer::ScreenH() const
{
    return m_screenH;
}

// Used for temporary res change while rendering into texture of different size.
void Renderer::SetScreenRes(int w, int h)
{
	m_screenW = w;
	m_screenH = h;
}

void Renderer::SetupProjMatrixFor3D() const
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (m_renderingPoster == PosterMakerTiling)
	{
		float const screenH = ScreenH();
		float const screenW = ScreenW();
        int posterResolution = g_prefsManager->GetInt( "RenderPosterResolution", 1 );

		float scale = g_app->m_camera->GetFov() / 78.0f;
		float left = -scale * m_nearPlane;
        float aspect = screenH / screenW;
		float top = -aspect * scale * m_nearPlane;
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


void Renderer::SetupMatricesFor2D( bool _noOverscan ) const
{
	GLint v[4];
	glGetIntegerv(GL_VIEWPORT, &v[0]);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

    float left = v[0];
	float right = v[0] + v[2];
	float bottom = v[1] + v[3];
	float top = v[1];

    float overscan = 0.0f;
    if( _noOverscan )
    {
        left -= overscan;
        right += overscan;
        bottom += overscan;
        top -= overscan;
    }

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
	AppDebugAssert(_nearPlane < _farPlane);
	AppDebugAssert(_nearPlane > 0.0f);
	m_nearPlane = _nearPlane;
	m_farPlane = _farPlane;
}


int Renderer::GetGLStateInt(int pname) const
{
	GLint returnVal;
	glGetIntegerv((GLenum)pname, &returnVal);
	return returnVal;
}


float Renderer::GetGLStateFloat(int pname) const
{
	float returnVal;
	glGetFloatv(pname, &returnVal);
	return returnVal;
}


void Renderer::CheckOpenGLState(bool fullCheck, int lineNumber) const
{   
	GLint results[10];
	float resultsf[10];
	GLint glError = glGetError();
	
	if(glError != GL_NO_ERROR)
		printf("glError: %d, %d\n", glError, lineNumber);
	AppDebugAssert(glError == GL_NO_ERROR);

	if(!fullCheck)
		return;
	
	// Geometry
//	AppDebugAssert(glIsEnabled(GL_CULL_FACE));
	AppDebugAssert(GetGLStateInt(GL_FRONT_FACE) == GL_CCW);
	glGetIntegerv(GL_POLYGON_MODE, results);
	AppDebugAssert(results[0] == GL_FILL);
	AppDebugAssert(results[1] == GL_FILL);
	AppDebugAssert(!glIsEnabled(GL_NORMALIZE));

	// Colour
	AppDebugAssert(!glIsEnabled(GL_COLOR_MATERIAL));
	AppDebugAssert(GetGLStateInt(GL_COLOR_MATERIAL_FACE) == GL_FRONT_AND_BACK);
	AppDebugAssert(GetGLStateInt(GL_COLOR_MATERIAL_PARAMETER) == GL_AMBIENT_AND_DIFFUSE);

	// Lighting
	AppDebugAssert(!glIsEnabled(GL_LIGHTING));

    AppDebugAssert(!glIsEnabled(GL_LIGHT0));
	AppDebugAssert(!glIsEnabled(GL_LIGHT1));
	AppDebugAssert(!glIsEnabled(GL_LIGHT2));
	AppDebugAssert(!glIsEnabled(GL_LIGHT3));
	AppDebugAssert(!glIsEnabled(GL_LIGHT4));
	AppDebugAssert(!glIsEnabled(GL_LIGHT5));
	AppDebugAssert(!glIsEnabled(GL_LIGHT6));
	AppDebugAssert(!glIsEnabled(GL_LIGHT7));

	glGetFloatv(GL_LIGHT_MODEL_AMBIENT, resultsf);
	AppDebugAssert(resultsf[0] < 0.001f &&
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
	//			AppDebugAssert(fabsf(lightPos1[i] - pos1_actual[i]) < 0.001f);
	//			AppDebugAssert(fabsf(light->m_colour[i] - diffuse1_actual[i]) < 0.001f);
	//			AppDebugAssert(fabsf(light->m_colour[i] - specular1_actual[i]) < 0.0001f);
	//			AppDebugAssert(fabsf(ambCol1[i] - ambient1_actual[i]) < 0.001f);
			}
		}
	}

	// Blending, Anti-aliasing, Fog and Polygon Offset
//	AppDebugAssert(!glIsEnabled(GL_BLEND));
	AppDebugAssert(GetGLStateInt(GL_BLEND_DST) == GL_ONE_MINUS_SRC_ALPHA);
	AppDebugAssert(GetGLStateInt(GL_BLEND_SRC) == GL_SRC_ALPHA);
	AppDebugAssert(!glIsEnabled(GL_ALPHA_TEST));
	AppDebugAssert(GetGLStateInt(GL_ALPHA_TEST_FUNC) == GL_GREATER);
	AppDebugAssert(GetGLStateFloat(GL_ALPHA_TEST_REF) == 0.01f);
	AppDebugAssert(!glIsEnabled(GL_FOG));
	AppDebugAssert(GetGLStateFloat(GL_FOG_DENSITY) == 1.0f);
	AppDebugAssert(GetGLStateFloat(GL_FOG_END) >= 4000.0f);
	//AppDebugAssert(GetGLStateFloat(GL_FOG_START) >= 1000.0f);
	glGetFloatv(GL_FOG_COLOR, resultsf);
//	AppDebugAssert(fabsf(resultsf[0] - g_app->m_location->m_backgroundColour.r/255.0f) < 0.001f);
//	AppDebugAssert(fabsf(resultsf[1] - g_app->m_location->m_backgroundColour.g/255.0f) < 0.001f);
//	AppDebugAssert(fabsf(resultsf[2] - g_app->m_location->m_backgroundColour.b/255.0f) < 0.001f);
//	AppDebugAssert(fabsf(resultsf[3] - g_app->m_location->m_backgroundColour.a/255.0f) < 0.001f);
	AppDebugAssert(GetGLStateInt(GL_FOG_MODE) == GL_LINEAR);
	AppDebugAssert(!glIsEnabled(GL_LINE_SMOOTH));
	AppDebugAssert(!glIsEnabled(GL_POINT_SMOOTH));
	
	// Texture Mapping
	AppDebugAssert(!glIsEnabled(GL_TEXTURE_2D));
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, results);
	AppDebugAssert(results[0] == GL_CLAMP);
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, results);
	AppDebugAssert(results[0] == GL_CLAMP);
	glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, results);
	AppDebugAssert(results[0] == GL_MODULATE);
	glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, results);
	AppDebugAssert(results[0] == 0);
	AppDebugAssert(results[1] == 0);
	AppDebugAssert(results[2] == 0);
	AppDebugAssert(results[3] == 0);

	// Frame Buffer
	AppDebugAssert(glIsEnabled(GL_DEPTH_TEST));
	AppDebugAssert(GetGLStateInt(GL_DEPTH_WRITEMASK) != 0);
	AppDebugAssert(GetGLStateInt(GL_DEPTH_FUNC) == GL_LEQUAL);
	AppDebugAssert(glIsEnabled(GL_SCISSOR_TEST) == 0);

	// Hints
	AppDebugAssert(GetGLStateInt(GL_FOG_HINT) == GL_DONT_CARE);
	AppDebugAssert(GetGLStateInt(GL_POLYGON_SMOOTH_HINT) == GL_DONT_CARE);
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
    START_PROFILE( "Pixel Pre-render");

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

    START_PROFILE( "blend old");
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
    END_PROFILE( "blend old");
           
    //glDisable           (GL_TEXTURE_2D);


    //
    // Draw all pixelated objects to the screen
    // Find the nearest pixelated object and update m_pixelSize at the end

    START_PROFILE( "Draw pixelated");
    glViewport( 0, 0, m_pixelSize, m_pixelSize );
    float nearest = 99999.9f;

    float cutoff = 1000.0f;
    Vector3 camPos = g_app->m_camera->GetPos();

    for( int t = 0; t < NUM_TEAMS; ++t )    
    {
        if( g_app->m_location->m_teams[t]->m_teamType != TeamTypeUnused )
        {
            for( int i = 0; i < g_app->m_location->m_teams[t]->m_units.Size(); ++i )
            {
                if( g_app->m_location->m_teams[t]->m_units.ValidIndex(i) )
                {
                    Unit *unit = g_app->m_location->m_teams[t]->m_units[i];
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
                                        if( !entity ) continue;

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
    
            for( int i = 0; i < g_app->m_location->m_teams[t]->m_others.Size(); ++i )
            {
                if( g_app->m_location->m_teams[t]->m_others.ValidIndex(i) )
                {
                    Entity *entity = g_app->m_location->m_teams[t]->m_others[i];
                    if( entity->IsInView() )
                    {
                        float distance = ( entity->m_pos - camPos ).Mag();
                        if( distance < cutoff )
                        {
                            bool rendered = false;
                            if( i <= g_app->m_location->m_teams[t]->m_others.GetLastUpdated() )
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
    
    END_PROFILE( "Draw pixelated");
    glViewport( 0, 0, m_screenW, m_screenH );


    //
    // Copy the screen to a texture

    START_PROFILE( "Gen new texture");
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
    END_PROFILE( "Gen new texture");

    glDepthMask( true );

	CHECK_OPENGL_STATE();


    //
    // Update pixel size

    //if      ( nearest < 30 )        m_pixelSize = 128;    
    //else if ( nearest < 200 )       m_pixelSize = 256;
    //else                            m_pixelSize = 512;

    END_PROFILE( "Pixel Pre-render");
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

    START_PROFILE( "Pixel Apply");

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
    START_PROFILE( "pass 1");
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glBlendFunc         ( GL_SRC_ALPHA, GL_ONE );
    glColor4f           ( 1.0f, 1.0f, 1.0f, 1.0f );
	PaintPixels();
    END_PROFILE( "pass 1");

    // Subtractive smooth
    START_PROFILE( "pass 2");
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glColor4f           ( 1.0f, 1.0f, 1.0f, 0.0f );
	PaintPixels();
    END_PROFILE( "pass 2");

    // Subtractive smooth
    START_PROFILE( "pass 3");
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glColor4f           ( 1.0f, 1.0f, 1.0f, 0.2f );
	PaintPixels();
    END_PROFILE( "pass 3");

    // Additive smooth
    START_PROFILE( "pass 4");
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glBlendFunc         ( GL_SRC_ALPHA, GL_ONE );
    glColor4f           ( 1.0f, 1.0f, 1.0f, 0.9f );
	PaintPixels();
    END_PROFILE( "pass 4");

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

    END_PROFILE( "Pixel Apply");
}


void Renderer::UpdateTotalMatrix()
{
    double m[16];
    double p[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, m);
    glGetDoublev(GL_PROJECTION_MATRIX, p);

	AppDebugAssert(m[3] == 0.0);
	AppDebugAssert(m[7] == 0.0);
	AppDebugAssert(m[11] == 0.0);
	AppDebugAssert(NearlyEquals(m[15], 1.0));

	AppDebugAssert(p[1] == 0.0);
	AppDebugAssert(p[2] == 0.0);
	AppDebugAssert(p[3] == 0.0);
	AppDebugAssert(p[4] == 0.0);
	AppDebugAssert(p[6] == 0.0);
	AppDebugAssert(p[7] == 0.0);
	if (m_renderingPoster == PosterMakerInactive)
	{
		AppDebugAssert(p[8] == 0.0);
		AppDebugAssert(p[9] == 0.0);
	}
	AppDebugAssert(p[12] == 0.0);
	AppDebugAssert(p[13] == 0.0);
	AppDebugAssert(p[15] == 0.0);

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

	int x1 = floor(topLeft.x * screenToGridFactor);
	int x2 = ceilf(bottomRight.x * screenToGridFactor);
	int y1 = floor(bottomRight.y * screenToGridFactor);
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
    START_PROFILE( "MarkUsedCells" );
	MarkUsedCells(_shape->m_rootFragment, _transform);
    END_PROFILE(  "MarkUsedCells" );
}

void Renderer::Clip(int _x, int _y, int _w, int _h)
{
    Vector3 a( _x, _y, 0 );
    Vector3 b( _x + _w, _y + _h, 0 );

    float x1, y1, x2, y2;

    g_app->m_camera->Get2DScreenPos( a, &x1, &y1 );
    g_app->m_camera->Get2DScreenPos( b, &x2, &y2 );

//    AppDebugOut(" Projected coordinates are: x1 = %f, y1 = %f, x2 = %f, y2 = %f\n", x1, y1, x2, y2 );

    glScissor( x1, y2, x2 - x1, y1 - y2 );
    glEnable( GL_SCISSOR_TEST );
}

void Renderer::EndClip()
{
    glDisable( GL_SCISSOR_TEST );
}


void Renderer::StartMenuTransition()
{
	if( m_menuTransitionState != MenuTransitionFinished )
    {
		m_menuTransitionState = MenuTransitionInProgress;
        m_menuTransitionStartTime = GetHighResTime();
    }
}

void Renderer::InitialiseMenuTransition( float _delay, int direction )
{

	if(RequirePowerOfTwoTextures())
	{
		m_texCoordW = min(1.0f, float(m_screenW)/float(nearestPowerOfTwo(m_screenW)));
		m_texCoordH = min(1.0f, float(m_screenH)/float(nearestPowerOfTwo(m_screenH)));
	}
	else
	{
		m_texCoordW = 1.0f;
		m_texCoordH = 1.0f;
	}

	//m_menuTransitionState = MenuTransitionBeginning;
	m_menuTransitionDuration = _delay;
    m_menuTransitionDirection = direction;
    CaptureFrameBuffer();
	m_menuTransitionState = MenuTransitionWaiting;
}


void Renderer::AdvanceMenuTransition()
{
	switch( m_menuTransitionState )
	{
		case MenuTransitionBeginning:
			CaptureFrameBuffer();
			m_menuTransitionState = MenuTransitionWaiting;
			break;

		case MenuTransitionInProgress:
			{
				double now = GetHighResTime();
				float f = (now - m_menuTransitionStartTime) / m_menuTransitionDuration;
			
				if( f <= 1.0f )
					RenderMenuTransition( f );
				else
					m_menuTransitionState = MenuTransitionFinished;
			}
			break;
	}

    if( g_app->m_doMenuTransition )
    {
        StartMenuTransition();
        g_app->m_doMenuTransition = false;
    }
}

#if 0
void Renderer::RenderMenuTransition( float _f )
{
	SetupMatricesFor2D();

	glBindTexture( GL_TEXTURE_2D, /*g_app->m_resource->GetTexture( "sprites/darwinian.bmp" )*/ m_lastFrameBufferTexture );
	glEnable( GL_TEXTURE_2D );

    float overscan = g_prefsManager->GetInt( "ScreenOverscan", 0 );
    
	float left = -1 - overscan;
	float right = m_screenW + overscan;
	float bottom = m_screenH + overscan;
	float top = -1 - overscan;

	float _p = _f / 2.0f;
    
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f( 1.0 - _f, 1.0f - _f, 1.0f - _f, 1.0f - _f );

	glEnable( GL_BLEND );

	glDisable( GL_CULL_FACE );
    glDepthMask         (false);
	glDisable( GL_DEPTH_TEST );

	glBegin( GL_QUADS );	
		// d3dOneMinus is to take into account that the window coordinates
		// for Direct3D and OpenGL are differently located, consequently functions like
		// glCopyTexImage2D behave differently in Direct3D and OpenGL
		glTexCoord2f( _p, d3dOneMinus(1 - _p) );		glVertex2f( left, top );		
		glTexCoord2f( 1 - _p, d3dOneMinus(1 - _p) );	glVertex2f( right, top );		
		glTexCoord2f( 1 - _p, d3dOneMinus(_p) );		glVertex2f( right, bottom );		
		glTexCoord2f( _p, d3dOneMinus(_p) );			glVertex2f( left, bottom );		
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
}
#endif

void Renderer::RenderMenuTransition( float _f )
{
    SetupMatricesFor2D();

    //
    // Get the texture ready for use

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, m_lastFrameBufferTexture );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glDisable       ( GL_TEXTURE_2D );

    //
    // Work out where the transition will be

    float overscan = 0.0f;
    float left = -overscan;
    float right = m_screenW + overscan;
    float bottom = m_screenH + overscan;
    float top = -overscan;

    float overallSpeed = 1.2f;
    float fraction = _f * overallSpeed * 3.0f;

    left += m_screenW * 0.2f * fraction * m_menuTransitionDirection;
    right -= m_screenW * 0.2f * fraction * m_menuTransitionDirection;
    bottom -= m_screenH * 0.2f * fraction * m_menuTransitionDirection;
    top += m_screenH * 0.2f * fraction * m_menuTransitionDirection;

    left -= m_screenW * pow(fraction,6) * m_menuTransitionDirection;
    right -= m_screenW * pow(fraction,6) * m_menuTransitionDirection;


    // 
    // Render some black under and around it for effect

    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glColor4f   ( 1.0f, 1.0f, 1.0f, max(0.0f, 1.0f - fraction * 0.2f) );
    glEnable    ( GL_BLEND );
    glDisable   ( GL_CULL_FACE );
    glDepthMask (false);
    glDisable   ( GL_DEPTH_TEST );

    glBegin( GL_QUADS );
        glColor4f( 0.0, 0.0f, 0.0f, 1.0f );
        glVertex2f( right, bottom );
        glVertex2f( right, top );
        glColor4f( 0.0, 0.0f, 0.0f, 0.0f );
        glVertex2f( right+20, top );
        glVertex2f( right+20, bottom );
    glEnd();        

    //
    // Render the transition in place

    glEnable    ( GL_TEXTURE_2D );
    glColor4f   ( 1.0f, 1.0f, 1.0f, max(0.0f, 1.0f - fraction * 0.2f) );

    glBegin( GL_QUADS );	
        glTexCoord2f( 0, d3dOneMinus(m_texCoordH) );		glVertex2f( left, top );		
        glTexCoord2f( m_texCoordW, d3dOneMinus(m_texCoordH) );	    glVertex2f( right, top );		
        glTexCoord2f( m_texCoordW, d3dOneMinus(0) );		glVertex2f( right, bottom );			
        glTexCoord2f( 0, d3dOneMinus(0) );		glVertex2f( left, bottom );		
    glEnd();

    glDisable   (GL_TEXTURE_2D);
    glEnable    (GL_CULL_FACE);

    SetupMatricesFor3D();

}


bool Renderer::IsMenuTransitionComplete() const
{
	return m_menuTransitionState == MenuTransitionFinished;
}

void CheckGLError()
{
	GLenum err = glGetError();
	if( err != GL_NO_ERROR )
	{
		AppDebugOut( "GL error: %d\n", err );
	}
}

void Renderer::CaptureFrameBuffer()
{
	// Also, we need to test this (by rendering it somehow?)

    double timeNow = GetHighResTime();

	if( m_lastFrameBufferTexture == -1 )
	{
		GLuint texId;
		glGenTextures( 1, &texId );
		CheckGLError();
		m_lastFrameBufferTexture = texId;

		glBindTexture( GL_TEXTURE_2D, m_lastFrameBufferTexture );

		glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
		glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

		if(RequirePowerOfTwoTextures())
		{
			glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 0, 0, nearestPowerOfTwo(m_screenW), 
															  nearestPowerOfTwo(m_screenH), 0 ); 
		}
		else
		{
			glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 0, 0, m_screenW, m_screenH, 0 ); 
		}

	}
	else
	{
		glBindTexture( GL_TEXTURE_2D, m_lastFrameBufferTexture );

		if(RequirePowerOfTwoTextures())
		{
			glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 0, 0, nearestPowerOfTwo(m_screenW), 
															  nearestPowerOfTwo(m_screenH), 0 ); 
		}
		else
		{
			glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 0, 0, m_screenW, m_screenH, 0 ); 
		}
	}
}
