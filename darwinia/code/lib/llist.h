//===============================================================//
//                        L L I S T                              //
//                                                               //
//                   By Christopher Delay                        //
//                           V1.3                                //
//===============================================================//

#ifndef _included_llist_h
#define _included_llist_h


//=================================================================
// Linked list object
// Use : A dynamicly sized list of data
// NOT sorted in any way - new data is simply added on at the end
// Indexes of data are not constant
// Sequential access is fast, random access is slow

template <class T>
class LListItem
{
protected:
public:
	T		  m_data;
	LListItem *m_next;
	LListItem *m_previous;

	LListItem();
	~LListItem();
};


template <class T>
class LList
{
protected:
	LListItem<T> *m_first;                 // Pointer to first node
	LListItem<T> *m_last;                  // Pointer to last node

	mutable LListItem<T> *m_previous;      // Used to get quick access
	mutable int m_previousIndex;           // for sequential reads (common)

	int m_numItems;

public:
	LList();
	LList( const LList<T> & );
	~LList();

	LList &operator = (const LList<T> &);

	inline void PutData			( const T &newdata );   // Adds in data at the end	
	void		PutDataAtEnd	( const T &newdata );
	void		PutDataAtStart	( const T &newdata );	
	void		PutDataAtIndex	( const T &newdata, int index );

    inline T	GetData			( int index ) const;	// slow unless sequential
    inline T   *GetPointer		( int index );			// slow unless sequential
	void		RemoveData		( int index );			// slow 
    int			FindData		( const T &data );		// -1 means 'not found'
  
    inline int	Size			() const;				// Returns the total size of the array
    inline bool ValidIndex		( int index ) const;

    void		Empty			();						// Resets the array to empty    
    void		EmptyAndDelete	();						// As above, deletes all data as well
	void        EmptyAndDeleteArray();                  // As above, but with delete[]

    inline T operator [] (int index);
};


//  ===================================================================

#include "llist.cpp"


#endif
