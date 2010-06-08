#include "lib/universal_include.h"
#include "lib/debug_utils.h"

#include "bounded_array.h"


// Constructor
template <class T>
BoundedArray<T>::BoundedArray()
:	m_numElements(0),
	m_data(NULL)
{
}


// Constructor
template <class T>
BoundedArray<T>::BoundedArray(unsigned int _numElements)
{
	Initialise( _numElements );
}


// Constructor
template <class T>
BoundedArray<T>::BoundedArray(BoundedArray &_otherArray)
{
    Initialise( _otherArray );
}


// Destructor
template <class T>
BoundedArray<T>::~BoundedArray()
{
	delete [] m_data;
	m_numElements = 0;
}


template <class T>
void BoundedArray<T>::Initialise(unsigned int _numElements)
{
	m_numElements = _numElements;
	m_data = new T[m_numElements];
}


template <class T>
void BoundedArray<T>::Initialise(BoundedArray &_otherArray)
{
    m_numElements = _otherArray.m_numElements;
    m_data = new T[m_numElements];
	
	for (unsigned int i = 0; i < m_numElements; ++i)
	{
		m_data[i] = _otherArray[i];
	}
}



template <class T>
T &BoundedArray<T>::operator [] (unsigned int _index)
{
	AppAssertArgs(_index < m_numElements && _index >= 0, "_index(%u), m_numElements(%u)", _index, m_numElements);
	return m_data[_index];
}


template <class T>
T const &BoundedArray<T>::operator [] (unsigned int _index) const
{
	AppAssertArgs(_index < m_numElements && _index >= 0, "_index(%u), m_numElements(%u)", _index, m_numElements);
	return m_data[_index];
}


template <class T>
unsigned int BoundedArray<T>::Size() const
{ 
	return m_numElements; 
}


template <class T>
void BoundedArray<T>::SetAll(T const &_value)
{
	for (int i = 0; i < m_numElements; ++i) 
	{
		m_data[i] = _value;
	}
}
