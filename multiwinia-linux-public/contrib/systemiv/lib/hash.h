#ifndef __SYSTEMIV_HASH
#define __SYSTEMIV_HASH

#include "contrib/unrar/rarbloat.h"
#include "contrib/unrar/sha1.h"

inline
void Hash( hash_context &c, unsigned i )
{
       unsigned char syncBuffer[4] = {
               i & 0xFF,
               (i >> 8) & 0xFF,
               (i >> 16) & 0xFF,
               (i >> 24) & 0xFF
       };

       hash_process( &c, (unsigned char *)syncBuffer, 4 );
}

inline
void Hash( hash_context &c, int i )
{
       Hash( c, (unsigned) i );
}

inline 
void Hash( hash_context &c, float f )
{
	union 
	{
		float floatPart;
		unsigned intPart;
	} value;

	value.floatPart = f;	
	Hash( c, value.intPart );
}

inline
void Hash( hash_context &c, double d )
{
	union
	{
		double doublePart;
		unsigned long long intPart;
	} value;
	
	value.doublePart = d;
	const unsigned long long i = value.intPart;
	
	unsigned char syncBuffer[8] = {
		i & 0xFF,
		(i >> 8) & 0xFF,
		(i >> 16) & 0xFF,
		(i >> 24) & 0xFF,
		(i >> 32) & 0xFF,
		(i >> 40) & 0xFF,
		(i >> 48) & 0xFF,
		(i >> 56) & 0xFF,
	};
	
	hash_process( &c, (unsigned char *)syncBuffer, 8 );
}

inline
void Hash( hash_context &c, const Vector3 &v )
{
	Hash(c, v.x);
	Hash(c, v.y);
	Hash(c, v.z);
}

#ifdef USE_FIXED
inline
void Hash( hash_context &c, const Fixed &f )
{
       Hash(c, f.DoubleValue() );
}
#endif

#endif
