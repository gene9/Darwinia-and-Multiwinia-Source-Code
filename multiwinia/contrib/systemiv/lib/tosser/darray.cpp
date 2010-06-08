#include <stdlib.h>

#include "lib/debug_utils.h"
#include "darray.h"


template <class T>
DArray<T>::DArray()
{
    m_stepSize = 1;
    m_arraySize = 0;
    array = NULL;
    shadow = NULL;
}


template <class T>
DArray<T>::DArray( int newstepsize )
{
    m_stepSize = newstepsize;
    m_arraySize = 0;
    array = NULL;
    shadow = NULL;
}


template <class T>
DArray<T>::~DArray()
{
    Empty();
}


template <class T>
void DArray<T>::SetSize( int newsize )
{
	if ( newsize > m_arraySize ) 
	{
		int oldarraysize = m_arraySize;

		m_arraySize = newsize;
		T *temparray = new T[ m_arraySize ];
		char *tempshadow = new char[ m_arraySize ];
		
		int a;

		for ( a = 0; a < oldarraysize; ++a ) 
		{
			temparray[a] = array[a];
			tempshadow[a] = shadow[a];
		}
		
		for ( a = oldarraysize; a < m_arraySize; ++a ) 
		{
			tempshadow[a] = 0;
		}
		
		delete [] array;
		delete [] shadow;
		
		array = temparray;
		shadow = tempshadow;
	}
	else if ( newsize < m_arraySize ) 
	{
		m_arraySize = newsize;
		T *temparray = new T[m_arraySize];
		char *tempshadow = new char[m_arraySize];

		for ( int a = 0; a < m_arraySize; ++a ) 
		{
			temparray[a] = array[a];
			tempshadow[a] = shadow[a];
		}

		delete [] array;
		delete [] shadow;
		
		array = temparray;
		shadow = tempshadow;
	}
	else if ( newsize == m_arraySize ) 
	{
		// Do nothing
	}
}


template <class T>
void DArray<T>::Grow()
{
	if (m_stepSize == -1)
	{
		// Double array size
		if (m_arraySize == 0)
		{
			SetSize(1);
		}
		else
		{
			SetSize( m_arraySize * 2 );
		}
	}
	else
	{
		// Increase array size by fixed amount
		SetSize( m_arraySize + m_stepSize );
	}
}


template <class T>
void DArray<T>::SetStepSize( int newstepsize )
{
	m_stepSize = newstepsize;
}


template <class T>
void DArray<T>::SetStepDouble()
{
	m_stepSize = -1;
}


template <class T>
int DArray<T>::PutData( const T &newdata )
{
    int freeslot = -1;				 // Find a free space
    
    for ( int a = 0; a < m_arraySize; ++a ) 
	{
		if ( shadow[a] == 0 ) 
		{
			freeslot = a;
			break;
		}
    }
    
    if ( freeslot == -1 )			// Must resize the array
	{			 
		freeslot = m_arraySize;
		Grow();
    }
	    
    array[freeslot] = newdata;
    shadow[freeslot] = 1;
    
    return freeslot;
}


template <class T>
void DArray<T>::PutData( const T &newdata, int index )
{
    AppAssertArgs( index >= 0, "index(%d)", index );

    while( index >= m_arraySize )
    {
        Grow();
    }

    array[index] = newdata;
    shadow[index] = 1;
}


template <class T>
void DArray<T>::Empty()
{
    delete [] array;
    delete [] shadow;
    
    array = NULL;
    shadow = NULL;
    
    m_arraySize = 0;
}


template <class T>
void DArray<T>::EmptyAndDelete()
{
	for (int i = 0; i < m_arraySize; ++i)
	{
		if (ValidIndex(i))
		{
			delete array[i];
		}
	}

	Empty();
}


template <class T>
T DArray<T>::GetData( int index ) const
{
	AppAssertArgs(index < m_arraySize && index >= 0, "index(%d), m_arraySize(%d)", index, m_arraySize);
	AppAssertArgs(shadow[index] != 0, "index(%d)", index);
	
	return array[index];
}


template <class T>
T *DArray<T>::GetPointer( int index ) const
{
	AppAssertArgs(index < m_arraySize && index >= 0, "index(%d), m_arraySize(%d)", index, m_arraySize);
	AppAssertArgs(shadow[index] != 0, "index(%d)", index);

    return &( array[index] );
}


template <class T>
inline T& DArray<T>::operator [] (int index)
{
    // assert used instead of SnlAssert
    // because this is inline
	AppAssertArgs(index < m_arraySize && index >= 0, "index(%d), m_arraySize(%d)", index, m_arraySize);
	AppAssertArgs(shadow[index] != 0, "index(%d)", index);
    
    return array[index];    
}


template <class T>
inline const T& DArray<T>::operator [] (int _index) const
{
	AppAssertArgs(_index < m_arraySize && _index >= 0, "_index(%d), m_arraySize(%d)", _index, m_arraySize);
	AppAssertArgs(shadow[_index] != 0, "_index(%d)", _index);
    
    return array[_index];    
}


template <class T>
void DArray<T>::MarkUsed( int _index )
{
	AppAssertArgs(_index < m_arraySize && _index >= 0, "_index(%d), m_arraySize(%d)", _index, m_arraySize);
	AppAssertArgs(shadow[_index] == 0, "_index(%d)", _index);
    
    shadow[_index] = 1;
}


template <class T>
void DArray<T>::RemoveData( int index )
{
	AppAssertArgs(index < m_arraySize && index >= 0, "index(%d), m_arraySize(%d)", index, m_arraySize);
	AppAssertArgs(shadow[index] != 0, "index(%d)", index);
    
    shadow[index] = 0;
}


template <class T>
int DArray<T>::NumUsed() const
{
    int count = 0;
    
    for ( int a = 0; a < m_arraySize; ++a )
	{
		if ( shadow[a] == 1 )	++count;
	}
		
    return count;
}


template <class T>
int DArray<T>::Size() const
{
    return m_arraySize;
}


template <class T>
bool DArray<T>::ValidIndex( int index ) const
{
    if (index >= m_arraySize || index < 0 )
	{
		return false;
	}
    
    if (!shadow [index])
	{
		return false;
	}
    
    return true;
}


template <class T>
int DArray<T>::FindData( const T &newdata ) const
{
    for ( int a = 0; a < m_arraySize; ++a )
	{
		if ( shadow [a] )
		{
		    if ( array [a] == newdata )	return a;
		}
	}
    
    return -1;    
}
