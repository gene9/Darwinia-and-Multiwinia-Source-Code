#ifndef INCLUDED_BOUNDED_ARRAY_H
#define INCLUDED_BOUNDED_ARRAY_H


// ****************************************************************************
// Class BoundedArray
// ****************************************************************************

template <class T>
class BoundedArray
{
protected:
	T				*m_data;
	unsigned int	m_numElements;

public:
	// Constructors
	BoundedArray();
	BoundedArray(BoundedArray *_otherArray);
	BoundedArray(unsigned int _numElements);

	// Destructor
	~BoundedArray();

	// Only need to call Initialise if you used the default constructor
	void Initialise(unsigned int _numElements);

	// Operators
    inline T &operator [] (unsigned int _index);
    inline const T &operator [] (unsigned int _index) const;

    inline unsigned int Size() const;

	void SetAll(T const &_value);
};


#include "lib/bounded_array.cpp"


#endif
