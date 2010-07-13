#include "lib/universal_include.h"

#include "lib/ogl_extensions.h"
#include "lib/render_utils.h"


void RenderSplitUpQuadTextured(
		float posNorth, float posSouth, float posEast, float posWest, float height,
		float texNorth, float texSouth, float texEast, float texWest, int steps)
{
    float sizeX = posWest - posEast;
    float sizeZ = posSouth - posNorth;
	float texSizeX = texWest - texEast;
	float texSizeZ = texSouth - texNorth;
	float posStepX = sizeX / (float)steps;
	float posStepZ = sizeZ / (float)steps;
	float texStepX = texSizeX / (float)steps;
	float texStepZ = texSizeZ / (float)steps;

    glBegin(GL_QUADS);
		for (int j = 0; j < steps; ++j)
		{
			float pz = posNorth + j * posStepZ;
			float tz = texNorth + j * texStepZ;

			for (int i = 0; i < steps; ++i)
			{
				float px = posEast + i * posStepX;
				float tx = texEast + i * texStepX;

				glTexCoord2f(tx + texStepX, tz);
				glVertex3f(px + posStepX, height, pz);

				glTexCoord2f(tx + texStepX, tz + texStepZ);
				glVertex3f(px + posStepX, height, pz + posStepZ);
				
				glTexCoord2f(tx, tz + texStepZ);
				glVertex3f(px, height, pz + posStepZ);

				glTexCoord2f(tx, tz);
				glVertex3f(px, height, pz);
			}
		}
	glEnd();
}


void RenderSplitUpQuadMultiTextured(
		float posNorth, float posSouth, float posEast, float posWest, float height,
		float texNorth1, float texSouth1, float texEast1, float texWest1,
		float texNorth2, float texSouth2, float texEast2, float texWest2, int steps)
{
    float sizeX = posWest - posEast;
    float sizeZ = posSouth - posNorth;
	float posStepX = sizeX / (float)steps;
	float posStepZ = sizeZ / (float)steps;
	
	float texSizeX1 = texWest1 - texEast1;
	float texSizeZ1 = texSouth1 - texNorth1;
	float texStepX1 = texSizeX1 / (float)steps;
	float texStepZ1 = texSizeZ1 / (float)steps;

	float texSizeX2 = texWest2 - texEast2;
	float texSizeZ2 = texSouth2 - texNorth2;
	float texStepX2 = texSizeX2 / (float)steps;
	float texStepZ2 = texSizeZ2 / (float)steps;

    glBegin(GL_QUADS);
		for (int j = 0; j < steps; ++j)
		{
			float pz = posNorth + j * posStepZ;
			float tz1 = texNorth1 + j * texStepZ1;
			float tz2 = texNorth2 + j * texStepZ2;

			for (int i = 0; i < steps; ++i)
			{
				float px = posEast + i * posStepX;
				float tx1 = texEast1 + i * texStepX1;
				float tx2 = texEast2 + i * texStepX2;

				gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, tx1 + texStepX1, tz1);
				gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, tx2 + texStepX2, tz2);
				glVertex3f(px + posStepX, height, pz);

				gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, tx1 + texStepX1, tz1 + texStepZ1);
				gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, tx2 + texStepX2, tz2 + texStepZ2);
				glVertex3f(px + posStepX, height, pz + posStepZ);
				
				gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, tx1, tz1 + texStepZ1);
				gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, tx2, tz2 + texStepZ2);
				glVertex3f(px, height, pz + posStepZ);

				gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, tx1, tz1);
				gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, tx2, tz2);
				glVertex3f(px, height, pz);
			}
		}
	glEnd();
}
