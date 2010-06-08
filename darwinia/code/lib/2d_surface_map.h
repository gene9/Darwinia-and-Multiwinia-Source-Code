#ifndef INCLUDE_2D_SURFACE_MAP_H
#define INCLUDE_2D_SURFACE_MAP_H


#include "lib/2d_array.h"


// ****************************************************************************
// Class SurfaceMap2D
// Builds on Array2D to provide continuous interpolating 2D map, rather than a
// discrete sampled one. In addition this class performs a coordinate space 
// transform so that data values can be looked up using world space co-ords 
// rather than direct indices into the 2D array.
// ****************************************************************************

template <class T>
class SurfaceMap2D: public Array2D<T>
{
public:
	// These variables are used when converting from real co-ords into
	// array indices
	float			m_x0;					
	float			m_y0;
	float			m_cellSizeX;
	float			m_cellSizeY;
	float			m_invCellSizeX;
	float			m_invCellSizeY;

public:
	SurfaceMap2D();
	SurfaceMap2D(float _width, float _height, 
				 float _x0, float _y0, 
				 float _cellSizeX, float _cellSizeY,
				 T _outsideValue);
	~SurfaceMap2D();

	void			Initialise(float _width, float _height, 
							   float _x0, float _y0, 
							   float _cellSizeX, float _cellSizeY,
							   T _outsideValue);

	T 				GetValue(float _x, float _y) const;
	T const			&GetValueNearest(float _x, float _y) const; // Like GetValue but without interpolation
	T				*GetPointerNearest(float _x, float _y) const;
	T				GetHighestValue() const;

    inline int		GetMapIndexX(float _realX) const;
    inline int		GetMapIndexY(float _realY) const;
	inline float	GetRealX(int _mapIndexX) const;
	inline float	GetRealY(int _mapIndexY) const;
};


#include "lib/2d_surface_map.cpp"


#endif

