#include "lib/debug_utils.h"
#include "sorting_hash_table.h"


//*****************************************************************************
// Protected Functions
//*****************************************************************************

template <class T>
void SortingHashTable<T>::Grow()
{
	unsigned int oldSize = this->m_size;
	this->m_size *= 2;
	this->m_mask = (this->m_mask << 1) + 1;

	// Copy pointers to existing data
	char **oldKeys = this->m_keys;
	T *oldData = this->m_data;
	short *oldOrderedIndices = m_orderedIndices;

	// Make new data
	this->m_keys = new char *[this->m_size];
	this->m_data = new T [this->m_size];
	m_orderedIndices = new short [this->m_size];

	// Set all new data to zero
	memset(this->m_keys, 0, sizeof(char *) * this->m_size);
	memset(this->m_data, 0, sizeof(T) * this->m_size);
	memset(m_orderedIndices, -2, sizeof(short) * this->m_size);


	//
	// Go through the existing ordered index list, inserting elements into the
	// new table as we go

	short oldI = m_firstOrderedIndex;
	short newI = this->GetInsertPos(oldKeys[oldI]);
	m_firstOrderedIndex = newI;
	while (oldI != -1)
	{
		short nextOldI = oldOrderedIndices[oldI];

		this->m_keys[newI] = oldKeys[oldI];
		this->m_data[newI] = oldData[oldI];

		short nextNewI = (nextOldI != -1) ? this->GetInsertPos(oldKeys[nextOldI]) : -1;

		m_orderedIndices[newI] = nextNewI;
		
		oldI = nextOldI;
		newI = nextNewI;
	}

	this->m_slotsFree += this->m_size - oldSize;

	delete [] oldKeys;
	delete [] oldData;
	delete [] oldOrderedIndices;
}


// See header for description
template <class T>
short SortingHashTable<T>::FindPrevKey(char const *_key) const
{
	short prevI = -1;
	short i = m_firstOrderedIndex;
	
	while (1)
	{
		if (i == -1) return prevI;

		if (stricmp(this->m_keys[i], _key) >= 0) return prevI;

		prevI = i;
		i = m_orderedIndices[i];
	}
}



//*****************************************************************************
// Public Functions
//*****************************************************************************

template <class T>
SortingHashTable<T>::SortingHashTable()
:	HashTable<T>(),
	m_firstOrderedIndex(-1)
{
	m_orderedIndices = new short [this->m_size];
	memset(m_orderedIndices, 0, sizeof(short) * this->m_size);
}


template <class T>
SortingHashTable<T>::~SortingHashTable()
{
	delete [] m_orderedIndices;
}


template <class T>
int SortingHashTable<T>::PutData(char const *_key, T const &_data)
{
	// 
	// Make sure the table is big enough

	if (this->m_slotsFree * 2 <= this->m_size)
	{
		Grow();
	}


	//
	// Do the main insert

	unsigned int index = this->GetInsertPos(_key);
	AppDebugAssert(this->m_keys[index] == NULL);
	this->m_keys[index] = strdup(_key);
	this->m_data[index] = _data;
	this->m_slotsFree--;


	//
	// Insert us into the alphabetically order index list
	
	short i = FindPrevKey(_key);
	if (i == -1)
	{
		// Handle the case when the table is empty, or the new element is going
		// to be the new first alphabetical element
		m_orderedIndices[index] = m_firstOrderedIndex;
		m_firstOrderedIndex = index;
	}
	else
	{
		m_orderedIndices[index] = m_orderedIndices[i];
		m_orderedIndices[i] = index;
	}

	return index;
}


template <class T>
void SortingHashTable<T>::RemoveData(char const *_key)
{
	int index = this->GetIndex(_key);
	if (index >= 0)
	{
		RemoveData(index);
	}
}


template <class T>
void SortingHashTable<T>::RemoveData(unsigned int _index)
{
	//
	// Remove data

	delete [] this->m_keys[_index];
	this->m_keys[_index] = NULL;
	this->m_slotsFree++;


	//
	// Remove from ordered list

	short prevIndex = -1;
	short index = m_firstOrderedIndex;
	while (m_orderedIndices[index] != _index)
	{
		AppDebugAssert(index != -1);
		prevIndex = index;
		index = m_orderedIndices[index];
	}

	if (prevIndex == -1)
	{
		m_firstOrderedIndex = -1;
	}
	else
	{
		m_orderedIndices[prevIndex] = m_orderedIndices[index];
	}
}


template <class T>
short SortingHashTable<T>::StartOrderedWalk()
{
	m_nextOrderedIndex = m_orderedIndices[m_firstOrderedIndex];
	return m_firstOrderedIndex;
}


template <class T>
short SortingHashTable<T>::GetNextOrderedIndex()
{
	short rv = m_nextOrderedIndex;
	m_nextOrderedIndex = m_orderedIndices[m_nextOrderedIndex];
	return rv;
}
