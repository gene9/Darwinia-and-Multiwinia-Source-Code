#ifndef INCLUDED_2D_ARRAY_H
#define INCLUDED_2D_ARRAY_H


// ****************************************************************************
// Class Array2D
//
// Provides a mechanism to store a fixed number of elements arranged as a 2D
// array. Get and Set methods assume that values for X and Y are within bounds.
// ****************************************************************************

template <class T>
class Array2D
{
protected:
	T				*m_data;
	T				m_outsideValue;
	unsigned short	m_numColumns;
	unsigned short	m_numRows;

public:
	Array2D();
	Array2D(unsigned short _numColumns, unsigned short _numRows, T _outsideValue);
	~Array2D();
	
	// Only need to call Initialise if you used the default constructor
	void					Initialise		(unsigned short _numCols, unsigned short _numRows, T _outsideValue);

	inline T const			&GetData		(unsigned short _x, unsigned short _y) const;
	inline T				*GetPointer		(unsigned short _x, unsigned short _y);
	inline T const			*GetConstPointer(unsigned short _x, unsigned short _y) const;
	
	inline void				PutData			(unsigned short _x, unsigned short _y, T const &_value);
	inline void				AddToData		(unsigned short _x, unsigned short _y, T const &_value);
	
	inline unsigned short	GetNumRows		() const;
	inline unsigned short	GetNumColumns	() const;

	void					SetAll			(T const &_value);
};


#include "lib/2d_array.cpp"


#endif
