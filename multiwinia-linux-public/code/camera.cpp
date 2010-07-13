#include "lib/universal_include.h"

#include <math.h>
#include <string.h>
#include <float.h>

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/input/input.h"
#include "lib/input/movement2d.h"
#include "lib/targetcursor.h"
#include "lib/window_manager.h"
#include "lib/math_utils.h"
#include "lib/matrix33.h"
#include "lib/debug_render.h"
#include "lib/math/random_number.h"
#include "lib/profiler.h"

#include "lib/preferences.h"
#include "interface/prefs_other_window.h"

#include "eclipse.h"

#include "network/clienttoserver.h"

#include "app.h"
#include "camera.h"
#include "global_world.h"
#include "level_file.h"
#include "location.h"
#include "main.h"
#include "renderer.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "team.h"
#include "unit.h"
#include "user_input.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "helpsystem.h"
#endif
#include "script.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "gamecursor.h"
#include "control_help.h"
#include "shaman_interface.h"
#include "multiwiniahelp.h"

#include "loaders/credits_loader.h"

#include "worldobject/teleport.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/radardish.h"
#include "entity_grid.h"



#define MIN_GROUND_CLEARANCE	10.0f	// Minimum height relative to land
#define MIN_HEIGHT				10.0f	// Height above sea level (which is y=0)
#define MAX_HEIGHT				5000.0f // Height above sea level (which is y=0)
#define MIN_TRACKING_HEIGHT     200.0f  // Minimum height of the camera when tracking an entity


std::vector<WorldObjectId> Camera::s_neighbours;


// ***************
// Private Methods
// ***************

void Camera::AdvanceDebugMode()
{
	//if( strcmp( EclGetCurrentFocus(), "none" ) != 0 && !g_app->m_editing ) return;

    if( g_app->m_editing )
    {
        m_targetFov = 60.0f;
    }
        
	float advanceTime = g_advanceTime;
	Vector3 right = m_front ^ m_up;

    float speedSideways = g_app->m_globalWorld->GetSize() / 30.0f;
	if (g_app->m_locationId != -1)
	{
		speedSideways = g_app->m_location->m_landscape.GetWorldSizeX() / 30.0f;
	}    
    float speedVertical = speedSideways;
    float speedForwards = speedSideways;

    if ( g_inputManager->controlEvent( ControlCameraSpeedup ) )
    {
        speedSideways *= 10.0f;
        speedVertical *= 10.0f;
        speedForwards *= 10.0f;
    }
    else if ( g_inputManager->controlEvent( ControlCameraSlowdown ) )
    {
        speedSideways *= 0.1f;
        speedVertical /= 10.0f;
        speedForwards /= 10.0f;
    }

    //speedForwards *= 20.0f;
	// TODO: Support mouse/joystick
    if( EclGetWindows()->Size() == 0 || g_app->m_editing)
    {
	    static DPadMovement cam_slide( ControlCameraForwards, ControlCameraBackwards,
	                                   ControlCameraLeft, ControlCameraRight, ControlCameraUp,
	                                   ControlCameraDown, 1 );
	    cam_slide.Advance();
	    m_pos -= right   * ( cam_slide.signX() * advanceTime * speedSideways );
	    m_pos += m_up    * ( cam_slide.signZ() * advanceTime * speedVertical );
	    m_pos += m_front * ( cam_slide.signY() * advanceTime * speedForwards );
    }

	int mx = g_target->dX();
	int my = g_target->dY();

	// TODO: Really?
    if ( g_inputManager->controlEvent( ControlCameraDebugRotate ) )
    {
        Matrix33 mat(1);
		mat.RotateAroundY((float)mx * -0.005f);
		m_up = m_up * mat;
		m_front = m_front * mat;

        Vector3 right = GetRight();
		mat.SetToIdentity();
		mat.FastRotateAround(right, (float)my * -0.005f);
		m_up = m_up * mat;
		m_front = m_front * mat;
    }
}


void Camera::Get2DScreenPos (Vector3 const &_vector, float *_screenX, float *_screenY)
{
    double outX, outY, outZ;

    GLint viewport[4];
    double viewMatrix[16];
    double projMatrix[16];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    gluProject(_vector.x, _vector.y, _vector.z, 
        viewMatrix,
        projMatrix,
        viewport,
        &outX,
        &outY,
        &outZ);

    *_screenX = outX;
	*_screenY = outY;
}


void Camera::SetHeight(float _height)
{
    m_height = _height;
}


void Camera::SetFOV(float _fov)
{
    m_targetFov = _fov;
    m_fov = _fov;
}


void Camera::SetTargetFOV(float _fov)
{
    m_targetFov = _fov;
}


void Camera::AdvanceSphereWorldMode()
{	
    bool chatLog = ChatLogVisible();
    if( chatLog ) return;

	m_targetFov = 100.0f;
    
	int const screenH = g_app->m_renderer->ScreenH();
	int const screenW = g_app->m_renderer->ScreenW();

	Vector3 focusPos = Vector3(0, m_height * -400,0);

	// Set up viewing matrices
	glPushMatrix();
	SetupProjectionMatrix(g_app->m_renderer->GetNearPlane(), g_app->m_renderer->GetFarPlane());
	SetupModelviewMatrix();

    // Get the 2D mouse coordinates before we move the camera
	Vector3 mousePos3D = g_app->m_userInput->GetMousePos3d();
	float oldMouseX, oldMouseY;
	Get2DScreenPos(mousePos3D, &oldMouseX, &oldMouseY);
	oldMouseY = screenH - oldMouseY;

    InputDetails details;
    if( g_inputManager->controlEvent( ControlCameraMove, details ) )
    {
        g_target->SetMousePos( g_target->X() + details.x, g_target->Y() + details.y );
        g_app->m_userInput->RecalcMousePos3d();
        mousePos3D = g_app->m_userInput->GetMousePos3d();
        Get2DScreenPos(mousePos3D, &oldMouseX, &oldMouseY);
	    oldMouseY = screenH - oldMouseY;
    }

	float factor1 = 2.0f * g_advanceTime;
	float factor2 = 1.0f - factor1;

	// Update camera orientation 
    if (mousePos3D.MagSquared() > 1.0f)
    {
        Vector3 desiredFront = mousePos3D - m_pos;
        desiredFront.Normalise();

        m_front = m_front * factor2 + desiredFront * factor1;
        m_front.Normalise();
        Vector3 right = m_front ^ g_upVector;
		right.Normalise();
        m_up = right ^ m_front;
    }


	//
    // Move towards the idealPos

	factor1 /= 2.0f;
	factor2 = 1.0f - factor1;
    
    Vector3 idealPos = focusPos - m_front * 30000;
    if( idealPos.Mag() > 35000.0f ) idealPos.SetLength( 35000.0f );
    Vector3 toIdealPos = idealPos - m_pos;
	float distToIdealPos = toIdealPos.Mag();
	m_pos = idealPos * factor1 + m_pos * factor2;

    // Set up viewing matrices
	SetupModelviewMatrix();

	// Get the 2D mouse coordinates now that we have moved the camera
    float newMouseX, newMouseY;
    Get2DScreenPos(mousePos3D, &newMouseX, &newMouseY);
    newMouseY = screenH - newMouseY;

	// Calculate how much to move the cursor to make it look like it is
	// locked to a point on the landscape (we need to take account of
	// sub-pixel error from previous frames)
    static float mouseDeltaX = 0.0f;
    static float mouseDeltaY = 0.0f;
    mouseDeltaX += (newMouseX - oldMouseX) * 1.0f;
    mouseDeltaY += (newMouseY - oldMouseY) * 1.0f;
    int intMouseDeltaX = floorf(mouseDeltaX);
    int intMouseDeltaY = floorf(mouseDeltaY);
    mouseDeltaX -= intMouseDeltaX;
    mouseDeltaY -= intMouseDeltaY;
	newMouseX = g_target->X() + intMouseDeltaX;
	newMouseY = g_target->Y() + intMouseDeltaY;

	// Make sure these movements don't put the mouse cursor somewhere stupid
	if (newMouseX < 0)				newMouseX = 0;
	if (newMouseX >= screenW)		newMouseX = screenW - 1;
	if (newMouseY < 0)				newMouseY = 0;
	if (newMouseY >= screenH)		newMouseY = screenH - 1;

	// Apply the mouse cursor movement
	g_target->SetMousePos(newMouseX, newMouseY);
    glPopMatrix();
}


void Camera::AdvanceSphereWorldScriptedMode()
{
    int b = 10;

    m_height = 50.0f;
    
	int const screenH = g_app->m_renderer->ScreenH();
	int const screenW = g_app->m_renderer->ScreenW();

	Vector3 focusPos = Vector3(0, m_height * -400,0);
    focusPos.x += iv_sin( g_gameTime*0.5f ) * 4000.0f;
    focusPos.y += iv_cos( g_gameTime*0.4f ) * 2000.0f;
    focusPos.z += iv_sin( g_gameTime*0.3f ) * 4000.0f;

	// Set up viewing matrices
	glPushMatrix();
	SetupProjectionMatrix(g_app->m_renderer->GetNearPlane(), g_app->m_renderer->GetFarPlane());
	SetupModelviewMatrix();

    // Get the 2D mouse coordinates before we move the camera
	Vector3 mousePos3D = g_app->m_userInput->GetMousePos3d();
	float oldMouseX, oldMouseY;
	Get2DScreenPos(mousePos3D, &oldMouseX, &oldMouseY);
	oldMouseY = screenH - oldMouseY;

	float factor1 = 1.0f * g_advanceTime;
	float factor2 = 1.0f - factor1;

	// Update camera orientation 
    Vector3 desiredFront = m_targetPos - m_pos;
    desiredFront.Normalise();

    m_front = m_front * factor2 + desiredFront * factor1;
    m_front.Normalise();
    Vector3 right = m_front ^ g_upVector;
	right.Normalise();
    m_up = right ^ m_front;


	//
    // Move towards the idealPos

	factor1 /= 2.0f;
	factor2 = 1.0f - factor1;
    
    Vector3 idealPos = focusPos - m_front * 30000;
    if( idealPos.Mag() > 35000.0f ) idealPos.SetLength( 35000.0f );
    Vector3 toIdealPos = idealPos - m_pos;
	float distToIdealPos = toIdealPos.Mag();
	m_pos = idealPos * factor1 + m_pos * factor2;

    glPopMatrix();
}


/*
void Camera::AdvanceSphereWorldIntroMode()
{
    m_targetFov = 100.0f;

    static float fixMeUp = 45000.0f;

    if( g_keys[KEY_G] )
    {
        fixMeUp = 45000.0;
    }

    if( fixMeUp == 45000.0f )
    {
        m_pos.Set( 0, 0, -1000000.0f );
        m_front.Set( 1, 0, -1 );
        m_up.Set( 1, 0, 0 );
    }
    
    fixMeUp -= g_advanceTime * 500.0f;

    Vector3 targetFront = Vector3(200, 200, 200) - m_pos;
    float distance = targetFront.Mag();
    
    float forwardSpeed = 3000.0f;
    
    targetFront.Normalise();

    float rotateSpeed = forwardSpeed / (fixMeUp*0.66f);
    float factor1 = g_advanceTime * rotateSpeed;
    float factor2 = 1.0f - factor1;
    m_front = m_front * factor2 + targetFront * factor1;
    m_up.RotateAround( m_front * g_advanceTime * rotateSpeed * 0.2f );

    m_pos += m_front * forwardSpeed;
}*/


