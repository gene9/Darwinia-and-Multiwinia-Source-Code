#ifndef INCLUDED_PLANE_H
#define INCLUDED_PLANE_H


#include "lib/vector3.h"


class Plane
{
public:
	Vector3 m_normal;
	float	m_distFromOrigin;

	Plane(Vector3 const &a, Vector3 const &b, Vector3 const &c);
	Plane(Vector3 const &normal, Vector3 const &pointInPlane);
	Plane(Vector3 const &normal, float const distFromOrigin);
	void GetCartesianDefinition(float *x, float *y, float *z, float *d) const;
};


#endif
