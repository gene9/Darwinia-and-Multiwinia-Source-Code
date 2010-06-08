#include "lib/universal_include.h"
#include "lib/math_utils.h"
#include "lib/matrix34.h"
#include "lib/vector3.h"
#include "lib/debug_render.h"

#include "app.h" // DELETEME
#include "camera.h" // DELETEME

#include "3d_sierpinski_gasket.h"


Sierpinski3D::Sierpinski3D(unsigned int _numPoints)
:	m_numPoints(_numPoints)
{
	m_points = new Vector3[_numPoints];
 
	float size = 20;
	float x1=    0, y1=    0, z1= size;
	float x2= size, y2= size, z2=-size;
	float x3= size, y3=-size, z3=-size;
	float x4=-size, y4= size, z4=-size;
	float x5=-size, y5=-size, z5=-size;
	float x = x1, y = y1, z = z1;
	
	for(unsigned int i = 0; i < m_numPoints; ++i)
	{
		switch( darwiniaRandom()%5 )
		{
			case 0:
				x = ( x+x1 ) / 2;
				y = ( y+y1 ) / 2;
				z = ( z+z1 ) / 2;
				break;
			case 1:
				x = ( x+x2 ) / 2;
				y = ( y+y2 ) / 2;
				z = ( z+z2 ) / 2;
				break;
			case 2:
				x = ( x+x3 ) / 2;
				y = ( y+y3 ) / 2;
				z = ( z+z3 ) / 2;
				break;
			case 3:
				x = ( x+x4 ) / 2;
				y = ( y+y4 ) / 2;
				z = ( z+z4 ) / 2;
				break;
			case 4:
				x = ( x+x5 ) / 2;
				y = ( y+y5 ) / 2;
				z = ( z+z5 ) / 2;
				break;
		}
		m_points[i].x = x;
		m_points[i].y = y;
		m_points[i].z = z;
	}
}


Sierpinski3D::~Sierpinski3D()
{
	delete [] m_points;
	m_numPoints = 0;
}


void Sierpinski3D::Render(float scale)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_LIGHTING);
	glPointSize(3.0f);
	//glDepthMask(false);
	glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
	glScalef(scale, scale, scale);

	float alpha = 128.0f * scale;
	glColor4ub(alpha*0.4f, alpha*0.7f, alpha, 128);

	glBegin(GL_POINTS);
		for (unsigned int i = 0; i < m_numPoints; ++i)
		{
			glVertex3fv(m_points[i].GetData());
		}
	glEnd();

	glEnable(GL_DEPTH_TEST);
	glDepthMask(true);
	glEnable(GL_LIGHTING);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
}
