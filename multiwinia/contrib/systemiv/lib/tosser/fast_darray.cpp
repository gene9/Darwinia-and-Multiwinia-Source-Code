#include "lib/universal_include.h"

#include <stdlib.h>

#include "lib/tosser/llist.h"



template <class T>
FastDArray <T>::FastDArray () 
	: DArray <T> ()
{
	numused = 0;
	freelist = NULL;
	firstfree = -1;
}


template <class T>
FastDArray <T>::FastDArray ( int newstepsize ) 
	: DArray<T> (newstepsize)
{
	numused = 0;
	freelist = NULL;
	firstfree = -1;
}


template <class T>
void FastDArray <T>::RebuildFreeList ()
{
	//  Reset free list

	delete freelist;
	freelist = new int[this->m_arraySize];
	firstfree = -1;
	int lastknownfree = -1;

	// Step through, rebuilding

	for ( int i = 0; i < this->m_arraySize; ++i ) {
		if ( this->shadow[i] == 0 ) {
			if ( firstfree == -1 ) {
				firstfree = i;
			}
			else {
				if ( lastknownfree != -1 ) {
					freelist[lastknownfree] = i;
				}
			}

			freelist[i] = -1;
			lastknownfree = i;
		}
		else {
			freelist[i] = -2;
		}
	}
}


template <class T>
void FastDArray <T>::RebuildNumUsed()
{
	numused = 0 ;
	for ( int i = 0; i < this->m_arraySize; ++i ) 
	{
		if ( this->shadow[i] == 1 ) ++numused;
	}
}


template <class T>
void FastDArray <T>::SetSize ( int newsize )
{
	if ( newsize > this->m_arraySize ) 
	{
		int oldarraysize = this->m_arraySize;

		this->m_arraySize = newsize;
		T *temparray = new T[ this->m_arraySize ];
		char *tempshadow = new char[ this->m_arraySize ];
		int *tempfreelist = new int[ this->m_arraySize ];
		
		int a;

		for ( a = 0; a < oldarraysize; ++a ) 
		{
			temparray[a] = this->array[a];
			tempshadow[a] = this->shadow[a];
			tempfreelist[a] = freelist[a];
		}
		
		for ( a = oldarraysize; a < this->m_arraySize; ++a )
			tempshadow[a] = 0;
		
		int oldfirstfree = firstfree;
		firstfree = oldarraysize;
		for ( a = oldarraysize; a < this->m_arraySize-1; ++a )
			tempfreelist[a] = a + 1;
		tempfreelist[this->m_arraySize-1] = oldfirstfree;

		delete[] this->array;
		delete[] this->shadow;
		delete[] freelist;
		
		this->array = temparray;
		this->shadow = tempshadow;
		freelist = tempfreelist;
	}
	else if ( newsize < this->m_arraySize ) 
	{
		this->m_arraySize = newsize;
		T *temparray = new T[this->m_arraySize];
		char *tempshadow = new char[this->m_arraySize];
		int *tempfreelist = new int[this->m_arraySize];

		for ( int a = 0; a < this->m_arraySize; ++a ) 
		{
			temparray[a] = this->array[a];
			tempshadow[a] = this->shadow[a];
			tempfreelist[a] = freelist[a];
		}

		delete[] this->array;
		delete[] this->shadow;
		delete[] freelist;
		
		this->array = temparray;
		this->shadow = tempshadow;
		freelist = tempfreelist;		

		RebuildNumUsed();
		RebuildFreeList();
	}
	else if ( newsize == this->m_arraySize ) 
	{
		// Do nothing
	}
}


template <class T>
void FastDArray <T>::Grow()
{
	if (this->m_stepSize == -1)
	{
		if (this->m_arraySize == 0)
		{
			SetSize( 1 );
		}
		else
		{
			// Double array size
			SetSize( this->m_arraySize * 2 );
		}
	}
	else
	{
		// Increase array size by fixed amount
		SetSize( this->m_arraySize + this->m_stepSize );
	}
}


template <class T>
int FastDArray <T>::PutData( const T &newdata )
{
    if ( firstfree == -1 )
	{
		// Must resize the array		
		Grow();
	}

    AppAssertArgs(firstfree != -1, "firstfree(%d)", firstfree);

    if ( firstfree == -1 )
	{
		// Must resize the array		
		Grow();
	}

	int freeslot = firstfree;
	int nextfree = freelist[freeslot];

    this->array[freeslot] = newdata;
	if ( this->shadow[freeslot] == 0 ) ++numused;
    this->shadow[freeslot] = 1;
	freelist[freeslot] = -2;
	firstfree = nextfree;
  
    return freeslot;
}


template <class T>
void FastDArray <T>::PutData( const T &newdata, int index )
{
    AppAssertArgs( index < this->m_arraySize && index >= 0, "index(%d), m_arraySize(%d)", index, this->m_arraySize );       

    this->array[index] = newdata;

    if ( this->shadow[index] == 0 ) 
	{
		this->shadow[index] = 1;
		++numused;
		RebuildFreeList();
	}
}


template <class T>
void FastDArray <T>::EmptyAndDelete()
{
	delete[] freelist;
	freelist = NULL;
    
	firstfree = -1;
	numused = 0;

    DArray<T>::EmptyAndDelete();
}

template <class T>
void FastDArray <T>::Empty()
{
	delete[] freelist;
	freelist = NULL;
    
	firstfree = -1;
	numused = 0;

	DArray<T>::Empty();
}


template <class T>
void FastDArray <T>::MarkUsed( int index )
{
    AppAssertArgs( index < this->m_arraySize && index >= 0, "index(%d), m_arraySize(%d)", index, this->m_arraySize );
    AppAssertArgs( this->shadow[index] == 0, "index(%d)", index );
    
    this->shadow[index] = 1;
	++numused;
    RebuildFreeList();
}


template <class T>
void FastDArray <T>::RemoveData( int index )
{
    AppAssertArgs( index < this->m_arraySize && index >= 0, "index(%d), m_arraySize(%d)", index, this->m_arraySize );
    AppAssertArgs( this->shadow[index] != 0, "index(%d)", index );
    
	--numused;
    this->shadow[index] = 0;
    freelist[index] = firstfree;
	firstfree = index;
}


template <class T>
T *FastDArray<T>::GetPointer ()
{
    return GetPointer( GetNextFree() );
}


template <class T>
T *FastDArray<T>::GetPointer(int index)
{
	return DArray<T>::GetPointer(index);
}


template <class T>
int FastDArray<T>::GetNextFree()
{
    if ( firstfree == -1 )
	{
		// Must resize the array		
		Grow();
	}

	int freeslot = firstfree;
	int nextfree = freelist[freeslot];
    
	if ( this->shadow[freeslot] == 0 ) 
	{
		++numused;
	}

    this->shadow[freeslot] = 1;
	freelist[freeslot] = -2;
	firstfree = nextfree;

    return freeslot;
}


template <class T>
int FastDArray <T>::NumUsed() const
{
	return numused;
}