void Camera::AdvanceSphereWorldIntroMode()
{
    m_targetFov = 70.0f;

    static bool fixMeUp = true;
    static double startTime = GetHighResTime();
    static double lastFrame = startTime;

    float timeNow = GetHighResTime();
    float frameTime = timeNow - lastFrame;
    float runningTime = timeNow - startTime;
    lastFrame = timeNow;

#ifdef _DEBUG
    if( g_inputManager->controlEvent( ControlDebugCameraFixUp ) )
    {
        fixMeUp = true;
    }
#endif

    if( fixMeUp )
    {
        startTime = GetHighResTime();

        m_pos.Set( -965852, 1720000, 2600000 );
        m_front.Set( -1, 0, 0 );
        m_up.Set( 0.15,0.93, 0.31 );
        m_front.Normalise();
        m_up.Normalise();
        
        fixMeUp = false;
    }
    
    Vector3 targetPos = g_zeroVector;
    if( runningTime < 30.0f )
    {
        targetPos.Set( -1000000, 4000000, 397000 );
    }
    else
    {
        targetPos.Set( 1000, 500, 1000 );
    }        
    Vector3 targetFront = targetPos - m_pos;
    float distance = targetFront.Mag();
    
    float forwardSpeed = 3000.0f;
    
    if( distance < 2000000 && runningTime > 30 )
    {
        forwardSpeed = (distance/1070000.0f) * 3000.0f;
    }

    targetFront.Normalise();

    float rotateSpeed = 4000.0f / 30000.0f;
    float factor1 = frameTime * rotateSpeed;
    float factor2 = 1.0f - factor1;
    m_front = m_front * factor2 + targetFront * factor1;
    m_up.RotateAround( m_front * frameTime * rotateSpeed * iv_sin(runningTime*0.1f+0.4f) * 1.6f );

    m_pos += m_front * forwardSpeed * frameTime * 62;

}



void Camera::AdvanceSphereWorldOutroMode()
{
    m_targetFov = 70;

    static bool fixMeUp = true;
    static double startTime = GetHighResTime();
    static double lastFrame = startTime;

    double timeNow = GetHighResTime();
    double frameTime = timeNow - lastFrame;
    double runningTime = timeNow - startTime;
    lastFrame = timeNow;

#ifdef _DEBUG
    if( g_inputManager->controlEvent( ControlDebugCameraFixUp ) )
    {
        fixMeUp = true;
    }
#endif

    if( fixMeUp )
    {
        startTime = GetHighResTime();

        m_pos.Set( 0, 0, 0 );
        m_front.Set( 0, 1, 0 );
        m_up.Set( 0, 0, 1 );
        m_front.Normalise();
        m_up.Normalise();
        
        m_vel = m_front;
        
        fixMeUp = false;
    }
    
    Vector3 targetPos( -1000000, 4000000, 397000 );

    if( runningTime > 30.0f )
    {
        targetPos.Set( -965852, 1720000, 2600000 );
    }

    Vector3 targetFront = (targetPos - m_pos) * -1.0f;
    float distance = targetFront.Mag();
    
    float forwardSpeed = iv_sqrt(m_pos.Mag()) * 4;
    forwardSpeed = max( forwardSpeed, 1000.0f );
    forwardSpeed = min( forwardSpeed, 2000.0f );
    
    targetFront.Normalise();

    float rotateSpeed = 4000.0f / 30000.0f;
    float factor1 = frameTime * rotateSpeed;
    float factor2 = 1.0f - factor1;
    m_vel = m_vel * factor2 + targetFront * factor1;
    m_up.RotateAround( m_front * frameTime * rotateSpeed * iv_sin(runningTime*0.1f+0.4f) * 0.6f );

    m_pos -= m_vel * forwardSpeed * frameTime * 62;

    targetFront = -m_pos;
    if( runningTime > 30.0f )
    {
        targetFront = m_vel;
    }

    targetFront.Normalise();
    factor1 *= 5.0f;
    m_front = m_front * factor2 + targetFront * factor1;

    if( runningTime > 70.0f )
    {
        int loaderType = Loader::GetLoaderIndex("credits");
        if( loaderType != -1 )
        {
            Loader *loader = Loader::CreateLoader( loaderType );
            loader->Run();
            delete loader;
        }

        RequestMode( ModeSphereWorld );
        m_pos.Set(0,0,100);
    }
}


void Camera::RequestSphereFocusMode()
{
	m_framesInThisMode = 0;
    m_mode = ModeSphereWorldFocus;
    m_targetPos.Zero();
    m_trackRange = 100000.0f;
    m_trackHeight = 0.0f;
    m_trackTimer = GetHighResTime();

    m_trackVector = ( m_pos );
    m_trackVector.y = 0.0f;
    m_trackVector.Normalise();
}


void Camera::AdvanceSphereWorldFocusMode()
{
    m_targetFov = 100;

    Vector3 oldPos = m_pos;
   
    m_trackRange = 100000;
    m_trackHeight = 50000;
    
    //
    // Target a point that slowly rotates around the building

    m_trackVector.RotateAroundY( g_advanceTime * -0.15f );
    m_trackVector.Normalise();
    float height = iv_sin( g_gameTime * 0.2f ) * m_trackHeight;
    float trackRange = iv_abs(m_trackRange) + iv_sin( g_gameTime * 0.3f ) * fabs(m_trackRange) * 0.4f;
    Vector3 realTargetPos = m_targetPos + m_trackVector * trackRange;           
    realTargetPos.y = -110000 + height;

    // Calculate a Movement Factor, so we start moving slowly towards our target,
    // then eventually reach full speed
    double timeSinceBegin = GetHighResTime() - m_trackTimer;
    float moveFactor = timeSinceBegin * 0.2f;
    moveFactor = min( moveFactor, 1.0f );

    float factor1 = moveFactor * 0.5f * g_advanceTime;    
    float factor2 = 1.0f - factor1;
    m_pos = m_pos * factor2 + realTargetPos * factor1;
        
    m_targetPos.y = 10000;
    
    Vector3 targetFront = ( m_targetPos - m_pos ).Normalise();
    if( m_trackRange < 0.0f ) 
    {
        targetFront.x *= -1.0f;
        targetFront.z *= -1.0f;
    }
	factor1 = moveFactor * 1.0f * g_advanceTime * 0.5f;
	factor2 = 1.0f - factor1;
    m_front = m_front * factor2 + targetFront * factor1;
    
    m_up.Set( 0.0f, -1.0f, 0.0f );
    Vector3 right = m_up ^ m_front;
    m_up = right ^ m_front;
}



// Returns the number of meters to the nearest blockage that the camera would
// experience if it travelled in the specified direction. A blockage is defined
// as a piece of land more than 10 metres higher than the camera's current 
// height. If there is no blockage FLT_MAX is returned.
float Camera::DistanceToBlockage(Vector3 const &_dir, float const _maxDist)
{
    if( !g_app->m_location ) return FLT_MAX;

	unsigned int const numSteps = 40;
	float const distStep = _maxDist / (float)numSteps;
	for (int i = 1; i <= numSteps; ++i)
	{
		float x = m_pos.x + _dir.x * distStep * (float)i;
		float z = m_pos.z + _dir.z * distStep * (float)i;
		float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(x, z);
		if (landHeight + MIN_GROUND_CLEARANCE > m_height)
		{
			return (float)i * distStep;
		}
	}

	return _maxDist;
}

