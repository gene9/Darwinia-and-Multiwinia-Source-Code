#ifndef _included_fdarray_h
#define _included_fdarray_h

#include "darray.h"

//=================================================================
// Fast Dynamic array object
// Use : A dynamically sized list of data
// Which can be indexed into - an entry's index never changes
// Same as DArray, but has new advantages: Insert is MUCH faster
//=================================================================


template <class T>
class FastDArray: public DArray <T>
{
protected:
	int         numused;
	int	        *freelist;
	int         firstfree;

	void RebuildFreeList();									// SLOW
	void RebuildNumUsed	();									// SLOW
	void Grow			();									// SLOW

public:
    FastDArray ();
    FastDArray ( int newstepsize );

    void        SetSize		( int newsize );					// SLOW

    inline int  PutData	    ( const T &newdata );				// FAST Returns index used
    void        PutData		( const T &newdata, int index );	// SLOW
    
	void        MarkUsed	( int index );						// SLOW
	inline void RemoveData  ( int index );					    // FAST
    
    inline T    *GetPointer ();                                 // FAST Returns next free element, sets to 'used'
    inline T    *GetPointer (int index);                        // FAST Returns next free element, sets to 'used'
    inline int  GetNextFree ();								    // FAST Sets the returned index to 'used'

    inline int  NumUsed() const;								// FAST Returns the number of used entries 
       
    void        Empty();										// FAST Resets the array to empty    
    void        EmptyAndDelete();                               // FAST
};


//  ===================================================================

#include "fast_darray.cpp"

#endif
