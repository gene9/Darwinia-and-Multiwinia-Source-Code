#include "lib/universal_include.h"

#include "lib/plane.h"
#include "lib/vector3.h"


// *************
//  Class Plane
// *************

// *** Constructors ***

// Three points
Plane::Plane(Vector3 const &a, Vector3 const &b, Vector3 const &c)
{
	Vector3 vectInPlane1 = a - b;
	Vector3 vectInPlane2 = a - c;
	m_normal = vectInPlane1 ^ vectInPlane2;
	m_normal.Normalise();
	m_distFromOrigin = -m_normal * a;
}


// Normal and Point
Plane::Plane(Vector3 const &normal, Vector3 const &pointInPlane)
:	m_normal(normal)
{
	m_distFromOrigin = normal * pointInPlane;
}


// Normal and distance
Plane::Plane(Vector3 const &normal, float const distFromOrigin)
:	m_normal(normal),
	m_distFromOrigin(distFromOrigin)
{
}


// Returns the cartesian definition of the plane in the form:
//   ax + by + cz + d = 0
void Plane::GetCartesianDefinition(float *a, float *b, float *c, float *d) const
{
	*a = m_normal.x;
	*b = m_normal.y;
	*c = m_normal.z;
	*d = m_distFromOrigin;
}
