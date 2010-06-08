
#ifndef _included_clouds_h
#define _included_clouds_h

#include "lib/vector3.h"


class Clouds
{
protected:
    Vector3     m_offset;
    Vector3     m_vel;
	float		m_faceTime;

	void RenderQuad		(float posNorth, float posSouth, float posEast, float posWest, float height,
						 float texNorth, float texSouth, float texEast, float texWest);

public:
    Clouds();

    void Advance();

    void Render         ( float _predictionTime );

    void RenderFlat     ( float _predictionTime );
    void RenderBlobby   ( float _predictionTime );
    void RenderSky		();
	void RenderFace		();

	void ShowDrSFace	();
};


#endif