void Camera::AdvanceFreeMovementMode()
{
	if (g_app->m_renderer->m_renderingPoster)
	{
		return;
	}

	UpdateEntityTrackingMode();

	// Check to see whether we should switch to entity tracking mode
	WorldObjectId selection;
	if (m_entityTrack && GetEntityToTrack(selection)) 
	{
		Entity *entity = g_app->m_location->GetEntity( selection );
		if( entity->m_type == Entity::TypeInsertionSquadie )
		{
			RequestEntityTrackMode( selection );
			return;
		}
		else
		{
			m_objectId = WorldObjectId();
		}
	}


	int screenW = g_app->m_renderer->ScreenW();
	int screenH = g_app->m_renderer->ScreenH();
	InputManager *im = g_inputManager;

	// Set up viewing matrices
	glPushMatrix();
	SetupProjectionMatrix(g_app->m_renderer->GetNearPlane(), g_app->m_renderer->GetFarPlane());
	SetupModelviewMatrix();

    // Get the 2D mouse coordinates before we move the camera
	Vector3 mousePos3D = g_app->m_userInput->GetMousePos3d();
	float oldMouseX, oldMouseY;
	Get2DScreenPos(mousePos3D, &oldMouseX, &oldMouseY);
	oldMouseY = screenH - oldMouseY;

    // Allow quake keys to move us
    if( !g_app->m_taskManagerInterface->IsVisible()  &&
		!g_app->m_shamanInterface->IsVisible() )
	{
		float moveRate = 250.0f;
		Vector3 accelForward = m_front;
		accelForward.y = 0.0f;
		accelForward.Normalise();
		Vector3 accelRight = GetRight();
		accelRight.y = 0.0f;
		accelRight.Normalise();
        Vector3 accelUp = g_upVector;

		// TODO: Support mouse/joystick
        if( g_inputManager->controlEvent( ControlCameraSpeedup ) ) moveRate *= 4.0f;
        
        bool keyForward     = g_inputManager->controlEvent( ControlCameraForwards );
        bool keyBackward    = g_inputManager->controlEvent( ControlCameraBackwards );
        bool keyLeft        = g_inputManager->controlEvent( ControlCameraLeft );
        bool keyRight       = g_inputManager->controlEvent( ControlCameraRight );

        double timeNow = GetHighResTime();
        static double s_lastAdvance = timeNow;

        float advanceTime = timeNow - s_lastAdvance;
        s_lastAdvance = timeNow;
        if( advanceTime > 1.0f ) advanceTime = 1.0f;

        if (keyLeft)		m_targetPos += accelRight * advanceTime * moveRate;
		if (keyRight)		m_targetPos -= accelRight * advanceTime * moveRate;
		if (keyForward)     m_targetPos += accelForward * advanceTime * moveRate;
		if (keyBackward)	m_targetPos -= accelForward * advanceTime * moveRate;

        if( m_mode == ModeFreeMovement )
        {
            InputDetails details;
            if( g_inputManager->controlEvent( ControlCameraMove, details ) )
            {
                m_targetPos -= accelRight * advanceTime * details.x * 10.0f;
                m_targetPos -= accelForward * advanceTime * details.y * 10.0f;

				g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondMoveCameraOrUnit);
            }
        }

        //
        // If the user holds down forwards for a long time without using shift,
        // Tell him he can speed up if he wants

        static double s_forwardHeldDown = -1.0f;

        if( keyForward && !keyLeft && !keyRight && !keyBackward &&
            !g_inputManager->controlEvent( ControlCameraSpeedup ) )
        {
            double timeNow = GetHighResTime();
            if( s_forwardHeldDown < 0.0f ) s_forwardHeldDown = timeNow;
            if( timeNow > s_forwardHeldDown + 4.0f )
            {
                g_app->m_multiwiniaHelp->ShowShiftSpeedupHelp();
            }
        }
        else
        {
            s_forwardHeldDown = -1.0f;
        }

        if( keyForward || keyBackward || keyLeft || keyRight )
        {
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
            g_app->m_helpSystem->PlayerDoneAction( HelpSystem::CameraMovement );
#endif
        }

//		if (m_pos.x < m_minX)	m_targetPos.x -= (m_pos.x - m_minX);
//		if (m_pos.x > m_maxX)	m_targetPos.x -= (m_pos.x - m_maxX);
//		if (m_pos.z < m_minX)	m_targetPos.z -= (m_pos.z - m_minZ);
//		if (m_pos.z > m_maxX)	m_targetPos.z -= (m_pos.z - m_maxZ);

		// Stop camera getting too close to cliffs
		{
			Vector3 horiDir = m_targetPos - m_pos;
			horiDir.y = 0.0f;
			if (horiDir.Mag() > 1e-4)
			{
				horiDir.Normalise();
				float const distToBlockage = DistanceToBlockage(horiDir, 100.0f);
				if (distToBlockage < 100.0f)
				{
					Vector3 targetVector = m_targetPos - m_pos;
					Vector3 targetVectorHori = targetVector;
					targetVectorHori.y = 0.0f;
					float maxSpeed = (distToBlockage - 30.0f) / (100.0f - 30.0f);
					if (maxSpeed < 0.0f) maxSpeed = 0.0f;
					targetVector *= maxSpeed;
					m_targetPos = m_pos + targetVector;
					m_height += (1.0f - maxSpeed) * 1.0f * targetVectorHori.Mag();
				}
			}
		}

		// Make sure we haven't set the height too low
		float const hitDownRadius = 10.0f;
		float landheight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_targetPos.x, m_targetPos.z);
		float landHeight2 = g_app->m_location->m_landscape.m_heightMap->GetValue(m_targetPos.x - hitDownRadius, m_targetPos.z - hitDownRadius);
		if (landHeight2 > landheight) landheight = landHeight2;
		landHeight2 = g_app->m_location->m_landscape.m_heightMap->GetValue(m_targetPos.x + hitDownRadius, m_targetPos.z - hitDownRadius);
		if (landHeight2 > landheight) landheight = landHeight2;
		landHeight2 = g_app->m_location->m_landscape.m_heightMap->GetValue(m_targetPos.x - hitDownRadius, m_targetPos.z + hitDownRadius);
		if (landHeight2 > landheight) landheight = landHeight2;
		landHeight2 = g_app->m_location->m_landscape.m_heightMap->GetValue(m_targetPos.x + hitDownRadius, m_targetPos.z + hitDownRadius);
		if (landHeight2 > landheight) landheight = landHeight2;
		
		if (landheight < MIN_HEIGHT) landheight = MIN_HEIGHT;

		if (landheight + MIN_GROUND_CLEARANCE > m_height)
		{
			m_targetPos.y = landheight + MIN_GROUND_CLEARANCE;
			m_height = m_targetPos.y;
		}
		else
		{
			m_targetPos.y = m_height;
		}

		// Update our position
		float factor1 = 4.0f * advanceTime;
		float factor2 = 1.0f - factor1;
		Vector3 deltaPos = -m_pos;    
		Vector3 oldPos = m_pos;

		if( !m_camLookAround )
		{
			m_pos = m_pos * factor2 + m_targetPos * factor1;
		}
	    
		deltaPos = m_pos + deltaPos;
		m_vel = ( m_pos - oldPos ) / advanceTime;
	    
		// Update camera orientation 
		bool chatLog = ChatLogVisible();

		bool disableMouseControl = false;

		if( !disableMouseControl &&
			!g_app->m_taskManagerInterface->IsVisible()  		
			&& !chatLog	)
		{
			if (mousePos3D.MagSquared() > 1.0f)
			{
				Vector3 desiredFront = mousePos3D - m_pos;
				desiredFront.Normalise();
				Vector3 oldFront = m_front;
				Vector3 desiredRotation = oldFront ^ desiredFront;
				float amountToRotate = iv_sqrt(advanceTime) * 1.0f;
				if (amountToRotate > 1.0f) amountToRotate = 1.0f;
				desiredRotation *= amountToRotate;
				m_front.RotateAround(desiredRotation);
	//            m_front = m_front * factor2 + desiredFront * factor1;
				m_front.Normalise();
	                        
				Vector3 right = m_front ^ g_upVector;
				right.Normalise();
				m_up = right ^ m_front;
			}
		}
	    
		// Set up viewing matrices
		SetupModelviewMatrix();

		// Get the 2D mouse coordinates now that we have moved the camera
		float newMouseX, newMouseY;
		mousePos3D += deltaPos;
		Get2DScreenPos(mousePos3D, &newMouseX, &newMouseY);
		newMouseY = screenH - newMouseY;

		// Calculate how much to move the cursor to make it look like it is
		// locked to a point on the landscape (we need to take account of
		// sub-pixel error from previous frames)
		static float mouseDeltaX = 0.0f;
		static float mouseDeltaY = 0.0f;
		mouseDeltaX += (newMouseX - oldMouseX) * 1.0f;
		mouseDeltaY += (newMouseY - oldMouseY) * 1.0f;
		int intMouseDeltaX = floorf(mouseDeltaX);
		int intMouseDeltaY = floorf(mouseDeltaY);
		mouseDeltaX -= intMouseDeltaX;
		mouseDeltaY -= intMouseDeltaY;
		// g_inputManager->PollForEvents();
		newMouseX = g_target->X() + intMouseDeltaX;
		newMouseY = g_target->Y() + intMouseDeltaY;

		// Make sure these movements don't put the mouse cursor somewhere stupid
		if (newMouseX < 0)			newMouseX = 0;
		if (newMouseX >= screenW)	newMouseX = screenW - 1;
		if (newMouseY < 0)			newMouseY = 0;
		if (newMouseY >= screenH)	newMouseY = screenH - 1;

		// Apply the mouse cursor movement
		g_target->SetMousePos(newMouseX, newMouseY);
		glPopMatrix();
	}
}   


void Camera::Render()
{
//    RenderArrow( m_targetPos, m_targetPos + m_trackVector * m_trackRange, 1.0f );
//    RenderSphere( m_targetPos + m_trackVector * m_trackRange, 20.0f );
}


void Camera::AdvanceBuildingFocusMode()
{
    Vector3 oldPos = m_pos;

    //
    // Target a point that slowly rotates around the building

    m_trackVector.RotateAroundY( g_advanceTime * -0.15f );
    m_trackVector.Normalise();
    float height = m_trackHeight + iv_sin( g_gameTime * 0.3f ) * m_trackHeight * 0.5f;
    float trackRange = iv_abs(m_trackRange) + iv_sin( g_gameTime * 0.3f ) * fabs(m_trackRange) * 0.3f;
    Vector3 realTargetPos = m_targetPos + m_trackVector * trackRange;           
    realTargetPos.y = m_targetPos.y + height;

    // Calculate a Movement Factor, so we start moving slowly towards our target,
    // then eventually reach full speed
    float timeSinceBegin = GetHighResTime() - m_trackTimer;
    float moveFactor = timeSinceBegin * 1.0f;
    moveFactor = min( moveFactor, 1.0f );

    if( timeSinceBegin < 2.0f )
    {
        // Make the camera lift up when first moving towards a building
        float distance = ( m_pos - realTargetPos ).Mag();
        realTargetPos.y += distance * 0.75f * ( 2.0f - timeSinceBegin );
        realTargetPos.y = min( realTargetPos.y, 1000.0 );
    }

    float factor1 = moveFactor * 0.5f * g_advanceTime;    
    float factor2 = 1.0f - factor1;
    m_pos = m_pos * factor2 + realTargetPos * factor1;
        
    Vector3 targetFront = ( m_targetPos - m_pos ).Normalise();
    if( m_trackRange < 0.0f ) 
    {
        targetFront.x *= -1.0f;
        targetFront.z *= -1.0f;
    }
	factor1 = moveFactor * 1.0f * g_advanceTime;
	factor2 = 1.0f - factor1;
    m_front = m_front * factor2 + targetFront * factor1;
    
    m_up.Set( 0.0f, -1.0f, 0.0f );
    Vector3 right = m_up ^ m_front;
    m_up = right ^ m_front;

    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    if( m_pos.y < landHeight + 10.0f )
    {
        m_pos.y = landHeight + 10.0f;
    }
}

// Determine whether there is an entity or unit that has been selected
// by the player, and if so, return true and fill in the object id.
bool Camera::GetEntityToTrack( WorldObjectId &selection )
{
	if( g_inputManager->controlEvent( ControlCameraFreeMovement ) ) 
		return false;

	if( !g_app->m_location )
		return false;    

    Team *team = g_app->m_location->GetMyTeam();

	if( !team )
		return false;

	Entity *teamEntity = team->GetMyEntity();

	if( teamEntity )
    {
        selection = teamEntity->m_id;
		return true;
    }

    Task *currentTask = team->m_taskManager->GetCurrentTask();
    // if the task has just been ended or killed, it isnt valid
    if (currentTask && currentTask->m_state == Task::StateStopping )
        return false;

    if( !currentTask )
        return false;

	Unit *unit = team->GetMyUnit();
    if( !unit )
		return false;

	if( unit->m_troopType == Entity::TypeInsertionSquadie ) 
	{
		InsertionSquad *squad = (InsertionSquad *) unit;
		Entity *pointMan = squad->GetPointMan();
		if (pointMan) {
			selection = pointMan->m_id;
			return true;
		}
	}

	return false;
}

void Camera::AdvanceAutomaticTracking()
{
	if( !m_resettingCameraPos ) m_resettingCameraPos = g_inputManager->controlEvent( ControlCameraZoom );

	float distChange = g_advanceTime*10;
	if( g_inputManager->controlEvent( ControlCameraSquadZoomIn ) ) 
	{
		m_trackingCameraDist = max(m_trackingCameraDist-distChange,50.0f);
	}
	if( g_inputManager->controlEvent( ControlCameraSquadZoomOut ) ) 
	{
		m_trackingCameraDist = min( m_trackingCameraDist+distChange,500.0f);
	}
	
	int acceleration = m_resettingCameraPos ? 3 : 1;

	float factor1 = (g_advanceTime * 4.0f * acceleration);
	if( factor1 > 1.0f ) factor1 = 1.0f;
	float factor2 = 1.0f - factor1;

	Vector3 avgCameraTarget;
	int numInputs = 0;

	Vector3 cameraTarget;
	
	if (AdvanceCameraMoveBehind( cameraTarget )) {
		avgCameraTarget += cameraTarget;
		numInputs++;
	}

	/*
	if (AdvanceCanSeeUnits( cameraTarget )) {
		avgCameraTarget += cameraTarget;
		numInputs++;
	}

	if (AdvanceNotTooLow( cameraTarget )) {
		avgCameraTarget += cameraTarget;
		numInputs++;
	}

	if (AdvanceNotTooFarAway( cameraTarget )) {
		avgCameraTarget += cameraTarget;
		numInputs++;
	}

	if (AdvanceManualRotateCamera( cameraTarget )) {
		avgCameraTarget += 3 * cameraTarget;
		numInputs += 3;
	}

	if( AdvanceManualCameraHeight ( cameraTarget )) 
	{
		avgCameraTarget += cameraTarget;
		numInputs++;
	}
	*/

	if (numInputs > 0) 
		avgCameraTarget = avgCameraTarget / float(numInputs);
	else
		avgCameraTarget = m_pos;

	m_cameraTarget = factor2 * m_cameraTarget + factor1 * avgCameraTarget;

	Vector3 diff = m_cameraTarget - m_currentEntityTrackPos;
	diff.y = 0;
	diff.SetLength(m_trackingCameraDist);
	m_cameraTarget = m_currentEntityTrackPos + diff;
	m_cameraTarget.y = m_trackingCameraDist;

	m_pos = factor2 * m_pos + factor1 * m_cameraTarget;

	if( m_resettingCameraPos && 
		m_pos.x - m_wantedPos.x < 5.0f &&
		m_pos.y - m_wantedPos.y < 5.0f &&
		m_pos.z - m_wantedPos.z < 5.0f )
	{
		AppDebugOut("Stopping camera reset\n");
		m_resettingCameraPos = false;
	}

	// Finally face the unit
    RotateTowardsEntity( m_trackingEntity );
	UpdateControlVector();
}
void Camera::UpdateControlVector()
{
	const float angleThreshold = 0.98f;

	bool recalc = false;
	if( g_inputManager->controlEvent( ControlUnitMoveDirectionChange ) )
	{
		m_skipDirectionCalculation = false;
		recalc = true;
	}

	float angle = iv_cos( g_upVector * GetUp() );
	if((iv_abs(angle) < angleThreshold  && !m_skipDirectionCalculation ) ||
		!g_inputManager->controlEvent( ControlUnitMove ) ||
		recalc )
	{
		m_controlVector = GetRight();
		m_skipDirectionCalculation = false;
	}
	else if( iv_abs(angle) >= angleThreshold  )
	{
		m_skipDirectionCalculation = true;
	}
}

