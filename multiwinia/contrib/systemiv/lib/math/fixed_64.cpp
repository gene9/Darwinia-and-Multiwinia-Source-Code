#include "lib/universal_include.h"

// So we only compile in one implementation of the Fixed class
//
#ifdef FIXED64_NUMERICS

#include "fixed_64.h"

using std::ostream;

static const SFix64 MIN_SFIX64 = 1ULL << 63;
static const SFix64 MAX_SFIX64 = ~(1ULL << 63);
static const SInt32 MAX_SINT32 = 0x7FFFFFFF;
static const SFix64 ONE_HUNDREDTH = 0x28F5C28;

static SInt64 SInt64_Add(SInt64 x, SInt64 y);
static SInt64 SInt64_Subtract(SInt64 x, SInt64 y);
static SFix64 SFix64_Multiply(SFix64 x, SFix64 y);
static SFix64 SFix64_Divide(SFix64 x, SFix64 y);

const Fixed Fixed::MAX = Fixed(MAX_SFIX64);
const Fixed Fixed::PI = Fixed(0x3243F6A88LL);

Fixed Fixed::FromDouble(double _f)
{
	double ignored;
	
	SInt64 intPart = (SInt64)_f;
	UInt64 fracPart = UInt64(modf(_f, &ignored) * 0x100000000ULL);

	Fixed result;
	result.m_value = (intPart << 32) + fracPart;
	return result;
}

Fixed Fixed::Hundredths(int _hundredths)
{
	return Fixed(SFix64_Multiply((SFix64)_hundredths << 32, ONE_HUNDREDTH));
}

Fixed operator + (const Fixed& _lhs, const Fixed& _rhs)
{
	Fixed temp(_lhs);
	
	temp += _rhs;
	return temp;
}

Fixed operator - (const Fixed& _lhs, const Fixed& _rhs)
{
	Fixed temp(_lhs);
	
	temp -= _rhs;
	return temp;
}

Fixed operator * (const Fixed& _lhs, const Fixed& _rhs)
{
	Fixed temp(_lhs);
	
	temp *= _rhs;
	return temp;
}

Fixed operator / (const Fixed& _lhs, const Fixed& _rhs)
{
	Fixed temp(_lhs);
	
	temp /= _rhs;
	return temp;
}

Fixed::Fixed()
{
	m_value = 0;
}

Fixed::Fixed(const Fixed& _f)
{
	m_value = _f.m_value;
}

Fixed::Fixed(SInt32 _n)
{
	m_value = (UInt64)_n << 32;
}

Fixed::Fixed(SInt64 _rawValue)
{
	m_value = _rawValue;
}

Fixed& Fixed::operator += (const Fixed& _rhs)
{
	m_value += _rhs.m_value;
	return *this;
}

Fixed& Fixed::operator -= (const Fixed& _rhs)
{
	m_value -= _rhs.m_value;
	return *this;
}

Fixed& Fixed::operator *= (const Fixed& _rhs)
{
	m_value = SFix64_Multiply(m_value, _rhs.m_value);
	return *this;
}

Fixed& Fixed::operator /= (const Fixed& _rhs)
{
	m_value = SFix64_Divide(m_value, _rhs.m_value);
	return *this;
}

bool Fixed::operator < (const Fixed& _rhs) const
{
	return m_value < _rhs.m_value;
}

bool Fixed::operator <= (const Fixed& _rhs) const
{
	return m_value <= _rhs.m_value;
}

bool Fixed::operator == (const Fixed& _rhs) const
{
	return m_value == _rhs.m_value;
}

bool Fixed::operator != (const Fixed& _rhs) const
{
	return m_value != _rhs.m_value;
}

bool Fixed::operator >= (const Fixed& _rhs) const
{
	return m_value >= _rhs.m_value;
}

bool Fixed::operator > (const Fixed& _rhs) const
{
	return m_value > _rhs.m_value;
}

double Fixed::DoubleValue() const
{
	SInt64 fixedValue = m_value;
	double floatValue;
			
	bool isNegative = ( fixedValue < 0 );
	if (isNegative)
		fixedValue = 0 - fixedValue;
	
	floatValue = (fixedValue >> 32) + (fixedValue & 0xFFFFFFFF) / (float)0x100000000ULL;
	
	return isNegative ? -floatValue : floatValue;
}

