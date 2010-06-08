
#include "lib/universal_include.h"

#include <stdio.h>
#include <math.h>
#include <float.h>

#include <eclipse.h>

#include "lib/avi_generator.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/input/input.h"
#include "lib/targetcursor.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/preferences.h"
#include "lib/language_table.h"

#include "interface/debugmenu.h"

#include "worldobject/worldobject.h"
#include "worldobject/engineer.h"

#include "network/clienttoserver.h"

#include "app.h"
#include "camera.h"
#include "globals.h"
#include "global_world.h"
#include "location.h"
#include "main.h"
#include "renderer.h"
#include "sound/soundsystem.h"
#include "unit.h"
#include "user_input.h"
#include "multiwinia.h"

#include <limits>

// *** Constructor
UserInput::UserInput()
:	m_removeTopLevelMenu(false)
{
	int const screenH = g_app->m_renderer->ScreenH();
	int const screenW = g_app->m_renderer->ScreenW();

    EclInitialise( g_app->m_renderer->ScreenW(), g_app->m_renderer->ScreenH() );
}


// *** AdvanceMenus
void UserInput::AdvanceMenus()
{
//	if ( g_keyDeltas[KEY_F1] )
//		DebugKeyBindings::DebugMenu();
		
	InputManager *im = g_inputManager;
	int mouseX = g_target->X();
	int mouseY = g_target->Y();
	bool lmb = im->controlEvent( ControlEclipseLMousePressed );
	bool rmb = im->controlEvent( ControlEclipseRMousePressed );

    EclUpdateMouse( mouseX, mouseY, lmb, rmb );
    EclUpdate();

	if ( im->controlEvent( ControlEclipseLMouseDown ) )
	{
		EclWindow *winUnderMouse = EclGetWindow( mouseX, mouseY );
		if (winUnderMouse)
		{
			im->suppressEvent( ControlEclipseLMouseDown );
		}
	}
}


// *** Advance
void UserInput::Advance()
{
    START_PROFILE( "Advance UserInput");

	g_inputManager->Advance();

	if (m_removeTopLevelMenu)
	{
		EclWindow *win = EclGetWindow("dialog_toolsmenu");
		if (win)
		{
			EclRemoveWindow(win->m_name);
		}
		m_removeTopLevelMenu = false;
	}

	AdvanceMenus();

#ifdef USE_DARWINIA_MOD_SYSTEM
    bool modsEnabled = g_prefsManager->GetInt( "ModSystemEnabled", 0 ) != 0;
#else
    bool modsEnabled = false;
#endif


    if( g_inputManager->controlEvent( ControlGamePause ) )
    {
		g_app->ToggleGamePaused();
	}

//    if (g_keyDeltas[KEY_F2]) DebugKeyBindings::DebugCameraButton();
#ifdef LOCATION_EDITOR
    /*if( modsEnabled )
    {   
        if ( g_inputManager->controlEvent( ControlToggleEditor ) ) DebugKeyBindings::EditorButton();
    }*/
#endif
//
#ifdef CHEATMENU_ENABLED
    if( EclGetWindows()->Size() == 0 )
    {
        if( g_inputManager->controlEvent( ControlToggleCheatMenu ) ) DebugKeyBindings::CheatButton();
    }
#endif

//
//    if (g_keyDeltas[KEY_F5]) DebugKeyBindings::FPSButton();
//#ifdef PROFILER_ENABLED
//	if (g_keyDeltas[KEY_F10]) DebugKeyBindings::ProfileButton();
//#endif // PROFILER_ENABLED

#ifdef NETWORK_STATS_ENABLED
    if( g_inputManager->controlEvent( ControlToggleNetworkStats ) ) DebugKeyBindings::NetworkButton();
#endif

#ifdef SOUND_EDITOR
    if( modsEnabled )
    {
//        if (g_keyDeltas[KEY_F7]) DebugKeyBindings::SoundStatsButton();
//        if (g_keyDeltas[KEY_F11]) DebugKeyBindings::SoundEditorButton();
//        if (g_keyDeltas[KEY_F9]) DebugKeyBindings::SoundProfileButton();
    }
#endif // SOUND_EDITOR
    
    END_PROFILE( "Advance UserInput");
}