bool Camera::TargetEntity(short _x, short _y)
{
	if( !m_trackingEntity || m_trackingEntity->m_type != Entity::TypeInsertionSquadie )
	{
		return false;
	}

	bool targetted = false;

	float range = 225.0f;// 150.0f (range of lasers) * 1.5

	Vector3 temp = m_trackingEntity->m_pos - m_pos;
	m_targetVector.x = _x;
	m_targetVector.y = 0.0f;
	m_targetVector.z = _y;

	m_targetVector.Normalise();
	m_targetVector.SetLength(range);
	m_targetVector.RotateAroundY(asin(temp.x / temp.MagSquared()));

	// Already targetting something, so check for nudges in each direction
	if( m_targettingEntity )
	{
	}

	// EntityGrid search
	if( !targetted )
	{
		int numFound = -1;

		g_app->m_location->m_entityGrid->GetEnemies(
			s_neighbours,
			m_trackingEntity->m_pos.x,
			m_trackingEntity->m_pos.z,
			range,
			&numFound,
			g_app->m_globalWorld->m_myTeamId);

		Vector3 temp = m_targetVector;
		temp.SetLength(m_targetPosOffset);
		int xPos = m_trackingEntity->m_pos.x + temp.x;
		int zPos = m_trackingEntity->m_pos.z + temp.z;

		Entity* closestE = NULL;
		float closestX = -1, closestZ = -1;
		
		for( int i = 0; i < numFound; i++ )
		{
			// Cull down with FOV angle
			Entity *e = g_app->m_location->GetEntity(s_neighbours[i]);
			if( !e ) 
			{
				continue;
			}

			Vector3 s = m_targetVector;
			Vector3 t = e->m_pos - m_trackingEntity->m_pos;
			s.Normalise();
			t.Normalise();
			float cosAngle = s * t;

			if( cosAngle < m_targettingFOVAngle )
			{
				continue;
			}

			// Find closest
			if( closestE )
			{
				int x1 = xPos - closestE->m_pos.x,
					z1 = zPos - closestE->m_pos.z,
					x2 = xPos - e->m_pos.x,
					z2 = zPos - e->m_pos.z;

				if( x1*x1 + z1*z1 > x2*x2 + z2*z2 )
				{
					closestE = e;
					m_targettingEntityId = s_neighbours[i];
				}
			}
			else
			{
				closestE = e;
				m_targettingEntityId = s_neighbours[i];
			}
		}

		targetted = m_targettingEntity = closestE;
	}

	if( !targetted )
	{
		// remove targetting vector and stuff
	}

	return targetted;
}

bool Camera::AdvanceManualRotateCamera( Vector3 &cameraTarget )
{
	if(g_inputManager->controlEvent( ControlCameraRotateLeft ) ||
       g_inputManager->controlEvent( ControlCameraRotateRight ) )
    {
        m_trackHeight = 0.0f;

        int rotSpeed = 100;

		int halfWidth = g_app->m_renderer->ScreenW() / 2;
		int deltaX = g_target->X() - halfWidth;
        if( g_inputManager->controlEvent( ControlCameraRotateLeft )  )
        {
            deltaX = rotSpeed;
        }

        if( g_inputManager->controlEvent( ControlCameraRotateRight ) )
        {
            deltaX = -rotSpeed;
        }

        float rotY = (float)deltaX * 0.015f;

		// Disable vertical camera adjustment, for now

		 int halfHeight = g_app->m_renderer->ScreenH() / 2;
		 int deltaY = g_target->Y() - halfHeight;
         float rotRight = (float)deltaY * -0.01f;

		Vector3 front = m_front;
        front.RotateAroundY(rotY);

        Vector3 right = (front ^ g_upVector).Normalise();
        
		Vector3 newUp = m_up;

        newUp.RotateAround(right * rotRight);
        if (newUp.y > 0.1f)
        {
            front.RotateAround(right * rotRight);
        }        

        cameraTarget = m_targetPos - front * m_currentDistance;
		return true;
    }
	else {
		m_currentDistance = (m_pos - m_predictedEntityPos).Mag();
		return false;
	}

}

bool AimingBuilding()
{
	Team* team = NULL;
	Building *building = NULL;

	if( !g_app || !g_app->m_location || !(team = g_app->m_location->GetMyTeam()) ) return false;

	if( team && team->m_currentBuildingId != -1 )
		if( (building = g_app->m_location->GetBuilding( team->m_currentBuildingId )) )
			return building->m_type == Building::TypeGunTurret || building->m_type == Building::TypeRadarDish;

	return false;
}

bool FiddlingWithOfficer()
{
	bool retVal = false;
	/*AppDebugOut("FIDDLING: %s, %s, %s\n", 
		g_app->m_location ? "location" : "no location",
		g_app->m_globalWorld ? "global world" : "no global world",
		g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD ? "gamepad" : "not gamepad");
	*/if( !g_app->m_location || 
		!g_app->m_globalWorld ||
		g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD )
	{
		retVal = false;
	}
	else
	{
		Entity* entity = g_app->m_location->m_teams[g_app->m_globalWorld->m_myTeamId]->GetMyEntity();
		retVal =  entity && entity->m_type == Entity::TypeOfficer;
	}

	static bool lastRet = retVal;
	static bool firstTime = true;
	if( firstTime )
	{
		AppDebugOut("Return value of fiddlingwithofficer is now %s\n", retVal ? "true" : "false");
		firstTime = false;
	}
	else if( lastRet != retVal )
	{
		AppDebugOut("Return value of fiddlingwithofficer is now %s\n", retVal ? "true" : "false");
		lastRet = retVal;
	}
	return retVal;
}

bool Camera::AdvanceManualCameraHeight( Vector3 &cameraTarget )
{
    if( g_app->m_location->GetMyTeam() && g_app->m_location->GetMyTeam()->GetMyEntity() ) return false;
	const float heightScale = 0.05f;

	bool camDown = false;

    if( EclGetWindows()->Size() == 0 )
    {
		
	    if( g_inputManager->controlEvent( ControlCameraUp ) && !FiddlingWithOfficer() && !AimingBuilding() )
	    {
		    m_heightMultiplier += heightScale;
		    g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondCameraUp);
		    g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondCameraDown);
	    }

	    if( g_inputManager->controlEvent( ControlCameraDown ) && !AimingBuilding() )
	    {
		    m_heightMultiplier -= heightScale;
		    camDown = true;
		    g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondCameraUp);
		    g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondCameraDown);
	    }

	    m_heightMultiplier = min( 2.0f, m_heightMultiplier );
	    m_heightMultiplier = max( m_heightMultiplier, 0.25f );

	    if( camDown )
	    {
		    cameraTarget = m_pos;
		    cameraTarget.y -= 25.0f;
		    return true;
	    }
    }

	return false;	
}

bool Camera::AdvanceCameraMoveBehind( Vector3 &cameraTarget )
{
	if( m_trackingEntity->m_type != Entity::TypeInsertionSquadie ) return false;

	static LList<Vector3*> vectors;

	cameraTarget = m_wantedPos == g_zeroVector ? m_pos : m_wantedPos;

	Squadie* squadie = (Squadie*) m_trackingEntity;
	// We are tracking on a squad
	if( !squadie )
	{
		return false;
	}

	// And we are auto-aiming at an enemy
	if( false && m_targettingEntity )
	{
		// Move the camera so that targetted enemy and squad are always on screen.
		Vector3 dir(m_targettingEntity->m_pos - m_trackingEntity->m_pos);
		dir.y = 0.0f;
		dir.SetLength(m_trackingCameraDist);
		cameraTarget = m_trackingEntity->m_pos - dir;
		cameraTarget.y = m_trackingCameraDist;
	}
	// We're just walking around
	else
	{
		// so make the camera move behind us
		// unless the squad is walking directly towards us (in a 90degree arc)
		if( squadie->m_id != m_lastSquadieId )
		{
			m_lastEntityTrackPos = m_pos;
			m_lastEntityTrackPos.y = 0.0f;
			m_lastSquadieId = squadie->m_id;
		}
		else
		{
			//m_lastEntityTrackPos = m_currentEntityTrackPos;
		}

		m_currentEntityTrackPos = squadie->m_pos;
		m_currentEntityTrackPos.y = 0.0f;

		if( (m_lastEntityTrackPos-m_currentEntityTrackPos).MagSquared() > 10 )
		{
			// Check to see if the squad is moving towards us within 90degrees.
			Vector3 angleCheckVector = m_currentEntityTrackPos-m_lastEntityTrackPos;
			Vector3 camVector = m_wantedPos-m_lastEntityTrackPos;
			angleCheckVector.Normalise();
			camVector.Normalise();
			m_camEntDiffAngle = angleCheckVector*camVector;

			if( !m_resettingCameraPos && m_camEntDiffAngle > 0.5f )
			{
				cameraTarget = m_wantedPos + (m_currentEntityTrackPos-m_lastEntityTrackPos);
				cameraTarget.y = m_trackingCameraDist;
			}
			else
			{
				// list of the last 10 movement vectors to average over (when not moving towards the camera)
				vectors.PutDataAtEnd(new Vector3(m_currentEntityTrackPos-m_lastEntityTrackPos));
				while( vectors.Size() < 10 ) vectors.PutDataAtEnd(new Vector3(m_currentEntityTrackPos-m_lastEntityTrackPos));
				while( vectors.Size() > 10 ) vectors.RemoveData(0);
				Vector3 moveVector;
				for( int i = 0; i < vectors.Size(); i++ ) moveVector += *(vectors.GetData(i));
				moveVector /= vectors.Size();
				moveVector.SetLength(m_trackingCameraDist);
				cameraTarget = m_currentEntityTrackPos - moveVector;
				cameraTarget.y = m_trackingCameraDist;
			}

			m_lastEntityTrackPos = m_currentEntityTrackPos;
		}
	}

	m_wantedPos = cameraTarget;
	
	return true;
}

bool Camera::AdvanceCanSeeUnits( Vector3 &targetCamera )
{
	float distToEntity = (m_predictedEntityPos - m_pos).Mag();

	if (DirectDistanceToBlockage(m_pos, m_predictedEntityPos, distToEntity) < distToEntity - 1.0f) {

	    Vector3 blockage;
		GetHighestTangentPoint( m_predictedEntityPos, m_pos, distToEntity, blockage );

		Vector3 ray = blockage - m_predictedEntityPos;
		ray.Normalise();
		targetCamera = m_predictedEntityPos + ray * distToEntity;
		return true;
	}

	return false;
}

bool Camera::AdvanceNotTooLow( Vector3 &targetCamera )
{
	// Code to check if the camera is not too low of the ground	
	float cameraHeight = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );

	if (m_pos.y < MIN_HEIGHT || cameraHeight < MIN_TRACKING_HEIGHT * m_heightMultiplier) {
		targetCamera = m_pos;
		targetCamera.y += 20;
		return true;
	}

	return false;
}

