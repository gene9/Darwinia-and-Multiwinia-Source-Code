#ifndef INCLUDED_PRESIZED_ARRAY_H
#define INCLUDED_PRESIZED_ARRAY_H

#include "lib/debug_utils.h"


template<class T, unsigned int N>
class PresizedArray
{
public:
    T m_data[N];    // fixed-size array of elements of type T

public:
    T & operator[](unsigned int i) { AppDebugAssert(i < N); return m_data[i]; }
    const T & operator[](unsigned int i) const { AppDebugAssert(i < N); return m_data[i]; }

    static unsigned int Size() { return N; }

    // Assign one value to all elements
    void SetAll(const T& _value)
    {
		for (unsigned int i = 0; i < N; ++i)
		{
			m_data[i] = _value;
		}
    }
};


// Comparisons
//template <class T, std::size_t N>
//bool operator == (const PresizedArray<T,N>& x, const PresizedArray<T,N>& y) 
//{
//    return std::equal(x.begin(), x.end(), y.begin());
//}
//
//template <class T, std::size_t N>
//bool operator < (const PresizedArray<T,N>& x, const PresizedArray<T,N>& y) 
//{
//    return std::lexicographical_compare(x.begin(),x.end(),y.begin(),y.end());
//}
//
//template <class T, std::size_t N>
//bool operator != (const PresizedArray<T,N>& x, const PresizedArray<T,N>& y) 
//{
//    return !(x == y);
//}
//
//template <class T, std::size_t N>
//bool operator > (const PresizedArray<T,N>& x, const PresizedArray<T,N>& y) 
//{
//    return y < x;
//}
//
//template <class T, std::size_t N>
//bool operator <= (const PresizedArray<T,N>& x, const PresizedArray<T,N>& y) 
//{
//    return !(y < x);
//}
//
//template < class T, std::size_t N>
//bool operator >= (const PresizedArray<T,N>& x, const PresizedArray<T,N>& y) 
//{
//    return !(x < y);
//}


#endif
