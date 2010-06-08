#ifndef INCLUDED_HASH_TABLE_H
#define INCLUDED_HASH_TABLE_H


// Implements a simple hash table with null terminated char arrays for keys.
// - You can initialise it to any size greater than 1. 
// - PutData() checks if the table is more than half full. If it is the table 
//	 size is doubled and the data is re-indexed. 
// - The table never shrinks. 
// - The hash collision rule is just to increment through the table until
//   a free slot is found
// - Looking up a key is O(1) even when key does not exist

// *** Note on memory usage ***
// This class is optimised for speed not memory usage. The RAM used by m_data
// will always be between (2 x num slots used x sizeof(T)) and 
// (4 x num slots used x sizeof(T))


template <class T>
class HashTable
{
protected:
	char					**m_keys;
	T						*m_data;
	unsigned int			m_slotsFree;
	unsigned int			m_size;
	unsigned int			m_mask;
	mutable unsigned int	m_numCollisions;						// In the current data set

	unsigned int	HashFunc	(char const *_key) const;
	void			Grow		();
    unsigned int	GetInsertPos(char const *_key) const;			// Returns the index of the position where _key should be placed

public:
	HashTable		();
	~HashTable		();

	int				GetIndex	(char const *_key) const;			                    // Returns -1 if key isn't present

	int				PutData		(char const *_key, T const &_data);                     // Returns slot used

    T				GetData		(char const *_key, T const &_default = NULL) const;
	T				GetData		(unsigned int _index) const;
	T *				GetPointer	(char const *_key) const;
	T *				GetPointer  (unsigned int _index) const;

    void			RemoveData	(char const *_key);					                    // SLOW. Client still MUST delete if data is a pointer
	void			RemoveData	(unsigned int _index);				                    // SLOW. Client still MUST delete if data is a pointer

    void            MarkNotUsed (unsigned int _index);                                  // FAST, but you MUST call Rebuild afterwords or it will break
    void            Rebuild     ();

	void			Empty		();
	void			EmptyAndDelete();

	bool			ValidIndex	(unsigned int _x) const;
	unsigned int	Size		() const;							                    // Returns total table size, NOT number of slots used
	unsigned int	NumUsed		() const;

	T				operator [] (unsigned int _index) const;
	char const *	GetName		(unsigned int _index) const;

	void			DumpKeys	() const;
};


#include "hash_table.cpp"


#endif
