#include "lib/universal_include.h"

#include "lib/3d_sprite.h"

#include "app.h"
#include "camera.h"
#include "renderer.h"


void Render3DSprite(Vector3 const &_pos, float _width, float _height, int _textureId)
{
	Vector3 camUp = g_app->m_camera->GetUp();
	Vector3 camRight = (camUp ^ g_app->m_camera->GetFront()) * (_width * 0.5f);
	camUp *= _height;

	Vector3 bottomLeft(_pos - camRight);
	Vector3 bottomRight(_pos + camRight);
	Vector3 topLeft(bottomLeft + camUp);
	Vector3 topRight(bottomRight + camUp);

	glEnable(GL_TEXTURE_2D);
	//glColor3ub(255, 255, 255);
	glDisable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, _textureId);
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glBegin(GL_QUADS);
		glTexCoord2f(1, 1);
		glVertex3fv(topLeft.GetData());
		glTexCoord2f(0, 1);
		glVertex3fv(topRight.GetData());
		glTexCoord2f(0, 0);
		glVertex3fv(bottomRight.GetData());
		glTexCoord2f(1, 0);
		glVertex3fv(bottomLeft.GetData());
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
}
