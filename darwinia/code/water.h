#ifndef INCLUDED_WATER_H
#define INCLUDED_WATER_H

// This module renders the sea. Conceptually the sea is a finite plane twice the
// size of the land rectangle. The land rectangle has one corner at 0,0 and the
// opposite corner at landSizeX,landSizeZ. The sea plane is from
// -landSizeX/2, -landSizeZ/2 to 1.5*landSizeX,1.5*landSizeZ.

// The dynamic sea is made from triangle strips. Strips are only made in places
// where the sea is visible. Rather than the sea looking the same everywhere,
// the water depth is used to add some character. 1, it is used to modulate
// the brightness of the polys - darker in deeper water. 2, it is used to
// alter the wave amplitude - smaller waves near the shore. 3, it is used to
// change the colouring - whiter waves near the shore.


#include "lib/fast_darray.h"
#include "lib/rgb_colour.h"
#include "lib/2d_surface_map.h"
#include "lib/vector3.h"


// ****************************************************************************
// Class WaterTriangleStrip
// ****************************************************************************

class WaterTriangleStrip
{
public:
	// Data used at render time
	int m_startRenderVertIndex;
	int m_numVerts;
};


class WaterVertex
{
public:
	Vector3		m_pos;
	RGBAColour	m_col;
	Vector3     m_normal;
};

// ****************************************************************************
// Class Water
// ****************************************************************************

struct IDirect3DVertexBuffer9;

class Water
{
protected:
	// Render data - referenced directly by OpenGL
	FastDArray		<WaterVertex> m_renderVerts;
	FastDArray		<WaterTriangleStrip *> m_strips;

#ifdef USE_DIRECT3D
	IDirect3DVertexBuffer9	*m_vertexBuffer;
#endif

	// Extra
	float			*m_waterDepths;			// 1-to-1 mapping with verts. 1.0 is deepest, 0.0 is shallowest
	float			*m_shoreNoise;			// 1-to-1 mapping with verts. Stores the extra whitening factor for polys near the shore
	SurfaceMap2D	<float>	*m_waterDepthMap;
	Array2D			<bool> *m_flatWaterTiles;// 16x16 array that stores whether the under water poly is needed

	float		    m_cellSize;				// Size of quads in sea mesh

	// Lookup table containing a range of colours from black to white via some pretty colours
	RGBAColour	    *m_colourTable;
	unsigned short	m_numColours;

	// Lookup tables containing some nice waves
	float			*m_waveTableX;
	float			*m_waveTableZ;
	int				m_waveTableSizeX;
	int				m_waveTableSizeZ;

	bool			m_renderWaterEffect;

	bool			IsVertNeeded			(float x, float z);
	void			BuildTriangleStrips		();

	void			RenderFlatWaterTiles	(float posNorth, float posSouth, float posEast, float posWest, float height,
											 float texNorth1, float texSouth1, float texEast1, float texWest1,
											 float texNorth2, float texSouth2, float texEast2, float texWest2, int steps);
	void			RenderFlatWater	        ();
	void			RenderReflectiveWater   ();
    void			UpdateDynamicWater      ();
	void			RenderDynamicWater      ();

public:
    Water();
	~Water();
#ifdef USE_DIRECT3D
	void            ReleaseD3DPoolDefaultResources();
	void            ReleaseD3DResources     ();
#endif

	void            GenerateLightMap		();
	inline RGBAColour const &GetColour(int _brightness);

	void			BuildOpenGlState		();

	void            Advance                 ();
    void			Render					();
};


inline RGBAColour const &Water::GetColour(int _brightness)
{
	if (_brightness >= m_numColours)
	{
		return m_colourTable[m_numColours - 1];
	}
	else if (_brightness < 0)
	{
		return m_colourTable[0];
	}

	return m_colourTable[_brightness];
}


#endif
