#ifndef INCLUDED_MOVEMENT3D_H
#define INCLUDED_MOVEMENT3D_H

#include <memory>

#include "lib/input/control_types.h"


class Movement2D {

protected:
	int vX, vY;

public:
	Movement2D() : vX(0), vY(0) {}

	virtual void Advance() = 0;

	virtual int velX();

	virtual int velY();

	int signX();

	int signY();

};


class Movement3D : public Movement2D {

protected:
	int vZ;

public:
	Movement3D() : Movement2D(), vZ(0) {}

	virtual int velZ();

	int signZ();

};


class DPadMovement : public Movement3D {

private:
	ControlType north, south, east, west, up, down;
	int sensitivity;

public:
	// These all have to be INPUT_TYPE_BOOL inputs
	DPadMovement( ControlType north, ControlType south,
	              ControlType east, ControlType west,
	              int sensitivity );

	DPadMovement( ControlType north, ControlType south,
	              ControlType east, ControlType west,
	              ControlType up, ControlType down,
	              int sensitivity );

	void Advance();

};


class AnalogMovement2D : public Movement2D {

private:
	ControlType move;
	int sensitivity;

public:
	// This has to be an INPUT_TYPE_2D input
	AnalogMovement2D( ControlType _move, int sensitivity );

	void Advance();

};


class PriorityMovement2D : public Movement2D {

private:
	std::auto_ptr<Movement2D> first, second;

public:
	PriorityMovement2D( std::auto_ptr<Movement2D> _first,
	                    std::auto_ptr<Movement2D> _second );

	void Advance();

};


#endif // INCLUDED_MOVEMENT2D_H
