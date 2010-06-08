#include "lib/universal_include.h"

#include "lib/targetcursor.h"
#include "lib/input/input.h"
#include "lib/preferences.h"

#include "lib/window_manager.h"

#include "app.h"
#include "control_help.h"
#include "camera.h"

#include "eclipse.h"
#include "taskmanager_interface.h"

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2


TargetCursor *g_target = NULL;


TargetCursor::TargetCursor() {
	m_screenCoords[AXIS_X] = 0;
	m_screenCoords[AXIS_Y] = 0;
	m_screenCoords[AXIS_Z] = 0;
	m_velocity[AXIS_X] = 0;
	m_velocity[AXIS_Y] = 0;
	m_velocity[AXIS_Z] = 0;
}


void TargetCursor::SetMousePos( int x, int y ) {
	m_screenCoords[AXIS_X] = x;
	m_screenCoords[AXIS_Y] = y;
	g_windowManager->NastySetMousePos(x, y);
}


void TargetCursor::MoveCursor( int x, int y ) {
	m_velocity[AXIS_X] += x;
	m_velocity[AXIS_Y] += y;
	m_screenCoords[AXIS_X] += x;
	m_screenCoords[AXIS_Y] += y;
	g_windowManager->NastyMoveMouse(x, y);
}


int TargetCursor::X() const {
	return m_screenCoords[AXIS_X];
}


int TargetCursor::Y() const {
	return m_screenCoords[AXIS_Y];
}


int TargetCursor::Z() const {
	return m_screenCoords[AXIS_Z];
}


int TargetCursor::dX() const {
	return m_velocity[AXIS_X];
}


int TargetCursor::dY() const {
	return m_velocity[AXIS_Y];
}


int TargetCursor::dZ() const {
	return m_velocity[AXIS_Z];
}


bool secondaryInputEnabled() {
	return ( EclGetWindows()->Size() == 0 ) &&
		!g_app->m_taskManagerInterface->m_visible;
}

void TargetCursor::Advance() {
	InputDetails details;
	if ( ( g_inputManager->controlEvent( ControlTargetMove, details ) ||
	       ( secondaryInputEnabled() &&
	         g_inputManager->controlEvent( ControlTargetMoveSecondary, details ) ) )
	    && INPUT_TYPE_2D == details.type ) {
		m_velocity[AXIS_X] = details.x;
		m_velocity[AXIS_Y] = details.y;
		m_screenCoords[AXIS_X] += m_velocity[AXIS_X];
		m_screenCoords[AXIS_Y] += m_velocity[AXIS_Y];

		if (g_app->m_camera->IsInMode(Camera::ModeFreeMovement))
			g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondCameraAim);
	} else
		m_velocity[AXIS_X] = m_velocity[AXIS_Y] = 0;

	if ( g_inputManager->controlEvent( ControlTargetMoveZ, details ) &&
	     INPUT_TYPE_1D == details.type ) {
		m_velocity[AXIS_Z] = details.x;
		m_screenCoords[AXIS_Z] += m_velocity[AXIS_Z];
	}
	else 
		m_velocity[AXIS_Z] = 0;
}