bool Camera::AdvanceNotTooFarAway( Vector3 &targetCamera )
{
 	Vector3 entityToCamera = m_pos - m_predictedEntityPos;

	float maxDist = m_distFromEntity * 1.2f;

	if (entityToCamera.MagSquared() > maxDist * maxDist) {
		entityToCamera.Normalise();
		targetCamera = m_predictedEntityPos + entityToCamera * maxDist;
		return true;
	}	

	return false;
}

void Camera::RotateTowardsEntity( Entity *entity )
{
	float screenPosX, screenPosY;
	Get2DScreenPos(entity->m_pos, &screenPosX, &screenPosY);
	float w = g_app->m_renderer->ScreenW(), h = g_app->m_renderer->ScreenH();
	w /= 2;
	h /= 2;

	// Distance from centre, normalised
	w = iv_abs(w-screenPosX) / w; 
	h = iv_abs(h-screenPosY) / h;

    float factor1 = (g_advanceTime * 2.0f) + sqrt(w*w + h*h)/10;
	if( factor1 > 1.0f )
	{
		factor1 = 1.0f;
	}
    float factor2 = 1.0f - factor1;

	Vector3 newTargetPos = entity->m_pos;/*(m_lastEntityTrackPos-m_currentEntityTrackPos).MagSquared() <= 10 ? 
		m_newTargetPosTemp : 
		m_currentEntityTrackPos + ((m_currentEntityTrackPos-m_lastEntityTrackPos).Normalise() * m_trackingCameraDist) * (m_camEntDiffAngle > 0.5f ? -1 : 1);//entity->GetCameraFocusPoint();
		*/
	newTargetPos.y = m_trackingEntity->m_pos.y;
    m_targetPos = factor1 * newTargetPos + factor2 * m_targetPos;

    Vector3 targetFront = ( m_targetPos - m_pos ).Normalise();
    m_front = m_front * factor2 + targetFront * factor1;
    m_up.Set( 0.0f, -1.0f, 0.0f );
    Vector3 right = m_up ^ m_front;
    m_up = right ^ m_front;

	m_newTargetPosTemp = newTargetPos;
}

void Camera::AdvanceEntityTrackMode()
{
    if( g_app->m_taskManagerInterface->IsVisible() )
		return;

	UpdateEntityTrackingMode();

	if (!g_app->m_location || !m_entityTrack) 
	{
		RequestMode( Camera::ModeFreeMovement );
		return;
	}
	GetEntityToTrack(m_objectId);
	Entity *entity = g_app->m_location->GetEntity( m_objectId );
	if (!entity || entity->m_dead)
	{
		RequestMode( Camera::ModeFreeMovement );
		return;
	}

    Task *currentTask = g_app->m_location->GetMyTeam()->m_taskManager->GetCurrentTask();
    if (currentTask && currentTask->m_state != Task::StateRunning )
	{
		RequestMode( Camera::ModeFreeMovement );
		return;
	}


    if( !currentTask && entity->m_type != Entity::TypeOfficer )
	{
		RequestMode( Camera::ModeFreeMovement );
		return;
	}


    if( !m_objectId.IsValid() )
	{
		RequestMode( Camera::ModeFreeMovement );
		return;
	}


	// Calculate the predicated position of the entity (which is where
	// it is actually rendered. This is used by the auxillary functions)
	m_predictedEntityPos = entity->m_pos + g_predictionTime * entity->m_vel;
	m_trackingEntity = entity;
	m_trackingEntityId = m_trackingEntity->m_id;
	
	AdvanceAutomaticTracking();

	// Ensure that the target cursor remains in the centre of the screen
    int halfHeight = g_app->m_renderer->ScreenH() / 2;
	int halfWidth = g_app->m_renderer->ScreenW() / 2;
	g_target->SetMousePos(halfWidth, halfHeight);

	Entity* e = g_app->m_location->GetEntity(m_trackingEntityId);
	if( !e || e->m_dead )
	{
		m_trackingEntity = NULL;
		m_trackVector = Vector3();
		m_targettingEntity = NULL;
		m_trackingEntityId = WorldObjectId();
	}

	if( (m_wantedTargetVector-m_targetVector).MagSquared() > 1.0f )
	{
		Vector3 direction = m_wantedTargetVector - m_targetVector;
		//if( direction.MagSquared() > 1.0f ) direction.SetLength(100 * g_advanceTime);
		direction /= 4;

		m_targetVector += direction;
	}

	return;
}


void Camera::GetHighestTangentPoint( Vector3 const &_from, Vector3 const &_to, float _maxDist, Vector3 &location )
{
	unsigned int const numSteps = 40;
	float const distStep = _maxDist / (float)numSteps;

	Vector3 fromAtSameHeight = _from;
	fromAtSameHeight.y = _to.y;
    Vector3 dir = ( _to - fromAtSameHeight ).Normalise();

	float x = _from.x;
	float z = _from.z;

	float const deltaX = distStep * dir.x;
	float const deltaZ = distStep * dir.z;

    float maxGradient = 0.0f;
	float distanceTravelled = 0.0f;

	for (int i = 1; i <= numSteps; ++i)
	{
		x += deltaX;
		z += deltaZ;
		distanceTravelled += distStep;

		float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(x, z);

		float gradient = (landHeight - _from.y) / distanceTravelled;

		if (gradient > maxGradient) {
			maxGradient = gradient;
			location.Set( x, landHeight, z );
		}
	}
}


void Camera::GetHighestPoint( Vector3 const &_from, Vector3 const &_to, float _maxDist, Vector3 &location )
{
	unsigned int const numSteps = 40;
	float const distStep = _maxDist / (float)numSteps;

    Vector3 dir = ( _to - _from ).Normalise();

    float maxHeight = 0.0f;

	for (int i = 1; i <= numSteps; ++i)
	{
		float x = _from.x + dir.x * distStep * (float)i;
		float z = _from.z + dir.z * distStep * (float)i;
		float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(x, z);
		if (landHeight > maxHeight)
		{
            maxHeight = landHeight;
            location.Set( x, landHeight, z );
		}
	}
}


// Returns the number of meters to the nearest blockage that the camera would
// experience if it travelled in the specified direction. A blockage is defined
// as a piece of land more than 10 metres higher than the camera's current 
// height. If there is no blockage FLT_MAX is returned.
float Camera::DirectDistanceToBlockage(Vector3 const &_from, Vector3 const &_to, float const _maxDist)
{
    if( !g_app->m_location ) return FLT_MAX;

	unsigned int const numSteps = 40;
	float const distStep = _maxDist / (float)numSteps;
	
	float x = _from.x;
	float y = _from.y;
	float z = _from.z;

	Vector3 dir = ( _to - _from ).Normalise();

	float const deltaX = distStep * dir.x;
	float const deltaY = distStep * dir.y;
	float const deltaZ = distStep * dir.z;

	for (int i = 1; i <= numSteps; ++i)
	{
		x += deltaX;
		y += deltaY;
		z += deltaZ;

		float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(x, z);

		if (landHeight > y)	
		{
			return (float)i * distStep;
		}
	}

	return _maxDist;
}



// Expects m_targetPos to have been set
void Camera::AdvanceRadarAimMode()
{
	int const screenH = g_app->m_renderer->ScreenH();
	int const screenW = g_app->m_renderer->ScreenW();

	Vector3 groundPos = m_targetPos;
	groundPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(groundPos.x, groundPos.z);

	Vector3 focusPos = groundPos;
	focusPos.y += m_height;

	// Set up viewing matrices
	glPushMatrix();
	SetupProjectionMatrix(g_app->m_renderer->GetNearPlane(), g_app->m_renderer->GetFarPlane());
	SetupModelviewMatrix();

    // Get the 2D mouse coordinates before we move the camera
	Vector3 mousePos3D = g_app->m_userInput->GetMousePos3d();
	float oldMouseX, oldMouseY;
	Get2DScreenPos(mousePos3D, &oldMouseX, &oldMouseY);
	oldMouseY = screenH - oldMouseY;
    
	float factor1 = 4.0f * g_advanceTime;
	float factor2 = 1.0f - factor1;

	// Update camera orientation 
    if (mousePos3D.MagSquared() > 1.0f)
    {
        Vector3 desiredFront = mousePos3D - m_pos;
        desiredFront.Normalise();

        m_front = m_front * factor2 + desiredFront * factor1;
        m_front.Normalise();
        Vector3 right = m_front ^ g_upVector;
		right.Normalise();
        m_up = right ^ m_front;
    }


	//
    // Move towards the idealPos
    
	Vector3 idealPos = groundPos;
	idealPos.y += m_height;
	Vector3 horiCamFront = m_front;
	horiCamFront.y = 0.0f;
	horiCamFront.Normalise();
	idealPos -= horiCamFront * (0.0f + m_height * 1.5f);
    Vector3 toIdealPos = idealPos - m_pos;
	float distToIdealPos = toIdealPos.Mag();
	m_pos = idealPos * factor1 + m_pos * factor2;

    // Set up viewing matrices
	SetupModelviewMatrix();

	// Get the 2D mouse coordinates now that we have moved the camera
    float newMouseX, newMouseY;
    Get2DScreenPos(mousePos3D, &newMouseX, &newMouseY);
    newMouseY = screenH - newMouseY;

	// Calculate how much to move the cursor to make it look like it is
	// locked to a point on the landscape (we need to take account of
	// sub-pixel error from previous frames)
    static float mouseDeltaX = 0.0f;
    static float mouseDeltaY = 0.0f;
    mouseDeltaX += (newMouseX - oldMouseX) * 1.0f;
    mouseDeltaY += (newMouseY - oldMouseY) * 1.0f;
    int intMouseDeltaX = floorf(mouseDeltaX);
    int intMouseDeltaY = floorf(mouseDeltaY);
    mouseDeltaX -= intMouseDeltaX;
    mouseDeltaY -= intMouseDeltaY;
	newMouseX = g_target->X() + intMouseDeltaX;
	newMouseY = g_target->Y() + intMouseDeltaY;

	// Make sure these movements don't put the mouse cursor somewhere stupid
	if (newMouseX < 0)				newMouseX = 0;
	if (newMouseX >= screenW)		newMouseX = screenW - 1;
	if (newMouseY < 0)				newMouseY = 0;
	if (newMouseY >= screenH)		newMouseY = screenH - 1;

	// Apply the mouse cursor movement
//alleg    position_mouse(newMouseX, newMouseY);
	g_target->SetMousePos(newMouseX, newMouseY);
    glPopMatrix();
}


