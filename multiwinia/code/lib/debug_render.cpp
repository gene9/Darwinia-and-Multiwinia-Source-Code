#include "lib/universal_include.h"

#include <math.h>
#include <stdarg.h>

#include "lib/math_utils.h"
#include "lib/sphere_renderer.h"
#include "lib/text_renderer.h"
#include "lib/debug_utils.h"

#include "app.h"
#include "camera.h"
#include "debug_render.h"
#include "renderer.h"


#ifdef DEBUG_RENDER_ENABLED


void RenderSquare2d(float x, float y, float size, RGBAColour const &_col)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	GLint v[4];
	glGetIntegerv(GL_VIEWPORT, &v[0]);
    float left = v[0];
	float right = v[0] + v[2];
	float bottom = v[1] + v[3];
	float top = v[1];
	gluOrtho2D(left, right, bottom, top);
    glMatrixMode(GL_MODELVIEW);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	if (_col.a != 255)
	{
		glEnable(GL_BLEND);
	}

	glColor4ubv(_col.GetData());

	glBegin(GL_QUADS);
		glVertex2f(x - size, y - size);
		glVertex2f(x + size, y - size);
		glVertex2f(x + size, y + size);
		glVertex2f(x - size, y + size);
	glEnd();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}


void RenderCube(Vector3 const &_centre, float _sizeX, float _sizeY, float _sizeZ, RGBAColour const &_col)
{
	float halfX = _sizeX * 0.5;
	float halfY = _sizeY * 0.5;
	float halfZ = _sizeZ * 0.5;

	if (_col.a != 255)
	{
		glEnable(GL_BLEND);
	}

	glColor4ubv(_col.GetData());
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(2.0);
	glBegin(GL_LINE_LOOP);
		glVertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z - halfZ);
		glVertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z - halfZ);
		glVertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z - halfZ);
		glVertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z - halfZ);
	glEnd();

	glBegin(GL_LINE_LOOP);
		glVertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z + halfZ);
		glVertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z + halfZ);
		glVertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z + halfZ);
		glVertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z + halfZ);
	glEnd();

	glBegin(GL_LINES);
		glVertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z - halfZ);
		glVertex3f(_centre.x - halfX, _centre.y - halfY, _centre.z + halfZ);
	glEnd();

	glBegin(GL_LINES);
		glVertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z - halfZ);
		glVertex3f(_centre.x - halfX, _centre.y + halfY, _centre.z + halfZ);
	glEnd();

	glBegin(GL_LINES);
		glVertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z - halfZ);
		glVertex3f(_centre.x + halfX, _centre.y + halfY, _centre.z + halfZ);
	glEnd();

	glBegin(GL_LINES);
		glVertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z - halfZ);
		glVertex3f(_centre.x + halfX, _centre.y - halfY, _centre.z + halfZ);
	glEnd();

	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}


void RenderSphereRings(Vector3 const &_centre, float _radius, RGBAColour const &_col)
{
	glColor3ubv(_col.GetData());

	int const _segs = 20;
	float thetaIncrement = M_PI * 2.0 / (float)_segs;

	static float cx[_segs] = {-1};
	static float sx[_segs];

	int i;

	if (cx[0] == -1)
	{
		for (i = 0; i < _segs; i++)
		{
			float theta = (float)i * thetaIncrement;
			sx[i] = iv_sin(theta);
			cx[i] = iv_cos(theta);
		}
	}
	
	glLineWidth(2.0);
	glBegin(GL_LINE_LOOP);
		for (i = 0; i < _segs; i++)
		{
			Vector3 pos = _centre;
			pos.x += sx[i] * _radius;
			pos.z += cx[i] * _radius;
			glVertex3dv(pos.GetData());
		}
	glEnd();
	glBegin(GL_LINE_LOOP);
		for (i = 0; i < _segs; i++)
		{
			Vector3 pos = _centre;
			pos.y += sx[i] * _radius;
			pos.z += cx[i] * _radius;
			glVertex3dv(pos.GetData());
		}
	glEnd();
	glBegin(GL_LINE_LOOP);
		for (i = 0; i < _segs; i++)
		{
			Vector3 pos = _centre;
			pos.x += sx[i] * _radius;
			pos.y += cx[i] * _radius;
			glVertex3dv(pos.GetData());
		}
	glEnd();
}

static Sphere aSphere;

void RenderSphere(Vector3 const &_centre, float _radius, RGBAColour const &_col)
{
    if( Sphere::s_regenerateDisplayList )
    {
        aSphere.GenerateDisplayList();
    }

	glColor4ubv(_col.GetData());
	aSphere.Render(_centre, _radius);
}


void RenderSphereFilled(Vector3 const &_centre, float _radius, RGBAColour const &_col)
{
    if( Sphere::s_regenerateDisplayList )
    {
        aSphere.GenerateDisplayList();
    }

    glColor4ubv(_col.GetData());
    aSphere.RenderFilled(_centre, _radius);
}


