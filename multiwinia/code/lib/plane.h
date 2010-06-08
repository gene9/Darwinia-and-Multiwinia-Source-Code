#ifndef INCLUDED_PLANE_H
#define INCLUDED_PLANE_H


#include "lib/vector3.h"


class Plane
{
public:
	Vector3 m_normal;
	double	m_distFromOrigin;

	Plane(Vector3 const &a, Vector3 const &b, Vector3 const &c);
	Plane(Vector3 const &normal, Vector3 const &pointInPlane);
	Plane(Vector3 const &normal, double const distFromOrigin);
	void GetCartesianDefinition(double *x, double *y, double *z, double *d) const;
};


#endif