void Camera::AdvanceTurretAimMode()
{
    Team *team = g_app->m_location->m_teams[g_app->m_globalWorld->m_myTeamId];
    if( team )
    {
        Building *b = g_app->m_location->GetBuilding(team->m_currentBuildingId);
        if( !b || b->m_type != Building::TypeGunTurret || b->m_id.GetTeamId() != team->m_teamId )
        {
            RequestMode( Camera::ModeFreeMovement );
            return;
        }
    }
    SetTargetFOV( 50 );

	int const screenH = g_app->m_renderer->ScreenH();
	int const screenW = g_app->m_renderer->ScreenW();

	Vector3 groundPos = m_targetPos;
    groundPos.y += 20.0f;
    float minY = g_app->m_location->m_landscape.m_heightMap->GetValue(groundPos.x, groundPos.z);
    
    groundPos -= m_front * m_height;
    //groundPos.y = max( groundPos.y, minY );
    
	//groundPos.y = ;
    //groundPos.y -= 10.0f;
    
	Vector3 focusPos = groundPos;
	focusPos.y += m_height;

	// Set up viewing matrices
	glPushMatrix();
	SetupProjectionMatrix(g_app->m_renderer->GetNearPlane(), g_app->m_renderer->GetFarPlane());
	SetupModelviewMatrix();

    // Get the 2D mouse coordinates before we move the camera
	Vector3 mousePos3D = g_app->m_userInput->GetMousePos3d();
	float oldMouseX, oldMouseY;
	Get2DScreenPos(mousePos3D, &oldMouseX, &oldMouseY);
	oldMouseY = screenH - oldMouseY;

	float factor1 = 4.0f * g_advanceTime;
	float factor2 = 1.0f - factor1;

	// Update camera orientation 
    if (mousePos3D.MagSquared() > 1.0f)
    {
        Vector3 desiredFront = mousePos3D - m_pos;
        desiredFront.Normalise();

        m_front = m_front * factor2 + desiredFront * factor1;
        m_front.Normalise();
        Vector3 right = m_front ^ g_upVector;
		right.Normalise();
        m_up = right ^ m_front;
    }


	//
    // Move towards the idealPos
    
	Vector3 idealPos = groundPos;
	idealPos.y += m_height;
	Vector3 horiCamFront = m_front;
	horiCamFront.y = 0.0f;
	horiCamFront.Normalise();
	idealPos -= horiCamFront * (0.0f + m_height * 0.8f);
    Vector3 toIdealPos = idealPos - m_pos;
	float distToIdealPos = toIdealPos.Mag();
	m_pos = idealPos * factor1 + m_pos * factor2;

    // Set up viewing matrices
	SetupModelviewMatrix();

	// Get the 2D mouse coordinates now that we have moved the camera
    float newMouseX, newMouseY;
    Get2DScreenPos(mousePos3D, &newMouseX, &newMouseY);
    newMouseY = screenH - newMouseY;

	// Calculate how much to move the cursor to make it look like it is
	// locked to a point on the landscape (we need to take account of
	// sub-pixel error from previous frames)
    static float mouseDeltaX = 0.0f;
    static float mouseDeltaY = 0.0f;
    mouseDeltaX += (newMouseX - oldMouseX) * 1.0f;
    mouseDeltaY += (newMouseY - oldMouseY) * 1.0f;
    int intMouseDeltaX = floorf(mouseDeltaX);
    int intMouseDeltaY = floorf(mouseDeltaY);
    mouseDeltaX -= intMouseDeltaX;
    mouseDeltaY -= intMouseDeltaY;
	newMouseX = g_target->X() + intMouseDeltaX;
	newMouseY = g_target->Y() + intMouseDeltaY;

	// Make sure these movements don't put the mouse cursor somewhere stupid
	if (newMouseX < 0)				newMouseX = 0;
	if (newMouseX >= screenW)		newMouseX = screenW - 1;
	if (newMouseY < 0)				newMouseY = 0;
	if (newMouseY >= screenH)		newMouseY = screenH - 1;

	// Apply the mouse cursor movement
//alleg    position_mouse(newMouseX, newMouseY);
	g_target->SetMousePos(newMouseX, newMouseY);
    glPopMatrix();
}


void Camera::AdvanceFirstPersonMode()
{
    if( g_inputManager->controlEvent( ControlCameraFreeMovement ) )
    {
        RequestMode(Camera::ModeFreeMovement);
        return;
    }

	// JAMES_TODO: Support directional firing mode
	if( g_inputManager->controlEvent( ControlUnitPrimaryFireTarget ) )
    {
        static float lastFire = 0.0f;
        if( GetHighResTime() > lastFire )
        {
            Vector3 from = m_pos +
                           GetRight() * -2.0f +
                           GetUp() * -3.0f;
            g_app->m_location->FireLaser( from, m_front * 200.0f, 3 );
            lastFire = GetHighResTime() + 0.1f;
        }
    }

    //
    // Allow quake keys to move us
    
    Vector3 accelForward = m_front;
    accelForward.y = 0.0f;
    accelForward.Normalise();
    Vector3 accelRight = GetRight();
    accelRight.y = 0.0f;
    accelRight.Normalise();
    float moveRate = 100.0f;
//    if (g_controlBindings->CameraLeft())		m_vel += accelRight * g_advanceTime * moveRate;
//	if (g_controlBindings->CameraRight())		m_vel -= accelRight * g_advanceTime * moveRate;
//	if (g_controlBindings->CameraForwards())	m_vel += accelForward * g_advanceTime * moveRate * 2.0f;
//	if (g_controlBindings->CameraBackwards())	m_vel -= accelForward * g_advanceTime * moveRate;


    //
    // Update our position and orientation

    m_vel *= 0.9993f;
    m_pos += g_advanceTime * m_vel;       

    int mx=0, my=0;
	mx = g_target->dX();
	my = g_target->dY();

    Matrix33 mat(1);
	mat.RotateAroundY((float)mx * -0.01f);
	m_up = m_up * mat;
	m_front = m_front * mat;

    Vector3 right = GetRight();
	mat.SetToIdentity();
	mat.RotateAround(right, (float)my * 0.01f);
	m_up = m_up * mat;
	m_front = m_front * mat;

    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    m_pos.y = landHeight + 3.0f;
}


void Camera::AdvanceMoveToTargetMode()
{
	double currentTimeFraction = (g_gameTime - m_startTime) / m_moveDuration;
    currentTimeFraction = max( currentTimeFraction, 0.2 );


	// Pos
	Vector3 direction = m_targetPos - m_startPos;
	float dist = direction.Mag();
	direction /= dist;
	float maxSpeed = (2.0f * dist) / m_moveDuration;
	
	// Orientation
	Vector3 fullRotationDirection = m_startFront ^ m_targetFront;
	Vector3 middleFront = m_startFront;
	middleFront.RotateAround(fullRotationDirection * 0.5f);

	if (currentTimeFraction < 0.5)
	{
		m_pos += direction * maxSpeed * 2.0 * currentTimeFraction * g_advanceTime;
		float amount2 = RampUpAndDown(m_startTime, m_moveDuration, g_gameTime) * 2.0f;
		float amount1 = 1.0f - amount2;
		m_front = (m_startFront * amount1 + middleFront * amount2).Normalise();
	}
	else if (currentTimeFraction < 1.0)
	{
		m_pos += direction * maxSpeed * 2.0 * (1.0 - currentTimeFraction) * g_advanceTime;
		float amount2 = (RampUpAndDown(m_startTime, m_moveDuration, g_gameTime) - 0.5f) * 2.0f;
		float amount1 = 1.0f - amount2;
		m_front = (middleFront * amount1 + m_targetFront * amount2).Normalise();
	}
	else
	{
		RequestMode(Camera::ModeDoNothing);
		m_front = m_targetFront;
	}
    
	m_front.Normalise();
	m_up = g_upVector;
    //Vector3 right = m_front ^ m_up;
	//right.Normalise();
	//m_up = right ^ m_front;
	//m_up.Normalise();

	//float dot = m_front * m_up;
	//dot *= 1.0f;
}


void Camera::AdvanceEntityFollowMode()
{
    Entity *obj = (Entity*)g_app->m_location->GetEntity( m_objectId );    
    if( !obj ) 
    {
        RequestMode(Camera::ModeFreeMovement);
        return;
    }


	//
	// Get X and Y mouse move
	
	int halfHeight = g_app->m_renderer->ScreenH() / 2;
	int halfWidth = g_app->m_renderer->ScreenW() / 2;
	int deltaX = g_target->X() - halfWidth;
	int deltaY = g_target->Y() - halfHeight;
	g_target->SetMousePos(halfWidth, halfHeight);
	

	//
	// Get Z mouse move

	m_distFromEntity *= 1.0f + ((float)g_target->dZ() * -0.1f);
	float radius = 15.0f;
	if (m_distFromEntity < radius)
	{
		m_distFromEntity = radius;
	}
	else if (m_distFromEntity > 5000.0f)
	{
		m_distFromEntity = 5000.0f;
	}

	
	// 
	// Do rotation

	float rotY = (float)deltaX * -0.015f;
	float rotRight = (float)deltaY * -0.01f;

    m_front.RotateAroundY(rotY);
    Vector3 right = (m_front ^ g_upVector).Normalise();
	Vector3 newUp(m_up);
	newUp.RotateAround(right * rotRight);
	if (newUp.y > 0.1f)
	{
		m_front.RotateAround(right * rotRight);
	}
    m_up = (right ^ m_front).Normalise();


	//
	// Update position

	float factor1 = g_advanceTime * 5.0f;
	float factor2 = 1.0f - factor1;
    Vector3 newTargetPos = obj->m_pos + g_predictionTime * obj->m_vel;
	m_targetPos = factor1 * newTargetPos + factor2 * m_targetPos;
	m_pos = m_targetPos - m_front * m_distFromEntity;
}



// **************
// Public Methods
// **************

// *** Constructor
Camera::Camera()
:   m_fov(60.0f),
    m_targetFov(60.0f),
    m_height(50.0f),
    m_vel(0,0,0),
    m_mode(ModeDoNothing),
	m_debugMode(DebugModeAuto),
    m_objectId(),
	m_framesInThisMode(0),
	m_distFromEntity(100.0f),
	m_anim(NULL),
    m_cameraShake(0.0f),
    m_entityTrack(false),
	m_trackingEntity(NULL),
    m_currentDistance(0.0f),
	m_heightMultiplier(1.0f),
	m_skipDirectionCalculation(false),
	m_trackingCameraDist(250),
	m_targettingEntity(NULL),
	m_targetPosOffset(0.0f),
	m_targettingFOVAngle(0.939692621f),	// 40degree FOV
	m_resettingCameraPos(false),
	m_camLookAround(false)
{
	m_cosFov = iv_cos(m_fov / 180.0f * M_PI);
    m_pos = Vector3(1000.0f, //g_app->m_location->m_landscape.GetWorldSizeX() / 2.0f, 
					500.0f,
					1000.0f);//g_app->m_location->m_landscape.GetWorldSizeZ() / 2.0f);

	m_minX = -1e6;
	m_maxX = 1e6;
	m_minZ = -1e6;
	m_maxZ = 1e6;
    
    //m_front = Vector3(0, -0.7f, 1);
    m_front.Set( 0,-0.5f,-1 );
    m_front.Normalise();
    
    m_up = g_upVector;
    Vector3 right = m_up ^ m_front;
    right.Normalise();

    m_up = m_front ^ right;
    m_up.Normalise();

	m_controlVector = right;

	m_currentEntityTrackPos = g_zeroVector;
	m_lastEntityTrackPos = g_zeroVector;
}


void Camera::CreateCameraShake(float _intensity)
{
    m_cameraShake = max( m_cameraShake, _intensity );
}


void Camera::SetupProjectionMatrix(float _nearPlane, float _farPlane)
{
	//AppDebugAssert(m_fov > 0.0f);
	//AppDebugAssert(m_fov < 180.0f);

    clamp( m_fov, 1, 180 );

	g_app->m_renderer->SetNearAndFar(_nearPlane, _farPlane);
	g_app->m_renderer->SetupProjMatrixFor3D();

	float fovRadians = m_fov * M_PI / 180.0f;
	m_cosFov = iv_cos(fovRadians);

	// m_fov is actually the vertical fov. We need a fov covering
	// the long diagonal of the screen for visibility checking.

	float screenW = g_app->m_renderer->ScreenW();
	float screenWHalf = screenW / 2.0;
	float screenH = g_app->m_renderer->ScreenH();
	float screenHHalf = screenH / 2.0;
	
	// Distance from camera to top-centre and bottom-centre of screen 
	float dtc = screenHHalf / iv_sin(fovRadians/2.0);
	
	// Distance from camera to any corner of the screen
	float dc = iv_sqrt( dtc*dtc + screenWHalf*screenWHalf );
	
	// Half distance from one corner of the screen to the diagonally opposite corner
	float ddHalf = iv_sqrt(screenW * screenW + screenH * screenH) / 2.0;	
	
	m_maxFovRadians = 2.0 * iv_asin(ddHalf/dc);
}


