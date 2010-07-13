#ifndef INCLUDED_LANDSCAPE_RENDERER_H
#define INCLUDED_LANDSCAPE_RENDERER_H


#include "lib/2d_surface_map.h"
#include "lib/tosser/fast_darray.h"
#include "lib/rgb_colour.h"
#include "lib/texture_uv.h"
#include "lib/vector3.h"
#include "lib/float_vector3.h"


class BitmapRGBA;


class LandVertex
{
public:
	FloatVector3 m_pos;
	FloatVector3 m_norm;
	RGBAColour	 m_col;
	TextureUV	 m_uv;
};


//*****************************************************************************
// Class LandTriangleStrip
//*****************************************************************************

class LandTriangleStrip
{
public:
	int				m_firstVertIndex;
	int				m_numVerts;

	LandTriangleStrip(): m_firstVertIndex(-1), m_numVerts(-2) {}
};


//*****************************************************************************
// Class LandscapeRenderer
//*****************************************************************************

struct IDirect3DVertexBuffer9;

class LandscapeRenderer
{
protected:
	enum
	{
		RenderModeVertexArray,
		RenderModeDisplayList,
		RenderModeVertexBufferObject,
		RenderModeVertexBufferDirect3D
	};

    const BitmapRGBA      *m_landscapeColour;
	float			m_highest;
	int				m_renderMode;

	FastDArray		<LandVertex> m_verts;

	GLuint			m_vertexBuffer;

	FastDArray		<LandTriangleStrip *> m_strips;

	void BuildVertArrayAndTriStrip(SurfaceMap2D <double> *_heightMap);
	void BuildNormArray();
	void BuildUVArray(SurfaceMap2D <double> *_heightMap);
	void GetLandscapeColour(float _height, float _gradient, 
							unsigned int _x, unsigned int _y, RGBAColour *_colour);
	void BuildColourArray();

	void DoBindBuffers();

public:
	static const unsigned int	m_posOffset;
	static const unsigned int	m_normOffset;
	static const unsigned int	m_colOffset;
	static const unsigned int	m_uvOffset;

public:
	int				m_numTriangles;

	LandscapeRenderer(SurfaceMap2D <double> *_heightMap);
	~LandscapeRenderer();

#ifdef USE_DIRECT3D
	void ReleaseD3DPoolDefaultResources();
	void ReleaseD3DResources();
#endif

	void BuildOpenGlState(SurfaceMap2D <double> *_heightMap);

	void Initialise();

	void RenderMainSlow();
	void RenderOverlaySlow();
	void Render();
};


#endif
