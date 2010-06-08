#ifndef INCLUDED_SORTING_HASH_TABLE_H
#define INCLUDED_SORTING_HASH_TABLE_H


#include "hash_table.h"


// Extends HashTable to provide a mechanism to iterating over all the 
// table elements in alphabetical order.
//
// - PutData and RemoveData are now O(n) rather than O(1)
// - It is now possible to iterate over all elements without
//   having to manually discard all the empty slots.


template <class T>
class SortingHashTable: public HashTable<T>
{
protected:
	short			*m_orderedIndices;								// Stores a chain of indices that make it easy to walk through the table in alphabetical key order
	short			m_firstOrderedIndex;							// The index of the alphabetically first table element
	short			m_nextOrderedIndex;								// Used by GetNextOrderedIndex

	void			Grow		();
	short			FindPrevKey	(char const *_key) const;			// Returns the index of the table element whose key is alphabetically previous to the specified key

public:
	SortingHashTable();
	~SortingHashTable();

	int				PutData		(char const *_key, T const &_data); // Returns slot used
	void			RemoveData	(char const *_key);					// Client still MUST delete if data is a pointer
	void			RemoveData	(unsigned int _index);				// Client still MUST delete if data is a pointer

	short			StartOrderedWalk();								// These two are used to while loop across all elements
	short			GetNextOrderedIndex();							// in alphabetical order. Both will return -1
																	// when there are no more elements
};


#include "sorting_hash_table.cpp"


#endif
