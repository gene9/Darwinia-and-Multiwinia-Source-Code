#ifndef __MATRIX_CACHE_H
#define __MATRIX_CACHE_H

#include <vector>
#include <algorithm>
#include <utility>

template< class T>
class MatrixCache 
{
public:
	MatrixCache( int _width, int _height )
		: m_width( _width ), m_height( _height ),
		  m_set( _width * _height, false ), m_values( _width * _height )
	{
	}

	T operator () ( int _x, int _y ) 
	{
		return Get( _x, _y );
	}

	virtual T Get( int _x, int _y ) 
	{
		int offset = _x * m_width + _y;
		if( m_set[offset] )
			return m_values[offset];
		else 
		{
			m_set[offset] = true;
			return m_values[offset] = Compute( _x, _y );
		}
	}

	virtual T Compute( int _x, int _y ) = 0;
					
private:
	int m_width, m_height;
	std::vector<bool> m_set;
	std::vector<T> m_values;
};

template <class T>
class SymmetricMatrixCache : public MatrixCache<T>
{
public:
	SymmetricMatrixCache( int _width, int _height )
		: MatrixCache<T>( _width, _height )
	{
	}

	virtual T Get( int _x, int _y )
	{
		if( _x > _y )
			return MatrixCache<T>::Get( _y, _x );
		else 
			return MatrixCache<T>::Get( _x, _y );
	}
};

#endif
