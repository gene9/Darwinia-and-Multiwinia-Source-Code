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
	double			m_x0;					
	double			m_y0;
	double			m_cellSizeX;
	double			m_cellSizeY;
	double			m_invCellSizeX;
	double			m_invCellSizeY;

public:
	SurfaceMap2D();
	SurfaceMap2D(double _width, double _height, 
				 double _x0, double _y0, 
				 double _cellSizeX, double _cellSizeY,
				 T _outsideValue);
	~SurfaceMap2D();

	void			Initialise(double _width, double _height, 
							   double _x0, double _y0, 
							   double _cellSizeX, double _cellSizeY,
							   T _outsideValue);

	T 				GetValue(double _x, double _y) const;
	T const			&GetValueNearest(double _x, double _y) const; // Like GetValue but without interpolation
	T				*GetPointerNearest(double _x, double _y) const;
	T				GetHighestValue() const;

    inline int		GetMapIndexX(double _realX) const;
    inline int		GetMapIndexY(double _realY) const;
	inline double	GetRealX(int _mapIndexX) const;
	inline double	GetRealY(int _mapIndexY) const;
};


#include "lib/2d_surface_map.cpp"


#endif