// *** Render
void UserInput::Render()
{
    START_PROFILE( "Render UserInput");

    //
    // Render 2D overlays

    g_editorFont.BeginText2D();


	//
	// Eclipse

    glEnable    ( GL_BLEND );
    glDisable   ( GL_CULL_FACE );
    glDisable   ( GL_DEPTH_TEST );
    glDepthMask ( false );

    EclRender();
    
    glDepthMask ( true );
    glEnable    ( GL_DEPTH_TEST );
    glEnable    ( GL_CULL_FACE );
    glDisable   ( GL_BLEND );


    g_editorFont.EndText2D();

	
    //
    // Render 3d mouse history
    
//    glEnable    ( GL_BLEND );
//    glEnable    ( GL_LINE_SMOOTH );
//    glDisable   ( GL_DEPTH_TEST );
//    glLineWidth ( 5.0 );
//    glBegin     ( GL_LINE_STRIP );   
    
//    for( int i = 0; i < m_mousePosHistory.Size(); ++i )
//    {
//        float alpha = 1.0 - ((float) i / (float) m_mousePosHistory.Size());
//        alpha *= 0.5;
//        glColor4f( 1.0, 1.0, 0.0, alpha );
//        Vector3 *thisPos = m_mousePosHistory[i];
//        glVertex3dv( thisPos->GetData() );
//    }
    
//    glEnd       ();
//    glEnable    ( GL_DEPTH_TEST );
//    glDisable   ( GL_LINE_SMOOTH );
//    glDisable   ( GL_BLEND );

    END_PROFILE( "Render UserInput");
}


// *** GetMousePos3d
Vector3 UserInput::GetMousePos3d()
{
	return m_mousePos3d;
}



// *** RecalcMousePos3d
void UserInput::RecalcMousePos3d()
{
	//if(g_app->m_spectator)
	//{
	//	return;
	//}

	// Get click ray
    Vector3 rayStart;
    Vector3 rayDir;
    g_app->m_camera->GetClickRay( g_target->X(), g_target->Y(), &rayStart, &rayDir );

	ASSERT_VECTOR3_IS_SANE(rayStart);
	ASSERT_VECTOR3_IS_SANE(rayDir);
	rayStart += rayDir * 0.0;

    
    bool landscapeHit = false;
	if (g_app->m_location)
	{
		landscapeHit = g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &m_mousePos3d );
	}
    else
    {
        // We are in the global world
        // So hit against the outer sphere

		Vector3 sphereCentre(0,0,0);
		float sphereRadius = 36000.0;

		rayStart += rayDir * (sphereRadius * 4.0);
		rayDir = -rayDir;
		landscapeHit = RaySphereIntersection(rayStart, rayDir, 
			    							 sphereCentre, sphereRadius, 1e10,
				    						 &m_mousePos3d);
        return;
    }


	if (!landscapeHit)
	{
		// OK, we didn't hit against the landscape mesh, so hit against a sphere that 
		// encloses the whole world
		Vector3 sphereCentre;
		sphereCentre.x = g_app->m_globalWorld->GetSize() * 0.5;
		sphereCentre.y = 0.0;
		sphereCentre.z = g_app->m_globalWorld->GetSize() * 0.5;

		float sphereRadius = g_app->m_globalWorld->GetSize() * 40.0;

        float dist = (rayStart - sphereCentre).Mag();
        //AppDebugAssert(dist < sphereRadius);

		rayStart += rayDir * (sphereRadius * 4.0);
		rayDir = -rayDir;
		landscapeHit = RaySphereIntersection(rayStart, rayDir, 
											 sphereCentre, sphereRadius, 1e10,
											 &m_mousePos3d);
		//AppDebugAssert(landscapeHit);
	}
}
