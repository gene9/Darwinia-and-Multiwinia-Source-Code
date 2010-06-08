#include "lib/universal_include.h"


#include <stdlib.h>

#include "lib/debug_utils.h"
#include "lib/slice_darray.h"

template <class T>
SliceDArray <T> :: SliceDArray () 
	: FastDArray <T> (),
      totalNumSlices(0),
      lastSlice(-1),
      lastUpdated(0)
{
}

template <class T>
SliceDArray <T> :: SliceDArray (int _totalNumSlices) 
	: FastDArray <T> (),
      totalNumSlices(_totalNumSlices),
      lastSlice(-1),
      lastUpdated(0)
{
}


template <class T>
SliceDArray <T> :: SliceDArray (int _totalNumSlices, int newstepsize) 
	: FastDArray<T> (newstepsize),
      totalNumSlices(_totalNumSlices),
      lastSlice(-1),
      lastUpdated(0)
{
}


template <class T>
void SliceDArray<T>::Empty()
{
	lastSlice = -1;
	lastUpdated = 0;
	FastDArray<T>::Empty();
}


template <class T>
void SliceDArray <T>::GetNextSliceBounds (int _slice, int *_lower, int *_upper)
{
    DarwiniaDebugAssert (lastSlice == -1 ||
				 _slice == lastSlice + 1 || 
				 (_slice == 0 && lastSlice == totalNumSlices -1 ));

    if (_slice == 0)
    {
        *_lower = 0;
    }
    else
    {
        *_lower = lastUpdated + 1;
    }

    if (_slice == totalNumSlices - 1)
    {
        *_upper = Size() - 1;   
    }
    else
    {
        int numPerSlice = int(Size() / (float)totalNumSlices);
        *_upper = *_lower + numPerSlice;
    }

    lastUpdated = *_upper;
    lastSlice = _slice;
}


template <class T>
void SliceDArray <T>::SetTotalNumSlices(int _slices)
{
    totalNumSlices = _slices;
}


template <class T>
int SliceDArray <T>::GetLastUpdated()
{
    return lastUpdated;
}
