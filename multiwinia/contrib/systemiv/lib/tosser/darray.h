#ifndef _included_darray_h
#define _included_darray_h


//=================================================================
// Dynamic array template class
// Use : A dynamically sized list of data
// Which can be indexed into - an entry's index never changes
// A record is kept of which elements are in use. Accesses to 
// elements that are not in use will cause an assert.
//=================================================================


template <class T>
class DArray
{
protected:
    int         m_stepSize;								                // -1 means each step doubles the array size
    int         m_arraySize;
    T           *array;
    char        *shadow;								                // 0=not used, 1=used
    
	void        Grow();

public:
    DArray      ();
    DArray      ( int newstepsize );
    ~DArray     ();

    void        SetSize		    ( int newsize );
	void        SetStepSize	    ( int newstepsize );
	void        SetStepDouble	();						                // Switches to array size doubling when growing, rather than incrementing by m_stepSize

	inline T    GetData	    ( int index ) const;
	inline T    *GetPointer ( int index ) const;

    int         PutData		( const T &newdata );	                    // Returns index used
    inline void PutData	    ( const T &newdata, int index );
    
    void        RemoveData 	( int index );    
	void        MarkUsed    ( int index );
	int         FindData	( const T &data ) const;                    // -1 means 'not found'
    
    int         NumUsed		() const;				                    // Returns the number of used entries
    inline int  Size		() const;				                    // Returns the total size of the array
    
    inline bool ValidIndex ( int index ) const;	                        // Returns true if the index contains used data
    
    void Empty			();						                        // Resets the array to empty    
	void EmptyAndDelete ();						                        // Same as Empty() but deletes the elements that are pointed to as well
    
    T& operator         [] (int index);
	const T& operator   [] (int index) const;
};

//  ===================================================================

#include "darray.cpp"

#endif

