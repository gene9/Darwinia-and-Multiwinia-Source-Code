#ifndef AMIGA_LOADER_H
#define AMIGA_LOADER_H


#include "loaders/loader.h"


class Sierpinski3D;
class Star;


class AmigaLoader : public Loader
{
protected:
	void SetupFor2d();
	void SetupFor3d();
	void RenderStars(float _frameTime);
	void RenderScrollText(float _frameTime);
	void RenderLogo(float _frameTime);
	void RenderFracTri(float _frameTime);

	enum
	{
		ModeSilence,
		ModeHeader,
		ModeData,
		ModeScreen
	};

	Sierpinski3D	*m_sierpinski;

	float			m_scrollPhase;
	double			m_scrollOffsetX;	// Which pixel of the scroll message we are up to

	Star			*m_stars;
	int				m_numStars;

	float			m_timeSinceStart;

	float			m_fracTriPhase;
	float			m_fracTriSpinRate;

public:
	AmigaLoader();
    ~AmigaLoader();

	void Run();
};


#endif