void RenderVerticalCylinder(Vector3 const &_centreBase, Vector3 const &_verticalAxis, 
							float _height, float _radius, RGBAColour const &_col)
{
	Vector3 axis1 = _verticalAxis;
	axis1.Normalise();
	Vector3 axis2;
	Vector3 axis3;

	if (axis1.x > 0.5)			axis2.Set(0, 1, 0);
	else						axis2.Set(1, 0, 0);

	axis3 = axis1 ^ axis2;
	axis2 = axis1 ^ axis3;

	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_TEXTURE_2D);
	glLineWidth(1.0);
	glColor3ubv(_col.GetData());

	int const numEdges = 16;

	// Base
	glBegin(GL_LINE_LOOP);
		for (int i = 0; i < numEdges; ++i)
		{
			Vector3 pos = _centreBase;
			float theta = M_PI * 2.0 * (float)i / (float)numEdges;
			pos += axis2 * iv_sin(theta) * _radius;
			pos += axis3 * iv_cos(theta) * _radius;
			glVertex3dv(pos.GetData());
		}
	glEnd();

	// Top
	glBegin(GL_LINE_LOOP);
		for (int i = 0; i < numEdges; ++i)
		{
			Vector3 pos = _centreBase + axis1 * _height;
			float theta = M_PI * 2.0 * (float)i / (float)numEdges;
			pos += axis2 * iv_sin(theta) * _radius;
			pos += axis3 * iv_cos(theta) * _radius;
			glVertex3dv(pos.GetData());
		}
	glEnd();

	// Middle
	glBegin(GL_LINE_LOOP);
		for (int i = 0; i < numEdges; ++i)
		{
			Vector3 pos = _centreBase;
			float theta = M_PI * 2.0 * (float)i / (float)numEdges;
			pos += axis2 * iv_sin(theta) * _radius;
			pos += axis3 * iv_cos(theta) * _radius;
			glVertex3dv(pos.GetData());
			pos += axis1 * _height;
			glVertex3dv(pos.GetData());
		}
	glEnd();

	glEnable(GL_LIGHTING);
}


void RenderArrow(Vector3 const &start, Vector3 const &end, float width, RGBAColour const &_col/* =RGBAColour */)
{
	Camera *cam = g_app->m_camera;
	Vector3 midPoint = (start + end) * 0.5;
	Vector3 midPointToCamera = cam->GetPos() - midPoint;
	float midPointToCameraDist = midPointToCamera.Mag();
	Vector3 invDir = start - end;
	Vector3 sidewaysDir = ((cam->GetPos() - end) ^ invDir).Normalise();
	float arrowLen = invDir.Mag();
	invDir.SetLength(arrowLen * 0.2);

	if (_col.a != 255)
	{
		glEnable(GL_BLEND);
	}

	glLineWidth(width / midPointToCameraDist * arrowLen);
	glColor3ubv(_col.GetData());

	glBegin(GL_LINES);
		glVertex3dv(start.GetDataConst());
		glVertex3dv(end.GetDataConst());

		Vector3 p1 = end + invDir + sidewaysDir * arrowLen * 0.1;
		Vector3 p2 = end + invDir - sidewaysDir * arrowLen * 0.1;
		glVertex3dv(p1.GetDataConst());
		glVertex3dv(end.GetDataConst());
		glVertex3dv(p2.GetDataConst());
		glVertex3dv(end.GetDataConst());
	glEnd();

    glDisable( GL_LINE_SMOOTH );
	glDisable( GL_BLEND );
}


void RenderPointMarker(Vector3 const &point, char const *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);

	Vector3 end(point + Vector3(20,20,20));
	RenderArrow(end, point, 2.0);
	g_editorFont.DrawText3DCentre(end, 3.0, buf);
}


void PrintMatrix( const char *_name, GLenum _whichMatrix )
{
	GLdouble matrix[16];

	glGetDoublev( _whichMatrix, matrix );

	AppDebugOut( "\tMatrix: %s\n", _name );
	for (int row = 0; row < 4; row++) {
		AppDebugOut("\t\t");
		for (int col = 0; col < 4; col++) 
			AppDebugOut("% 13.1 ", matrix[col * 4 + row]);
		AppDebugOut("\n");
	}
	AppDebugOut("\n");
}

void PrintMatrices( const char *_title )
{
	static int numTimes = 0;
	numTimes++;
	if (numTimes > 10)
		return;

	AppDebugOut(
#ifdef USE_DIRECT3D
		"Direct3D: "
#else
		"OpenGL:   "
#endif
		"%s\n", _title);
	PrintMatrix( "Model View", GL_MODELVIEW_MATRIX );
	PrintMatrix( "Projection", GL_PROJECTION_MATRIX );

}

#endif // ifdef DEBUG_RENDER_ENABLED
