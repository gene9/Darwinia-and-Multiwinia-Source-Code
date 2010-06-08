#include "lib/universal_include.h"

#include <memory>

#include "lib/input/input.h"
#include "lib/input/movement2d.h"

using std::auto_ptr;


int Movement2D::velX() { return vX; }


int Movement2D::velY() { return vY; }


int Movement3D::velZ() { return vZ; }


int Movement2D::signX() {
	if ( vX > 0 ) return  1;
	if ( vX < 0 ) return -1;
	return 0;
}


int Movement2D::signY() {
	if ( vY > 0 ) return  1;
	if ( vY < 0 ) return -1;
	return 0;
}


int Movement3D::signZ() {
	if ( vZ > 0 ) return  1;
	if ( vZ < 0 ) return -1;
	return 0;
}


DPadMovement::DPadMovement( ControlType _north, ControlType _south,
                            ControlType _east, ControlType _west,
							int _sensitivity )
: Movement3D(), north( _north ), south( _south ),  east( _east ), west( _west ),
  up( ControlNull ), down( ControlNull ), sensitivity( _sensitivity ) {}


DPadMovement::DPadMovement( ControlType _north, ControlType _south,
	                        ControlType _east, ControlType _west,
	                        ControlType _up, ControlType _down,
	                        int _sensitivity )
: Movement3D(), north( _north ), south( _south ),  east( _east ), west( _west ),
  up( _up ), down( _down ), sensitivity( _sensitivity ) {}


void DPadMovement::Advance() {
	int n = g_inputManager->controlEvent( north ) ? 1 : 0;
	int s = g_inputManager->controlEvent( south ) ? 1 : 0;
	int e = g_inputManager->controlEvent( east )  ? 1 : 0;
	int w = g_inputManager->controlEvent( west )  ? 1 : 0;
	int u = g_inputManager->controlEvent( up )    ? 1 : 0;
	int d = g_inputManager->controlEvent( down )  ? 1 : 0;

	vY = ( n - s ) * sensitivity;
	vX = ( e - w ) * sensitivity;
	vZ = ( u - d ) * sensitivity;
}


AnalogMovement2D::AnalogMovement2D( ControlType _move, int _sensitivity )
: Movement2D(), move( _move ), sensitivity( _sensitivity ) {}


void AnalogMovement2D::Advance() {
	InputDetails details;
	if ( g_inputManager->controlEvent( move, details ) &&
	     INPUT_TYPE_2D == details.type ) {
		vX = details.x * sensitivity;
		vY = details.y * sensitivity;
	} else
		vX = vY = 0;
}


PriorityMovement2D::PriorityMovement2D( std::auto_ptr<Movement2D> _first,
                                        std::auto_ptr<Movement2D> _second )
: first( _first ), second( _second ) {}


void PriorityMovement2D::Advance() {
	first->Advance(); second->Advance();
	vX = first->velX();
	vY = first->velY();
	if ( vX == 0 && vY == 0 ) {
		vX = second->velX();
		vY = second->velY();
	}
}
