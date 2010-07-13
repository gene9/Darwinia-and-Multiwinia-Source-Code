// Used for ints, floats and doubles, taking into account endianness

#include "lib/debug_utils.h"

inline void reverseCopy( char *_dest, const char *_src, size_t count )
{
	_src += count;
	while (count--)
		*(_dest++) = *(--_src);
} 


template <class T>
std::istream &ReadNetworkValue( std::istream &input, T &v )
{
#ifndef __BIG_ENDIAN__
	return input.read( (char *) &v, sizeof(T) );
#else
	char u[sizeof(T)];
	input.read( u, sizeof(T) );
	
	char *w = (char *) &v;
	reverseCopy(w, u, sizeof(T));
	return input;
#endif
}


template <class T>
std::ostream &WriteNetworkValue( std::ostream &output, T v )
{
#ifndef __BIG_ENDIAN__
	return output.write( (char *) &v, sizeof(T) );
#else
	const char *u = (const char *) &v;
	char data[sizeof(T)];
	reverseCopy( data, u, sizeof(T) );
	return output.write( data, sizeof(T) );
#endif	
}


inline
std::istream &ReadNetworkValue( std::istream &input, bool &v )
{
	v = input.get() != 0;
	return input;
}


inline
std::ostream &WriteNetworkValue( std::ostream &output, bool v )
{
	return output.put( v != 0 );
}


inline
std::istream &ReadNetworkValue( std::istream &input, char &v )
{
	v = input.get();
	return input;	
}


inline
std::ostream &WriteNetworkValue( std::ostream &output, char v )
{
	return output.put( v );
}


template <class T>
void WriteNumbersData	( std::ostream &output, const T *data, unsigned numElems, unsigned maxDataSize )
{
	AppAssert( numElems * sizeof(T) < maxDataSize );
	WritePackedInt( output, numElems );
	for (unsigned i = 0; i < numElems; i++)
	{
		WriteNetworkValue( output, data[i] );
	}
}

template <class T>
T *ReadNumbersData		( std::istream &input, unsigned *numElems, unsigned maxDataSize )
{
	*numElems = ReadPackedInt( input );
	AppAssert( input && *numElems * sizeof(T) < maxDataSize );

	T *data = new T[*numElems];
	for (unsigned i = 0; i < *numElems; i++)
	{
		ReadNetworkValue( input, data[i] );
	}

	return data;
}