SInt32 Fixed::IntValue() const
{
	return m_value >> 32;
}

Fixed min(const Fixed &a, const Fixed &b)
{
	return a < b ? a : b;
}

Fixed max(const Fixed &a, const Fixed &b)
{
	return a > b ? a : b;
}

static SFix64 SFix64_Multiply(SFix64 x, SFix64 y)
{
	SFix64		z;

	if (x && y)
	{
		UInt64		AC, AD, BC, BD;
		UInt64		a, b, c, d;
		int			sign = 0;
		
		/* Convert to unsigned words */
		if (x < 0) sign = 1 - sign, x = 0 - x;
		a = (x >> 32) & 0xFFFFFFFFULL; 
		b = x & 0xFFFFFFFFULL;

		if (y < 0) sign = 1 - sign, y = 0 - y;
		c = (y >> 32) & 0xFFFFFFFFULL; 
		d = y & 0xFFFFFFFFULL;
		
		/* Multiply terms */
		AC = (a * c);
		AD = (a * d);
		BC = (b * c);
		BD = (b * d);
		/* Check for overflow, then add products */
		if (AC > MAX_SINT32)
			z = MAX_SFIX64;
		else if (AC + (AD >> 32) + (BC >> 32) > MAX_SINT32)
			z = MAX_SFIX64;
		else
			z = (AC << 32) + AD + BC + (BD >> 32);
			
		if (sign) z = 0 - z;
		
	}
	else z = 0;
		
	return(z);
}

static UInt128 UInt128_ShiftLeft(UInt128 x, UInt64 y)
{
	if (y >= 128)
	{
		x.hi = 0;
		x.lo = 0;
	}
	else if (y >= 64)
	{
		x.hi = x.lo << (y - 64);
		x.lo = 0;
	}
	else if (y)
	{
		x.hi = (x.hi << y) | (x.lo >> (64 - y));
		x.lo = (x.lo << y);
	}
	return(x);
}

static UInt128 UInt128_Assign64(UInt64 x)
{
	UInt128 z;
	z.hi = 0;
	z.lo = x;
	return(z);
}

static UInt64 UInt128_Return64(UInt128 x)
{
	return(x.lo);
}

static SInt64 UInt128_Compare(UInt128 x, UInt128 y)
{
	return((SInt64) ((x.hi != y.hi) ? (x.hi - y.hi) : (x.lo - y.lo)));
}

static void DivideModulo128(UInt128 x, UInt128 y, UInt128 *div, UInt128 *mod)
{
	UInt64		x_hi = x.hi, x_lo = x.lo;
	UInt64		y_hi = y.hi, y_lo = y.lo;
	UInt64		div_hi = 0, div_lo = 0;
	UInt64		mod_hi = 0, mod_lo = 0;
	UInt64		z_hi, z_lo;
	int			i, j, bit, bits;
	
	/* All 128-bit operations have been inlined by hand for speed */

	/* y == 0 */
	if (!y_hi && !y_lo) goto DONE;
	/* if (y > x) */
	if (y_hi > x_hi) { mod_hi = x_hi, mod_lo = x_lo; goto DONE; } 
	if (y_hi == x_hi)
	{
		if (y_lo > x_lo) { mod_hi = x_hi, mod_lo = x_lo; goto DONE; }
		/* if (y == x) */
		if (y_lo == x_lo) { div_lo = 1; goto DONE; } 
	}

	/* Shift the upper bits of the dividend into the remainder */
	for(bits = 128; (mod_hi < y_hi) || 
		((mod_hi == y_hi) && (mod_lo < y_lo)); bits--)
	{
		bit = x_hi >> 63;
		/* mod = (mod << 1) | bit */
		mod_hi = (mod_hi << 1) | (mod_lo >> 63);
		mod_lo = (mod_lo << 1) | bit;
		z_hi = x_hi, z_lo = x_lo;
		/* x = x << 1 */
		x_hi = (x_hi << 1) | (x_lo >> 63);
		x_lo = (x_lo << 1);
	}
	
	/* Undo the last iteration */
	x_hi = z_hi, x_lo = z_lo;
	/* mod = mod >> 1 */
	mod_lo = (mod_lo >> 1) | (mod_hi << 63);
	mod_hi = (mod_hi >> 1);
	bits++;

	/* Subtract the divisor from the value in the remainder. The high order 
	   bit of the result become a bit of the quotient (division result). */
	for(i=0; i<bits; i++)
	{
		bit = x_hi >> 63;
		/* mod = (mod << 1) | bit */
		mod_hi = (mod_hi << 1) | (mod_lo >> 63);
		mod_lo = (mod_lo << 1) | bit;
		/* z = mod - y */
		z_hi = mod_hi - y_hi;
		z_lo = mod_lo - y_lo;
		if (z_lo > mod_lo) z_hi--;
		/* j = (z_hi & (1 << 63)) ? 0 : 1; */
		j = 1 - (z_hi >> 63);
		/* x = x << 1 */
		x_hi = (x_hi << 1) | (x_lo >> 63);
		x_lo = (x_lo << 1);
		/* div = (div << 1) | j */
		div_hi = (div_hi << 1) | (div_lo >> 63);
		div_lo = (div_lo << 1) | j;
		if (j) mod_hi = z_hi, mod_lo = z_lo;
	}

DONE:
	if (div) div->hi = div_hi, div->lo = div_lo;
	if (mod) mod->hi = mod_hi, mod->lo = mod_lo;
}

