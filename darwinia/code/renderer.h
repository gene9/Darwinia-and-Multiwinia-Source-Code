#ifndef INCLUDED_RENDERER_H
#define INCLUDED_RENDERER_H

#if 0
  #include "app.h"
  #define CHECK_OPENGL_STATE() g_app->m_renderer->CheckOpenGLState();
#else
  #define CHECK_OPENGL_STATE()
#endif


#define PIXEL_EFFECT_GRID_RES	16


class Shape;
class ShapeFragment;
class Vector3;
class Matrix34;


class Renderer
{
public:
    int     m_fps;
	bool	m_displayFPS;
    bool    m_renderDebug;
	bool    m_displayInputMode;
	int 	m_renderingPoster;

    float   m_renderDarwinLogo;
    
private:
    float   m_nearPlane;
    float   m_farPlane;
	int		m_screenW;
	int		m_screenH;
	int		m_tileIndex;			// Used when rendering a poster

	double	m_totalMatrix[16];		// Modelview matrix * Projection matrix

	float	m_fadedness;			// 1.0 means black screen. 0.0 means not fade out at all.
	float	m_fadeRate;				// +ve means fading out, -ve means fading in
	float	m_fadeDelay;			// Amount of time left to wait before starting fade

	unsigned int m_pixelEffectTexId;
	float	m_pixelEffectGrid[PIXEL_EFFECT_GRID_RES][PIXEL_EFFECT_GRID_RES];	// -1.0 means cell not used
    int     m_pixelSize;
	
private:
	int		GetGLStateInt	(int pname) const;
	float	GetGLStateFloat	(int pname) const;

	void	RenderFlatTexture();
	void	RenderLogo		();
    void    RenderHelp      ();

	void	PaintPixels			();
    void    PreRenderPixelEffect();
    void    ApplyPixelEffect    ();

	void	RenderFadeOut	();
	void	RenderFrame		(bool withFlip = true);
    void	RenderPaused	();
	
public:
	Renderer				();

    void    Initialise      ();
	void	Restart			();
    
	void	Render			();
    void	FPSMeterAdvance	();

	void	BuildOpenGlState();

    float	GetNearPlane	() const;
    float	GetFarPlane		() const;
	void	SetNearAndFar	(float _nearPlane, float _farPlane);

	void	CheckOpenGLState() const;
	void	SetOpenGLState	() const;

	void	SetObjectLighting() const;
	void	UnsetObjectLighting() const;

    int     ScreenW() const;
    int     ScreenH() const;

	void	SetupProjMatrixFor3D() const;
	void	SetupMatricesFor3D() const;
	void	SetupMatricesFor2D() const;

	void	UpdateTotalMatrix();
	void	Get2DScreenPos(Vector3 const &_in, Vector3 *_out);
	const double* GetTotalMatrix();

	void	RasteriseSphere(Vector3 const &_pos, float _radius);
	void	MarkUsedCells(ShapeFragment const *_frag, Matrix34 const &_transform);
	void	MarkUsedCells(Shape const *_shape, Matrix34 const &_transform);

	bool	IsFadeComplete() const;
	void	StartFadeOut();
	void	StartFadeIn(float _delay);
};

#endif