void Camera::SetupModelviewMatrix()
{
	float dot = m_front * m_up;
	AppDebugAssert(NearlyEquals(m_front.MagSquared(), 1.0f));
	//AppDebugAssert(NearlyEquals(m_up.MagSquared(), 1.0f));
	AppDebugAssert(NearlyEquals(dot, 0.0f));

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	float magOfPos = m_pos.Mag();
	Vector3 front = m_front * magOfPos;
	Vector3 up = m_up * magOfPos;
	Vector3 forwards = m_pos + front;
	gluLookAt(m_pos.x, m_pos.y, m_pos.z,
              forwards.x, forwards.y, forwards.z,
              up.x, up.y, up.z);
}

 
bool Camera::PosInViewFrustum(Vector3 const &_pos)
{
	Vector3 dirToPos = (_pos - m_pos).Normalise();
	float angle = dirToPos * m_front;
	float fovInRadians = m_cosFov;
    float tolerance = 0.2f;
	if (angle < fovInRadians - tolerance)
	{
		return false;
	}
	return true;
}


bool Camera::SphereInViewFrustum(Vector3 const &_centre, float _radius )
{
    Vector3 dirToPos = _centre - m_pos;
    float distance = dirToPos.Mag();
    if( distance < _radius ) return true;

    float angularSize = iv_asin(_radius / distance);
    float angle = iv_acos( dirToPos.Normalise() * m_front );

	return angle - angularSize <= m_maxFovRadians / 2;
}


Building *Camera::GetBestBuildingInView()
{
    static float s_recalculateTimer = 0;
    static int s_buildingId = -1;

    if( GetVel().Mag() > 5.0f )
    {
        // We are moving too fast to be focussing on a building
        s_buildingId = -1;
    }
    else if( !g_app->m_location )
    {
        // We aren't in a location
        s_buildingId = -1;
    }
    else
    {
        if( GetHighResTime() > s_recalculateTimer + 1.0f )
        {
	        Vector3 rayStart;
	        Vector3 rayDir;
	        GetClickRay(g_app->m_renderer->ScreenW()/2, 
			            g_app->m_renderer->ScreenH()/2, &rayStart, &rayDir);

            float nearest = 200.0f;
            s_buildingId = -1;         

            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *building = g_app->m_location->m_buildings[i];
                    if( building->DoesRayHit( rayStart, rayDir ) )
                    {
                        float distance = ( building->m_pos - m_pos ).Mag();
                        if( distance < nearest )
                        {
                            nearest = distance;
                            s_buildingId = building->m_id.GetUniqueId();
                        }
                    }
                }
            }

            s_recalculateTimer = GetHighResTime();
        }
    }
    
    return g_app->m_location->GetBuilding( s_buildingId );
}


float Camera::GetFov()
{    
    return m_fov;
}


void Camera::AdvanceComponentZoom()
{
	// No zoom inside the task manager 
    if( g_app->m_taskManagerInterface->IsVisible()  ||
        IsInMode( ModeEntityTrack ) )
    {
        return;
    }

    float change = 30.0f;    
	float adjustedTargetFov = m_targetFov;

	// JAMES CHECK:
	if( g_inputManager->controlEvent( ControlCameraZoom ) && EclGetWindows()->Size() == 0 )
    {
        adjustedTargetFov /= 4.0f;
        change = 100.0f;
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
        g_app->m_helpSystem->PlayerDoneAction( HelpSystem::CameraZoom );
#endif
    }

    if( m_mode == ModeSphereWorldScripted )
    {
        change = 10.0f;
    }

    if( m_mode == ModeMoveToTarget || 
        m_mode == ModeDoNothing ||
        m_mode == ModeBuildingFocus )
    {
        change = 1.0f;
    }
    
//    if( m_fov < adjustedTargetFov )
//    {
//        m_fov += change * g_advanceTime;
//        if( m_fov > adjustedTargetFov )
//        {
//            m_fov = adjustedTargetFov;
//        }
//    }
//    else if( m_fov > adjustedTargetFov )
//    {
//        m_fov -= change * g_advanceTime; 
//        if( m_fov < adjustedTargetFov )
//        {
//            m_fov = adjustedTargetFov;
//        }
//    }

    float factor = g_advanceTime * change * 0.1f;
    m_fov = m_fov * (1-factor) + adjustedTargetFov * factor;

//    if( m_cameraShake > 0.0f )
//    {
//        m_fov += sfrand( m_cameraShake * 10.0f );
//        m_cameraShake -= SERVER_ADVANCE_PERIOD;
//    }
}



void Camera::AdvanceComponentMouseWheelHeight()
{
	float delta = g_target->dZ();

    if( EclGetWindows()->Size() == 0 )
    {
        bool keyUp   = g_inputManager->controlEvent( ControlCameraUp ) && !AimingBuilding();
        bool keyDown = g_inputManager->controlEvent( ControlCameraDown ) && !AimingBuilding();
        if (keyUp)          delta += g_advanceTime * 7.0f;
        if (keyDown)        delta -= g_advanceTime * 7.0f;
        if( keyUp || keyDown )
        {
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
            g_app->m_helpSystem->PlayerDoneAction( HelpSystem::CameraHeight );
#endif
		    g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondCameraUp);
		    g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondCameraDown);
        }
    }

	if( g_app->m_location)
    {
		float landheight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
		if (landheight < MIN_HEIGHT) landheight = MIN_HEIGHT;
		float altitude = m_height - landheight;
		m_height += delta * 2.0f * iv_sqrt(fabsf(altitude));
        
		if (m_mode == ModeTurretAim)
        {
            m_height = max( m_height, MIN_GROUND_CLEARANCE );
            m_height = min( m_height, 50.0f );
        }
        else if( m_mode == ModeRadarAim )
        {
            m_height = max( m_height, 50.0f );
            m_height = min( m_height, 120.0f );
        }
        else
        {
            if(landheight + MIN_GROUND_CLEARANCE > m_height)		
            {
			    m_height = landheight + MIN_GROUND_CLEARANCE;
            }
		}

		if (m_height > MAX_HEIGHT) m_height = MAX_HEIGHT;

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
        g_app->m_helpSystem->PlayerDoneAction( HelpSystem::CameraHeight );
#endif
    }
    else
    {
        m_height += delta * (m_height * 0.1f + 5.0f);
		if (m_height < 0.0f)			m_height = 0.1f;
		else if (m_height > 1000.0f)	m_height = 1000.0f;
    }



}


void Camera::AdvanceAnim()
{
	CamAnimNode *node = m_anim->m_nodes[m_animCurrentNode];
	float finishTime = m_animNodeStartTime + node->m_duration;
	
	if (g_gameTime > finishTime)
	{
		switch (node->m_transitionMode)
		{
		case CamAnimNode::TransitionMove:
			break;
		case CamAnimNode::TransitionCut:
			CutToTarget();
			break;
		}

		m_animCurrentNode++;
		if (m_animCurrentNode < m_anim->m_nodes.Size())
		{
			node = m_anim->m_nodes[m_animCurrentNode];
			SetTarget(node->m_mountName);
			m_animNodeStartTime = g_gameTime;

			switch (node->m_transitionMode)
			{
			case CamAnimNode::TransitionMove:
				RequestMode(ModeMoveToTarget);
				SetMoveDuration(node->m_duration);
				break;
			case CamAnimNode::TransitionCut:
				RequestMode(ModeDoNothing);
				break;
			}
		}
		else
		{
			RequestMode(m_modeBeforeAnim);
			m_anim = NULL;
		}
	}
}

bool Camera::DoesLocationDataExist()
{
	if( !g_app )
		return false;

	if( !g_app->m_location )
		return false;

	if( !g_app->m_location->m_landscape.m_heightMap )
		return false;

	return true;
}

void Camera::Advance()
{
    START_PROFILE( "Advance Camera");
    
	if (m_anim)
	{
		AdvanceAnim();
	}

	// Toggle entity tracking
	/*if (g_inputManager->controlEvent( ControlCameraSwitchMode ))
		m_entityTrack = !m_entityTrack;*/

//	switch (m_mode)
//	{
//		case ModeReplay:
//		case ModeSphereWorld:	
//		case ModeFreeMovement:  
//		case ModeBuildingFocus: 
//		case ModeEntityTrack:   
//		case ModeRadarAim:      
//		case ModeFirstPerson:
//		case ModeEntityFollow:
//        case ModeTurretAim:
//        case ModeMoveToTarget:
//        case ModeSphereWorldScripted:
	if( !m_camLookAround && m_mode != ModeEntityTrack )		AdvanceComponentZoom();
//	}

	switch (m_mode)
	{
		case ModeSphereWorld:	
		case ModeFreeMovement:  
		case ModeBuildingFocus: 
		case ModeEntityTrack:   
		case ModeRadarAim:      
		case ModeFirstPerson:  
        case ModeTurretAim:
			AdvanceComponentMouseWheelHeight();
	}

	// 
	// Pick an advancer

	if( m_anim == NULL &&
		   (m_debugMode == DebugModeAlways ||
		   (m_debugMode == DebugModeAuto && EclGetWindows()->Size() > 0) ||
		   m_framesInThisMode < 2 ) )
	{
		g_windowManager->EnsureMouseUncaptured();
		AdvanceDebugMode();
	}
	else
	{
		if (g_app->m_gameMode == App::GameModeNone)
			g_windowManager->EnsureMouseUncaptured();
		else
			g_windowManager->EnsureMouseCaptured();
			
		switch(m_mode)
		{
			case ModeSphereWorld:		    AdvanceSphereWorldMode();		    break;
			case ModeFreeMovement:          if( DoesLocationDataExist() ) AdvanceFreeMovementMode();          break;
			case ModeBuildingFocus:         if( DoesLocationDataExist() ) AdvanceBuildingFocusMode();         break;
			case ModeEntityTrack:           if( DoesLocationDataExist() ) AdvanceEntityTrackMode();           break;
			case ModeRadarAim:              if( DoesLocationDataExist() ) AdvanceRadarAimMode();              break;
			case ModeFirstPerson:           if( DoesLocationDataExist() ) AdvanceFirstPersonMode();           break;
			case ModeMoveToTarget:		    AdvanceMoveToTargetMode();		    break;
			case ModeEntityFollow:		    if( DoesLocationDataExist() ) AdvanceEntityFollowMode();		    break;
            case ModeTurretAim:             if( DoesLocationDataExist() ) AdvanceTurretAimMode();             break;
            case ModeSphereWorldScripted:   AdvanceSphereWorldScriptedMode();   break;
            case ModeSphereWorldIntro:      AdvanceSphereWorldIntroMode();      break;
            case ModeSphereWorldOutro:      AdvanceSphereWorldOutroMode();      break;
            case ModeSphereWorldFocus:      AdvanceSphereWorldFocusMode();      break;
		}
	}

    
    if( m_cameraShake > 0.0f )
    {
        m_front.RotateAroundY( sfrand(m_cameraShake * 0.05f) );
        Vector3 up = g_upVector;
        up.RotateAround( m_front * sfrand(m_cameraShake) * 0.3f );
        Vector3 right = m_front ^ up;
        right.Normalise();
		m_front.Normalise();
        m_up = right ^ m_front;
        m_cameraShake -= g_advanceTime;
    }

	Normalise();

	ASSERT_VECTOR3_IS_SANE(m_pos);
	float dot = m_front * m_up;
	AppDebugAssert(NearlyEquals(dot, 0.0f));

	g_app->m_userInput->RecalcMousePos3d();

	/* We no longer need targetting
	Entity* e = g_app->m_location->GetEntity(m_targettingEntityId);
	static Vector3 lastTargettingEntPos;
	lastTargettingEntPos = e ? e->m_pos : g_zeroVector;

	// The entity is gone or just died, set the target to null.
	m_targettingEntity = e && !e->m_dead ? m_targettingEntity : NULL;
	// If the target has just died, find a new target in the same area.
	if( e && e->m_dead && m_trackingEntity ) 
	{
		lastTargettingEntPos == g_zeroVector ? 
			TargetEntity(lastTargettingEntPos.x-m_trackingEntity->m_pos.x,lastTargettingEntPos.z-m_trackingEntity->m_pos.z) : 
			TargetEntity(lastTargettingEntPos.x,lastTargettingEntPos.z);
	}
	*/

	m_framesInThisMode++;

    END_PROFILE( "Advance Camera");
}

