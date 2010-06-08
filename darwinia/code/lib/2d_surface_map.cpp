#include "lib/2d_surface_map.h"


template <class T>
SurfaceMap2D<T>::SurfaceMap2D()
:	Array2D<T>(),
	m_x0(0.0f),
	m_y0(0.0f),
	m_cellSizeX(0.0f),
	m_cellSizeY(0.0f),
	m_invCellSizeX(0.0f),
	m_invCellSizeY(0.0f)
{
}


template <class T>
SurfaceMap2D<T>::SurfaceMap2D(float _width, float _height, 
							  float _x0, float _y0, 
							  float _cellSizeX, float _cellSizeY,
							  T _outsideValue)
:	Array2D<T>(ceilf(_width / _cellSizeX), ceilf(_height / _cellSizeY), _outsideValue),
	m_x0(_x0),
	m_y0(_y0),
	m_cellSizeX(_cellSizeX),
	m_cellSizeY(_cellSizeY),
	m_invCellSizeX(1.0f / _cellSizeX),
	m_invCellSizeY(1.0f / _cellSizeY)
{
}


template <class T>
SurfaceMap2D<T>::~SurfaceMap2D()
{
	m_x0 = 0.0f;
	m_y0 = 0.0f;
	m_cellSizeX = 0.0f;
	m_cellSizeY = 0.0f;
	m_invCellSizeX = 0.0f;
	m_invCellSizeY = 0.0f;
}


template <class T>
void SurfaceMap2D<T>::Initialise(float _width, float _height, 
								 float _x0, float _y0, 
								 float _cellSizeX, float _cellSizeY,
								 T _outsideValue)
{
	Array2D<T>::Initialise(ceilf(_width / _cellSizeX), ceilf(_height / _cellSizeY), _outsideValue);
	m_x0 = _x0;
	m_y0 = _y0;
	m_cellSizeX = _cellSizeX;
	m_cellSizeY = _cellSizeY;
	m_invCellSizeX = 1.0f / _cellSizeX;
	m_invCellSizeY = 1.0f / _cellSizeY;
}


template <class T>
T SurfaceMap2D<T>::GetValue(float _x, float _y) const
{
	_x -= m_x0;
	_y -= m_y0;

	float fractionalX = _x * m_invCellSizeX;
	float fractionalY = _y * m_invCellSizeY;
	
	unsigned short x1 = fractionalX;
	unsigned short y1 = fractionalY;
	unsigned short x2 = x1 + 1;
	unsigned short y2 = y1 + 1;

	fractionalX = fractionalX - floorf(fractionalX);
	fractionalY = fractionalY - floorf(fractionalY);

	if (x1 >= m_numColumns) x1 = 0; 
	if (x2 >= m_numColumns) x2 = 0;
	if (y1 >= m_numRows) y1 = 0; 
	if (y2 >= m_numRows) y2 = 0;

	T value11 = GetData(x1, y1); 
	T value12 = GetData(x1, y2); 
	T value21 = GetData(x2, y1); 
	T value22 = GetData(x2, y2); 

	float weight11 = (1.0f - fractionalX) * (1.0f - fractionalY);
	float weight12 = (1.0f - fractionalX) * (fractionalY);
	float weight21 = (fractionalX) * (1.0f - fractionalY);
	float weight22 = (fractionalX) * (fractionalY);

	T returnVal = value11 * weight11 +
				  value12 * weight12 +
				  value21 * weight21 +
				  value22 * weight22;

	return returnVal;
}


template <class T>
T const &SurfaceMap2D<T>::GetValueNearest(float _x, float _y) const
{
	return GetData(floorf(_x * m_invCellSizeX), floorf(_y * m_invCellSizeY));
}


template <class T>
T *SurfaceMap2D<T>::GetPointerNearest(float _x, float _y) const
{
	return GetPointer(floorf(_x * m_invCellSizeX), floorf(_y * m_invCellSizeY));
}


template <class T>
T SurfaceMap2D<T>::GetHighestValue() const
{
	T highest = GetData(0, 0);
	for (unsigned short y = 0; y < m_numRows; ++y)
	{
		for (unsigned short x = 0; x < m_numRows; ++x)
		{
			T val = GetData(x, y);
			if (val > highest)
			{
				highest = val;
			}
		}
	}

	return highest;
}


template <class T>
inline int SurfaceMap2D<T>::GetMapIndexX(float _realX) const
{
	return (_realX - m_x0) * m_invCellSizeX;
}


template <class T>
inline int SurfaceMap2D<T>::GetMapIndexY(float _realY) const
{
	return (_realY - m_y0) * m_invCellSizeY;
}


template <class T>
inline float SurfaceMap2D<T>::GetRealX(int _mapIndexX) const
{
	return _mapIndexX * m_cellSizeX + m_x0;
}


template <class T>
inline float SurfaceMap2D<T>::GetRealY(int _mapIndexY) const
{
	return _mapIndexY * m_cellSizeY + m_y0;
}
