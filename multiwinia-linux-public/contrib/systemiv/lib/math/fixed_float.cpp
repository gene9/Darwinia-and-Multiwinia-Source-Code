#include "lib/universal_include.h"

// So we only compile in one implementation of the Fixed class
//
#ifdef FLOAT_NUMERICS

#include <float.h>

#include "fixed_float.h"
#include "lib/debug_utils.h"
#include "random_number.h"

const Fixed Fixed::MAX = Fixed::FromDouble(DBL_MAX);
const Fixed Fixed::PI = Fixed::FromDouble(3.14159);

Fixed Fixed::Hundredths(int _hundredths)
{
	return Fixed::FromDouble(_hundredths / 100.0);
}

SInt32 Fixed::IntValue() const
{
	return (SInt32)m_value;
}

Fixed min(const Fixed &a, const Fixed &b)
{
	return a < b ? a : b;
}

Fixed max(const Fixed &a, const Fixed &b)
{
	return a > b ? a : b;
}

namespace fixed64 {
	const Fixed zero( 0 ),
		one( 1 ),
		// Need to convert these to internal representation
		twoThousandAndFortyEightOverPi( Fixed::FromDouble(651.8986f) ),
		oneThousandAndTwentyFour( 1024 ),
		half( Fixed::FromDouble(0.5f) ),
		piOverTwo( Fixed::FromDouble(1.5707964f) );
		

	SInt64 sinInternals[1025] = {
		#include "float_sin_table.inc"		
	};

	SInt64 asinInternals[1025] = {
		#include "float_asin_table.inc"				
	};

	Fixed *sinTable = (Fixed *) sinInternals;
	Fixed *asinTable = (Fixed *) asinInternals;
}

Fixed sin( const Fixed &_x )
{
	if (_x < fixed64::zero) 
		return -sin( -_x );
		
	// We store a table of 1024 entries which describes the first quarter
	// of the sin curve. 
	
	// So: 0 .. PI/2 == 0 .. 1024
	//     0 .. 2 PI == 0 .. 4096
	
	// This gives us a resolution of 4096 points, we could make it more
	// precise by interpolating.		
	int index = (_x * fixed64::twoThousandAndFortyEightOverPi).IntValue() % 4096;
	
	// std::cout << " (index = " << index << ")";
		
	if (index < 1024)		
		return fixed64::sinTable[index];
	else if (1024 <= index && index < 2048)
		return fixed64::sinTable[1024 - (index - 1024)];
	else if (2048 <= index && index < 3072)
		return -fixed64::sinTable[index - 2048];
	else // if (3072 <= index)
		return -fixed64::sinTable[1024 - (index - 3072)];
}

Fixed cos( const Fixed &_x )
{
	return sin(fixed64::piOverTwo - _x);
}

Fixed asin( const Fixed& _x )
{
	// Return results between -pi/2 and +pi/2 
	if (_x < fixed64::zero)
		return - asin( - _x );
		
	if (_x >= fixed64::one)
		return fixed64::piOverTwo;
		
	int index = (_x * fixed64::oneThousandAndTwentyFour).IntValue();
	
	return fixed64::asinTable[index];
}

Fixed acos(const Fixed& _x)
{
	// Return results bween pi and 0  pi/2 - (-pi/2) = pi... 
	// pi/2 - (pi/2) = 0
	return fixed64::piOverTwo - asin(_x);
}

std::ostream& operator << (std::ostream& _os, const Fixed& _f)
{
	return _os << "0x" << std::hex << _f.InternalValue() << "(" << _f.m_value << ")";
}

#endif // FLOAT_NUMERICS
