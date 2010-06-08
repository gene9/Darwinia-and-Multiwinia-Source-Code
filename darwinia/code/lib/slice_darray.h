//===============================================================//
//						  SLICE DARRAY                           //
//                                                               //
//                   By Christopher Delay                        //
//                           V1.3                                //
//===============================================================//

#ifndef _included_slice_darray_h
#define _included_slice_darray_h


#include "fast_darray.h"


//=================================================================
// Slice Dynamic array object
// Use : A dynamically sized list of data
// Which can be indexed into - an entry's index never changes
// Same as Fast DArray, but has data and methods to help "slicing" up a for loop
// across all the members.

template <class T>
class SliceDArray : public FastDArray <T>
{
protected:
    int totalNumSlices;
    int lastSlice;          // Slice number used last time GetNextSliceBounds was used
	int lastUpdated;        // Previous result returned as the "upper" value from GetNextSliceBounds

public:
    SliceDArray ();
    SliceDArray (int _totalNumSlices);
    SliceDArray ( int newstepsize, int _totalNumSlices );

    void Empty ();
    void GetNextSliceBounds (int slice, int *lower, int *upper);
    void SetTotalNumSlices(int _slices);
    int GetLastUpdated();
};


//  ===================================================================

#include "slice_darray.cpp"

#endif
