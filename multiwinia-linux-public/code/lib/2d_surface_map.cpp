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
SurfaceMap2D<T>::SurfaceMap2D(double _width, double _height, 
							  double _x0, double _y0, 
							  double _cellSizeX, double _cellSizeY,
							  T _outsideValue)
:	Array2D<T>(ceil(_width / _cellSizeX), ceil(_height / _cellSizeY), _outsideValue),
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
void SurfaceMap2D<T>::Initialise(double _width, double _height, 
								 double _x0, double _y0, 
								 double _cellSizeX, double _cellSizeY,
								 T _outsideValue)
{
	Array2D<T>::Initialise(ceil(_width / _cellSizeX), ceil(_height / _cellSizeY), _outsideValue);
	m_x0 = _x0;
	m_y0 = _y0;
	m_cellSizeX = _cellSizeX;
	m_cellSizeY = _cellSizeY;
	m_invCellSizeX = 1.0f / _cellSizeX;
	m_invCellSizeY = 1.0f / _cellSizeY;
}


template <class T>
T SurfaceMap2D<T>::GetValue(double _x, double _y) const
{
	_x -= m_x0;
	_y -= m_y0;

	double fractionalX = _x * m_invCellSizeX;
	double fractionalY = _y * m_invCellSizeY;
	
	unsigned short x1 = fractionalX;
	unsigned short y1 = fractionalY;
	unsigned short x2 = x1 + 1;
	unsigned short y2 = y1 + 1;

	fractionalX = fractionalX - floor(fractionalX);
	fractionalY = fractionalY - floor(fractionalY);

	if (x1 >= this->m_numColumns) x1 = 0; 
	if (x2 >= this->m_numColumns) x2 = 0;
	if (y1 >= this->m_numRows) y1 = 0; 
	if (y2 >= this->m_numRows) y2 = 0;

	T value11 = Array2D<T>::GetData(x1, y1); 
	T value12 = Array2D<T>::GetData(x1, y2); 
	T value21 = Array2D<T>::GetData(x2, y1); 
	T value22 = Array2D<T>::GetData(x2, y2); 

	double weight11 = (1.0f - fractionalX) * (1.0f - fractionalY);
	double weight12 = (1.0f - fractionalX) * (fractionalY);
	double weight21 = (fractionalX) * (1.0f - fractionalY);
	double weight22 = (fractionalX) * (fractionalY);

	T returnVal = value11 * weight11 +
				  value12 * weight12 +
				  value21 * weight21 +
				  value22 * weight22;

	return returnVal;
}


template <class T>
T const &SurfaceMap2D<T>::GetValueNearest(double _x, double _y) const
{
	return Array2D<T>::GetData(floor(_x * m_invCellSizeX), floor(_y * m_invCellSizeY));
}


template <class T>
T *SurfaceMap2D<T>::GetPointerNearest(double _x, double _y) const
{
	return Array2D<T>::GetPointer(floor(_x * m_invCellSizeX), floor(_y * m_invCellSizeY));
}


template <class T>
T SurfaceMap2D<T>::GetHighestValue() const
{
	T highest = Array2D<T>::GetData(0, 0);
	for (unsigned short y = 0; y < this->m_numRows; ++y)
	{
		for (unsigned short x = 0; x < this->m_numRows; ++x)
		{
			T val = Array2D<T>::GetData(x, y);
			if (val > highest)
			{
				highest = val;
			}
		}
	}

	return highest;
}


template <class T>
inline int SurfaceMap2D<T>::GetMapIndexX(double _realX) const
{
	return (_realX - m_x0) * m_invCellSizeX;
}


template <class T>
inline int SurfaceMap2D<T>::GetMapIndexY(double _realY) const
{
	return (_realY - m_y0) * m_invCellSizeY;
}


template <class T>
inline double SurfaceMap2D<T>::GetRealX(int _mapIndexX) const
{
	return _mapIndexX * m_cellSizeX + m_x0;
}


template <class T>
inline double SurfaceMap2D<T>::GetRealY(int _mapIndexY) const
{
	return _mapIndexY * m_cellSizeY + m_y0;
}
