#include "lib/universal_include.h"

#include <math.h>

#include "lib/vector2.h"
#include "lib/vector3.h"

#include "sphere_renderer.h"


// ******************
// * Class Triangle *
// ******************
Triangle::Triangle( Vector3 const &c1, Vector3 const &c2, Vector3 const &c3 )
{
	m_corner[0] = c1;
	m_corner[1] = c2;
	m_corner[2] = c3;
}


// ****************
// * Class Sphere *
// ****************

Sphere::Sphere()
{
	float const x = 0.5257311121;
	float const z = 0.85065080835;

	Vector3 c[12];
	c[0] = Vector3(-x, 0, z);
	c[1] = Vector3( x, 0, z);
	c[2] = Vector3(-x, 0,-z);
	c[3] = Vector3( x, 0,-z);
	c[4] = Vector3( 0, z, x);
	c[5] = Vector3( 0, z,-x);
	c[6] = Vector3( 0,-z, x);
	c[7] = Vector3( 0,-z,-x);
	c[8] = Vector3( z, x, 0);
	c[9] = Vector3(-z, x, 0);
	c[10] = Vector3( z,-x, 0);
	c[11] = Vector3(-z,-x, 0);

	m_topLevelTriangle[0] = Triangle( c[1], c[4], c[0]);
	m_topLevelTriangle[1] = Triangle( c[4], c[9], c[0]);
	m_topLevelTriangle[2] = Triangle( c[4], c[5], c[9]);
	m_topLevelTriangle[3] = Triangle( c[8], c[5], c[4]);
	m_topLevelTriangle[4] = Triangle( c[1], c[8], c[4]);
	m_topLevelTriangle[5] = Triangle( c[1],c[10], c[8]);
	m_topLevelTriangle[6] = Triangle(c[10], c[3], c[8]);
	m_topLevelTriangle[7] = Triangle( c[8], c[3], c[5]);
	m_topLevelTriangle[8] = Triangle( c[3], c[2], c[5]);
	m_topLevelTriangle[9] = Triangle( c[3], c[7], c[2]);
	m_topLevelTriangle[10] = Triangle( c[3],c[10], c[7]);
	m_topLevelTriangle[11] = Triangle(c[10], c[6], c[7]);
	m_topLevelTriangle[12] = Triangle( c[6],c[11], c[7]);
	m_topLevelTriangle[13] = Triangle( c[6], c[0],c[11]);
	m_topLevelTriangle[14] = Triangle( c[6], c[1], c[0]);
	m_topLevelTriangle[15] = Triangle(c[10], c[1], c[6]);
	m_topLevelTriangle[16] = Triangle(c[11], c[0], c[9]);
	m_topLevelTriangle[17] = Triangle( c[2],c[11], c[9]);
	m_topLevelTriangle[18] = Triangle( c[5], c[2], c[9]);
	m_topLevelTriangle[19] = Triangle(c[11], c[2], c[7]);

    m_displayListId = glGenLists(1);
	glNewList(m_displayListId, GL_COMPILE);
		RenderLong();
	glEndList();
}

void Sphere::ConsiderTriangle(int level, Vector3 const &a, Vector3 const &b, Vector3 const &c)
{
	if (level > 0) 
	{
		glBegin(GL_TRIANGLES);
			glVertex3f(a.x, a.y, a.z);
			glVertex3f(b.x, b.y, b.z);
			glVertex3f(c.x, c.y, c.z);
		glEnd();
	}
	else
	{
		level++;
		
		// Three new vertices (at the midpoints of each existing edge)
		Vector3 p = (a + b).Normalise();
		Vector3 q = (b + c).Normalise();
		Vector3 r = (c + a).Normalise();

		ConsiderTriangle(level, a, p, r);
		ConsiderTriangle(level, p, q, r);
		ConsiderTriangle(level, p, b, q);
		ConsiderTriangle(level, r, q, c);
	}
}

void Sphere::RenderLong() 
{
	// Render each top level triangle
	for(int i = 0; i < 20; i++) {
		ConsiderTriangle(0, m_topLevelTriangle[i].m_corner[0], 
						 m_topLevelTriangle[i].m_corner[1], 
						 m_topLevelTriangle[i].m_corner[2]);
	}
}


void Sphere::Render(Vector3 const &pos, float radius)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glScalef(radius, radius, radius);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1.0f);
	glCallList(m_displayListId);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPopMatrix();
}
