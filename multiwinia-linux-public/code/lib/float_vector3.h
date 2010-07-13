#ifndef FLOAT_VECTOR3_H
#define FLOAT_VECTOR3_H

#include "lib/vector3.h"

class FloatVector3
{
public:
	float x, y, z;

	FloatVector3()
	:	x(0.0), y(0.0), z(0.0) 
	{
	}

	FloatVector3(float _x, float _y, float _z)
	:	x(_x), y(_y), z(_z) 
	{
	}
	
	void Set(float _x, float _y, float _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}
	
	FloatVector3 const &operator = (Vector3 const &b)
	{
		x = b.x;
		y = b.y;
		z = b.z;
		return *this;
	}
	
	operator Vector3()
	{
		return Vector3(x, y, z);
	}
};

#endif