int Camera::GetDebugMode()
{
	return m_debugMode;
}

void Camera::SetDebugMode(int _mode)
{
	m_debugMode = _mode;
}

void Camera::SetNextDebugMode()
{
	m_debugMode++;
	if (m_debugMode >= Camera::DebugModeNumStates)
	{
		m_debugMode = 0;
	}
}

void Camera::RequestMode(int _mode)
{
	AppDebugAssert(_mode >= 0 && _mode < ModeNumModes);
	int screenW = g_app->m_renderer->ScreenW();
	int screenH = g_app->m_renderer->ScreenH();

	//m_targetFov = 60.0f;
	m_framesInThisMode = 0;
	m_mode = _mode;

	switch (_mode)
	{
		case ModeSphereWorld:
			g_target->SetMousePos(screenW/2, screenH/2);
			m_pos.Set(1000, 500, 15000);
			break;
		case ModeFreeMovement:
			m_targetPos = m_pos;
			m_height = m_pos.y;
            m_targetFov = 60.0f;
			g_target->SetMousePos(screenW/2, screenH/2);
			break;
		case ModeMoveToTarget:
			m_startPos = m_pos;
			m_startFront = m_front;
			m_startUp = m_up;
			m_startTime = g_gameTime;
			break;
	}
}

void Camera::RequestBuildingFocusMode( Building *_building, float _range, float _height )
{
	m_framesInThisMode = 0;
    m_mode = ModeBuildingFocus;
    m_targetPos = _building->m_centrePos;
    m_trackRange = _range;
    m_trackHeight = _height;
    m_trackTimer = GetHighResTime();

    m_trackVector = ( m_pos - _building->m_pos );
    m_trackVector.y = 0.0f;
    m_trackVector.Normalise();

    m_targetFov = 60.0f;
}


void Camera::RequestRadarAimMode( Building *_building )
{
	m_framesInThisMode = 0;
    m_mode = ModeRadarAim;
    if( _building->m_type == Building::TypeRadarDish )
    {
        RadarDish *radar = (RadarDish *)_building;
        m_targetPos = radar->GetDishPos(0.0);
    }
    else
    {
        m_targetPos = _building->m_pos;
    }
    m_height = 80;

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    g_app->m_helpSystem->PlayerDoneAction( HelpSystem::UseRadarDish );
#endif
}


void Camera::RequestTurretAimMode(Building *_building)
{
    m_framesInThisMode = 0;
    m_mode = ModeTurretAim;
    m_objectId = _building->m_id;
    m_targetPos = _building->m_pos;
    m_height = 20;
}


void Camera::RequestEntityTrackMode( WorldObjectId const &_id )
{
	m_trackingEntityId = _id;
	m_lastSquadieId = WorldObjectId();
	m_targettingEntity = NULL;
	m_targettingEntityId = m_lastSquadieId;
	m_trackingCameraDist = 250;
	m_targetVector = g_zeroVector;

	m_framesInThisMode = 0;
    m_mode = ModeEntityTrack;

    m_distFromEntity = 200.0f;
    m_objectId = _id;
    m_trackHeight = 0.0f;
    m_cameraTarget = m_pos;

	// Snap the camera to the look at the unit
    Entity *entity = g_app->m_location->GetEntity( _id );
    if( entity )
    {
        m_targetPos = entity->m_pos;
		m_trackingEntity = entity;
    }
}

void Camera::RequestEntityFollowMode( WorldObjectId const &_id )
{
	m_framesInThisMode = 0;
    m_mode = ModeEntityFollow;
    m_objectId = _id;
}


bool Camera::IsMoving()
{
	return m_mode == ModeMoveToTarget;
}


bool Camera::IsInteractive()
{
    //if( g_app->m_script->IsRunningScript() ) return false;
    
    return ( m_mode == ModeSphereWorld ||
             m_mode == ModeFreeMovement ||
             m_mode == ModeRadarAim ||
             m_mode == ModeTurretAim ||
             m_mode == ModeEntityTrack );
}


bool Camera::IsInMode(int _mode)
{
    return( m_mode == _mode );
}


void Camera::SetTarget(Vector3 const &_pos, Vector3 const &_front, Vector3 const &_up)
{
	m_targetPos = _pos;
	m_targetFront = _front;
	m_targetFront.Normalise();

	Vector3 right = _up ^ m_targetFront;
	right.Normalise();
	m_targetUp = m_targetFront ^ right;
}


bool Camera::SetTarget(char const *_mountName)
{    
	if (stricmp(_mountName, MAGIC_MOUNT_NAME_START_POS) == 0)
	{
		SetTarget(m_posBeforeAnim, m_frontBeforeAnim, m_upBeforeAnim);
		return true;
	}

	for (int i = 0; i < g_app->m_location->m_levelFile->m_cameraMounts.Size(); ++i)
	{
		CameraMount *mount = g_app->m_location->m_levelFile->m_cameraMounts[i];
		if (stricmp(mount->m_name, _mountName) == 0)
		{
			SetTarget(mount->m_pos, mount->m_front, mount->m_up);
			return true;
		}
	}

	return false;
}


void Camera::SetTarget(Vector3 const &_focusPos, float _distance, float _height )
{
    Vector3 themToUs = m_pos - _focusPos;
    themToUs.SetLength( _distance );
    Vector3 targetPos = _focusPos + themToUs;
    targetPos.y = _focusPos.y + _height;
    Vector3 targetFront = ( _focusPos - targetPos ).Normalise();
    targetFront.Normalise();
    Vector3 targetRight = targetFront ^ g_upVector;
    Vector3 targetUp = targetRight ^ targetFront;
    SetTarget( targetPos, targetFront, targetUp ); 
}


void Camera::SetMoveDuration(float _duration)
{
	m_moveDuration = _duration;
}

void Camera::CutToTarget()
{
	m_framesInThisMode = 0;
	m_pos = m_targetPos;
	m_height = m_targetPos.y;
	m_front = m_targetFront;
	m_up = m_targetUp;

	Normalise();
}

void Camera::Normalise()
{
	m_front.Normalise();
	Vector3 right = m_up ^ m_front;
	right.Normalise();
	m_up = m_front ^ right;
}

void Camera::GetClickRay(int _x, int _y, Vector3 *_rayStart, Vector3 *_rayDir)
{
    GLint viewport[4];
    GLdouble mvMatrix[16], projMatrix[16];
    GLdouble objx, objy, objz;

    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, mvMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    int realY = viewport[3] - _y - 1;
 

	//if ( (projMatrix[0] + projMatrix[1] + projMatrix[2] == 0) ||
	//	 (projMatrix[5] + projMatrix[6] + projMatrix[7] == 0) ||
	//	 (projMatrix[9] + projMatrix[10] + projMatrix[11] == 0) )
	//{
	//	glMatrixMode(GL_PROJECTION);
	//	glLoadIdentity();
	//    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	//}

	if ( (mvMatrix[0] + mvMatrix[1] + mvMatrix[2] == 0) ||
		 (mvMatrix[5] + mvMatrix[6] + mvMatrix[7] == 0) ||
		 (mvMatrix[9] + mvMatrix[10] + mvMatrix[11] == 0) )
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	    glGetDoublev(GL_MODELVIEW_MATRIX, mvMatrix);
	}

    gluUnProject (_x, realY, 0.0f,
        mvMatrix,
        projMatrix,
        viewport,
        &objx,
        &objy,
        &objz);
    _rayStart->Set(objx, objy, objz);

    gluUnProject (_x, realY, 1.0f,
        mvMatrix,
        projMatrix,
        viewport,
        &objx,
        &objy,
        &objz);
    _rayDir->Set(objx, objy, objz);
    *_rayDir -= *_rayStart;
    _rayDir->Normalise();
}


void Camera::SetBounds(float _minX, float _maxX, float _minZ, float _maxZ)
{
	m_minX = _minX;
	m_maxX = _maxX;
	m_minZ = _minZ;
	m_maxZ = _maxZ;
}


void Camera::PlayAnimation(CameraAnimation *_anim)
{
	if (_anim->m_nodes.Size() == 0) return;

	m_anim = _anim;
	m_animCurrentNode = 0;
	m_animNodeStartTime = g_gameTime;
	m_modeBeforeAnim = m_mode;
	m_posBeforeAnim = m_pos;
	m_upBeforeAnim = m_up;
	m_frontBeforeAnim = m_front;

	CamAnimNode *node = m_anim->m_nodes[0];
	SetTarget(node->m_mountName);

	switch (node->m_transitionMode)
	{
	case CamAnimNode::TransitionMove:
		RequestMode(ModeMoveToTarget);
		SetMoveDuration(node->m_duration);
		break;
	case CamAnimNode::TransitionCut:
		RequestMode(ModeDoNothing);
		break;
	}
}


void Camera::StopAnimation()
{
	if (m_anim)
	{
		RequestMode(m_modeBeforeAnim);
		m_anim = NULL;
	}
}


bool Camera::IsAnimPlaying()
{
	if( m_anim ) return true;

    if( m_mode == ModeMoveToTarget ) return true;

    return false;
}


void Camera::RecordCameraPosition()
{
	m_posBeforeAnim = m_pos;
	m_upBeforeAnim = m_up;
	m_frontBeforeAnim = m_front;
}


void Camera::RestoreCameraPosition( bool _cut )
{
    SetTarget( m_posBeforeAnim, m_frontBeforeAnim, m_upBeforeAnim );

    if( _cut )
    {
        CutToTarget();
    }
    else
    {
        RequestMode( ModeMoveToTarget );
        SetMoveDuration( 3 );
    }
}

void Camera::SwitchEntityTracking( bool _onOrOff )
{
    m_camLookAround = g_inputManager->controlEvent( ControlCameraFreeMovement ) && !g_inputManager->controlEvent( ControlUnitPrimaryFireTarget );
	m_entityTrack = _onOrOff && !m_camLookAround;
}

Vector3 Camera::GetControlVector()
{
	return m_controlVector;
}

void Camera::UpdateEntityTrackingMode()
{
	if ( g_prefsManager && g_inputManager ) {
		int camTracking = g_prefsManager->GetInt( OTHER_AUTOMATICCAM, 0 );
		if( camTracking != 1 && camTracking != 2 ) {
			// do automatic option detection
			if ( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
				camTracking = 2;
		}
		SwitchEntityTracking( camTracking == 2 );
	}
}

void Camera::WaterReflect()
{
	m_pos.y *= -1;
	m_front.y *= -1;
	m_up.y *= -1;
}

bool Camera::ChatLogVisible()
{
	if( EclGetWindows()->Size() > 0 )
	{
		return false;
	}

    if( g_prefsManager->GetInt("ControlMethod") == 1 &&
        g_inputManager->controlEvent( ControlIconsChatLog ) )
    {
        return true;
    }

    if( g_prefsManager->GetInt("ControlMethod") == 0 &&
        g_inputManager->controlEvent( ControlGesturesChatLog ) )
    {
        return true;
    }

    return false;
}
