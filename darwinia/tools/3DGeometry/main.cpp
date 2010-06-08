#include "lib/universal_include.h"

#include <math.h>
#include <gl/GLU.H>

#include "lib/debug_utils.h"
#include "lib/math_utils.h"
#include "lib/matrix34.h"
#include "lib/sphere_renderer.h"
#include "lib/vector2.h"
#include "lib/vector3.h"


void DrawPoint(Vector3 &pos)
{
	Matrix34 planeMat;
	GetPlaneMatrix(Vector3(0,0,0), Vector3(1,0,0), Vector3(0,0,1), &planeMat);
	Vector3 posOnPlane;
	ProjectPointOntoPlane(pos, planeMat, &posOnPlane);

	glColor3ub(255,255,255);
	glPointSize(2.0f);
	glBegin(GL_POINTS);
		glVertex3fv(pos.GetData());
	glEnd();
	glColor3ub(128,128,128);
	glBegin(GL_POINTS);
		glVertex3fv(posOnPlane.GetData());
	glEnd();

	glColor3ub(128,128,128);
	glLineWidth(1.0f);
	glBegin(GL_LINES);
		glVertex3fv(pos.GetData());
		glVertex3fv(posOnPlane.GetData());
	glEnd();		
}


void DrawLine(Vector3 &start, Vector3 &end)
{
	DrawPoint(start);
	DrawPoint(end);
	glBegin(GL_LINES);
		glVertex3fv(start.GetData());
		glVertex3fv(end.GetData());
	glEnd();
}


void DrawTriPlane(Vector3 &t1, Vector3 &t2, Vector3 &t3)
{
	float planeSize = 4.0f;
	Vector3 triCentre = (t1 + t2 + t3) / 3.0f;
	Vector3 dir1 = (t2 - t1).Normalise();
	Vector3 dir2 = (t3 - t2).Normalise();
	Vector3 planeNorm = dir1 ^ dir2;
	dir1 = (dir2 ^ planeNorm).Normalise();
	triCentre -= dir1 * 0.5f * planeSize;
	triCentre -= dir2 * 0.5f * planeSize;

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glColor4ub(255,128,128, 80);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
		glVertex3fv(triCentre.GetData());
		glVertex3fv((triCentre + dir1 * planeSize).GetData());
		glVertex3fv((triCentre + dir1 * planeSize + dir2 * planeSize).GetData());
		glVertex3fv((triCentre + dir2 * planeSize).GetData());
	glEnd();
	glDisable(GL_BLEND);
	
	glColor3ub(255,128,128);
	glLineWidth(1.0f);
	glBegin(GL_LINE_LOOP);
		glVertex3fv(t1.GetData());
		glVertex3fv(t2.GetData());
		glVertex3fv(t3.GetData());
	glEnd();
}


void DrawSphere(Vector3 const &pos, float radius)
{
	static Sphere sphere;
	sphere.Render(pos, radius);
}


void DrawGrid(Vector3 const &_camPos)
{
	float camPosMag = _camPos.Mag();
	int scale = log(camPosMag);
	float multiplier = exp(scale) * 0.1f;

	glScalef(multiplier, multiplier, multiplier);
	glColor3ub(99,99,99);
	glLineWidth(1.0f);
	glBegin(GL_LINES);
		for (int x = -10; x < 10; x++)
		{
			for (int y = -10; y < 10; y++)
			{
				glVertex3f(x, 0, y);
				glVertex3f(x+1, 0, y);
			}
		}
		for (int x = -10; x < 10; x++)
		{
			for (int y = -10; y < 10; y++)
			{
				glVertex3f(x, 0, y);
				glVertex3f(x, 0, y + 1);
			}
		}
	glEnd();
	glColor3ub(255,255,255);
	glLineWidth(2.0f);
	glBegin(GL_LINES);
		glVertex3f(-10, 0, 0);
		glVertex3f(10, 0, 0);
		glVertex3f(0, 0, -10);
		glVertex3f(0, 0, 10);
	glEnd();

	glScalef(1/multiplier, 1/multiplier, 1/multiplier);
}


void MainUserLoop()
{
//	Vector3 point(Vector3(3,5,1));
//	float radius = 1.4f;
//	Vector3 t1(0,1.5,0);
//	Vector3 t2(1,0.5,1);
//	Vector3 t3(0,2,3);
//
//	DrawSphere(point, radius);
//
//	DrawPoint(point);
//	DrawTriPlane(t1, t2, t3);
//
//	bool intersect = SphereTriangleIntersection(point, radius, t1, t2, t3);

	Vector3 rayStartWS(781, 49.5, 535);
	Vector3 rayDirWS(0.226, -0.515, 0.826);
	Vector3 sphereCentreWS(800, 5.31, 601);
	DrawLine(rayStartWS, rayStartWS + rayDirWS * 20.0f);
	DrawSphere(sphereCentreWS, 20.5f);

	Vector3 rayStartLS1(-4.19, 4.95, -5.4);
	Vector3 rayDirLS1(0.816, -0.515, 0.262);
	Vector3 sphereCentreLS1(-0.407, 0.897, -0.765);
	DrawLine(rayStartLS1, rayStartLS1 + rayDirLS1 * 20.0f);
	DrawSphere(sphereCentreLS1, 20.5f);
}


void AppMain()
{
	Vector3 camFr(0.476459, -0.334238, 0.813186);
	Vector3 camUp(00.168968, 0.942489, 0.288382);
	Vector3 camRi = camUp ^ camFr;
	Vector3 camPos(-6, 10, -13);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50,
				   (float)SCREEN_W / (float)SCREEN_H, // Aspect ratio
				   0.1, 1000); // Far plane

	while (!g_keys[KEY_ESC])
	{
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		float camPosMag = camPos.Mag();
		float moveSpeed = 0.01f * camPosMag;
		int velx, vely;
		get_mouse_mickeys(&velx, &vely);
		if (mouse_b & 2)
		{
			camFr.RotateAroundY((float)-velx / 100.0f);
			camUp.RotateAroundY((float)-velx / 100.0f);
			camRi = camUp ^ camFr;
			camFr.RotateAround(camRi, (float)vely / 100.0f);
			camUp.RotateAround(camRi, (float)vely / 100.0f);
			moveSpeed *= 3.0f;
		}

		if (key[KEY_W])	camPos += camFr * moveSpeed;
		if (key[KEY_S])	camPos -= camFr * moveSpeed;
		if (key[KEY_A])	camPos += camRi * moveSpeed;
		if (key[KEY_D])	camPos -= camRi * moveSpeed;
		if (key[KEY_Q]) camPos.y -= moveSpeed;
		if (key[KEY_E]) camPos.y += moveSpeed;

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		Vector3 lookAtPos = camPos + camFr;
		gluLookAt(camPos.x, camPos.y, camPos.z,
				  lookAtPos.x, lookAtPos.y, lookAtPos.z,
				  camUp.x, camUp.y, camUp.z);
	
		DrawGrid(camPos);

		// *** Do the maths we actually want to test
		MainUserLoop();

		allegro_gl_flip();
	}

	return 0;
}


END_OF_MAIN()