static SFix64 SFix64_Divide(SFix64 x, SFix64 y)
{
	SFix64		z;

	if (x && y)
	{
		UInt128	X, Y, DIV;
		int		sign = 0;

		/* Convert to unsigned long longs */
		if (x < 0) sign = 1 - sign, x = 0 - x;
		if (y < 0) sign = 1 - sign, y = 0 - y;
		X = UInt128_ShiftLeft(UInt128_Assign64(x), 32);
		Y = UInt128_Assign64(y);

  		DivideModulo128(X, Y, &DIV, 0);

		/* Extract the unsigned long, and convert to signed */
		if (UInt128_Compare(DIV, UInt128_Assign64(MAX_SFIX64)) < 0)
			z = UInt128_Return64(DIV);
		  else 
			z = MAX_SFIX64;
		if (sign) z = 0 - z;
	}
	else z = 0;
		
	return(z);
}


namespace fixed64
{
	/* We use Taylor expansion to define sin(x) between 0 and pi/4 (7 iterations) */

	const Fixed zero( 0 ),
				one( 1 ),
				minusOne( -1 ),
				threeFactorial( 6 ),
				fiveFactorial( 120 ),
				sevenFactorial( 5040 ),
				nineFactorial( 362880 ),
				elevenFactorial( 39916800 ),
				oneThousandAndTwentyFour( 1024 ),
				// These need to be converted to certain representations
				half( Fixed::Hundredths(50) ),
				piOverTwo( Fixed::FromDouble(1.5707964f) ),
				minusPiOverTwo( Fixed::FromDouble(-1.5707964f) ),
				twoThousandAndFortyEightOverPi( Fixed::FromDouble(651.8986f) );


	SInt64 sinInternals[1025] = {
		#include "fixed64_sin_table.inc"		
	};
	
	SInt64 asinInternals[1025] = {
		#include "fixed64_asin_table.inc"				
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
	else if (1024 <= index < 2048)
		return fixed64::sinTable[1024 - (index - 1024)];
	else if (2048 <= index < 3072)
		return -fixed64::sinTable[index - 2048];
	else if (3072 <= index)
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


Fixed sqrt( const Fixed &_x )
{
	// See: http://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Babylonian_method

	// We need to use an iterative (fixed point) algorithm to 
	// approximate sqrt. We use babylonian algorithm:
	Fixed prev = _x;
	int iterations = 0;
	
	assert(_x >= fixed64::zero);
	
	do {
		Fixed next = fixed64::half * ( prev + _x / prev );
		if (next == prev || ++iterations == 20)
			return next;
		prev = next;
	} while (true);
}

#endif // FIXED64_NUMERICS
