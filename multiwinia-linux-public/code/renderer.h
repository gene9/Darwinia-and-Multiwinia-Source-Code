#ifndef INCLUDED_RENDERER_H
#define INCLUDED_RENDERER_H

#ifdef OPENGL_DEBUG_ASSERTS
  #include "app.h"
  #define CHECK_OPENGL_STATE() g_app->m_renderer->CheckOpenGLState(false, __LINE__);
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
	int		m_lastFrameBufferTexture;

    float   m_renderDarwinLogo;

    float   m_gamma;
    
private:
    float   m_nearPlane;
    float   m_farPlane;
	int		m_screenW;
	int		m_screenH;
	float	m_texCoordW;
	float	m_texCoordH;
	int		m_tileIndex;			// Used when rendering a poster

	double	m_totalMatrix[16];		// Modelview matrix * Projection matrix

	float	m_fadedness;			// 1.0 means black screen. 0.0 means not fade out at all.
	float	m_fadeRate;				// +ve means fading out, -ve means fading in
	float	m_fadeDelay;			// Amount of time left to wait before starting fade

	GLuint  m_pixelEffectTexId;
	float	m_pixelEffectGrid[PIXEL_EFFECT_GRID_RES][PIXEL_EFFECT_GRID_RES];	// -1.0 means cell not used
    int     m_pixelSize;

	enum MenuTransition {
		MenuTransitionBeginning,
        MenuTransitionWaiting,
		MenuTransitionInProgress,
		MenuTransitionFinished
	};

	double	m_menuTransitionStartTime;
	double	m_menuTransitionDuration;
    int     m_menuTransitionDirection;
	MenuTransition m_menuTransitionState;
	
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

	void	AdvanceMenuTransition();
	void	RenderMenuTransition( float _f );
	void	CaptureFrameBuffer	();

	
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

	void	CheckOpenGLState(bool fullCheck, int lineNumber) const;
	void	SetOpenGLState	() const;

	void	SetObjectLighting() const;
	void	UnsetObjectLighting() const;

    int     ScreenW() const;
    int     ScreenH() const;
	void    SetScreenRes(int w, int h);

	void	SetupProjMatrixFor3D() const;
	void	SetupMatricesFor3D() const;
	void	SetupMatricesFor2D( bool _noOverscan = false ) const;

	void	UpdateTotalMatrix();
	void	Get2DScreenPos(Vector3 const &_in, Vector3 *_out);
	const double* GetTotalMatrix();

	void	RasteriseSphere(Vector3 const &_pos, float _radius);
	void	MarkUsedCells(ShapeFragment const *_frag, Matrix34 const &_transform);
	void	MarkUsedCells(Shape const *_shape, Matrix34 const &_transform);

	bool	IsFadeComplete() const;
	void	StartFadeOut();
	void	StartFadeIn(float _delay);

	void	InitialiseMenuTransition( float _delay, int direction );
	void	StartMenuTransition();
	bool	IsMenuTransitionComplete() const;

    void    Clip( int _x, int _y, int _w, int _h ); // takes the upper left coords of a rectangle, plus width and height
    void    EndClip();

#ifdef USE_DIRECT3D
    void    SetScreenGamma( float _gamma );
#endif
};

#endif
