#ifndef FIXED_FLOAT_H
#define FIXED_FLOAT_H
 
#include <ostream>
#include <math.h>

//
// Numeric type definitions
//
#ifndef __MACTYPES__ // avoid clash with Mac OS X typedefs
typedef unsigned long long UInt64;
typedef signed long long SInt64;
typedef int SInt32;
typedef unsigned int UInt32;
#endif

#if defined(__BIG_ENDIAN__)
	typedef struct { SInt64 hi; UInt64 lo; } SInt128;
	typedef struct { UInt64 hi; UInt64 lo; } UInt128;
#elif defined(__LITTLE_ENDIAN__)
	typedef struct { UInt64 lo; SInt64 hi; } SInt128;
	typedef struct { UInt64 lo; UInt64 hi; } UInt128;
#else
    //#warning unknown endianness
#endif
typedef SInt64 SFix64;

inline double CLAMP(double _f)
{
	union
	{
		double doublePart;
		unsigned long long intPart;
	} value;
	
	value.doublePart = _f;
	value.intPart &= ~(unsigned long long)0xF;
	
	return value.doublePart;
}

class Fixed
{
	private:
		inline Fixed(double _f)
		{
			m_value = CLAMP(_f);
		}
	
	public:
		static const Fixed MAX;
		static const Fixed PI;
		
		Fixed()
		{
			m_value = 0.0;
		}
		Fixed(SInt32 _n)
		{
			m_value = CLAMP(double(_n));
		}
		Fixed(const Fixed& _f) // copy constructor
		{
			m_value = _f.m_value;
		}
		
		static Fixed FromDouble(double _f)
		{
			return Fixed(_f);
		}
		static Fixed Hundredths(int _hundredths); // construct decimal numbers

		unsigned long long InternalValue() const 
		{
			return *(unsigned long long *)&m_value;
		} 
		
		inline Fixed& operator += (const Fixed& _rhs)
		{
			m_value = CLAMP(m_value + _rhs.m_value);
			return *this;
		}

		inline Fixed& operator -= (const Fixed& _rhs)
		{
			m_value = CLAMP(m_value - _rhs.m_value);
			return *this;
		}

		inline Fixed& operator *= (const Fixed& _rhs)
		{
			m_value = CLAMP(m_value * _rhs.m_value);
			return *this;
		}

		inline Fixed& operator /= (const Fixed& _rhs)
		{
			m_value = CLAMP(m_value / _rhs.m_value);
			return *this;
		}
		
		inline bool operator < (const Fixed& _rhs) const
		{
			return m_value < _rhs.m_value;
		}

		inline bool operator <= (const Fixed& _rhs) const
		{
			return m_value <= _rhs.m_value;
		}

		inline bool operator == (const Fixed& _rhs) const
		{
			return m_value == _rhs.m_value;
		}

		inline bool operator != (const Fixed& _rhs) const
		{
			return m_value != _rhs.m_value;
		}

		inline bool operator >= (const Fixed& _rhs) const
		{
			return m_value >= _rhs.m_value;
		}

		inline bool operator > (const Fixed& _rhs) const
		{
			return m_value > _rhs.m_value;
		}
		
		friend Fixed operator + (const Fixed& _lhs, const Fixed& _rhs);
		friend Fixed operator - (const Fixed& _lhs, const Fixed& _rhs);
		friend Fixed operator * (const Fixed& _lhs, const Fixed& _rhs);
		friend Fixed operator / (const Fixed& _lhs, const Fixed& _rhs);
		
		// unary minus
		inline Fixed operator -() const
		{
			Fixed opposite;
			opposite.m_value = -m_value;
			return opposite;
		}
		
		// for explicit conversions
		double DoubleValue() const
		{
			return m_value;
		}
		SInt32 IntValue() const;
				
		// Temporary, while we replace bare floats in source
		//operator float() { return FloatValue(); }
		
		Fixed abs()
		{
			Fixed absValue = *this;
			
			if (absValue.m_value < 0)
				absValue.m_value = -absValue.m_value;
				
			return absValue;
		}
	
		friend Fixed sqrt(const Fixed &number); 
		friend std::ostream& operator << (std::ostream& _os, const Fixed& _f);

	protected:
	
		double m_value;
};

inline Fixed operator + (const Fixed& _lhs, const Fixed& _rhs)
{
	return Fixed(_lhs.m_value + _rhs.m_value);
}

inline Fixed operator - (const Fixed& _lhs, const Fixed& _rhs)
{
	return Fixed(_lhs.m_value - _rhs.m_value);
}

inline Fixed operator * (const Fixed& _lhs, const Fixed& _rhs)
{
	return Fixed(_lhs.m_value * _rhs.m_value);
}

inline Fixed operator / (const Fixed& _lhs, const Fixed& _rhs) 
{
	return Fixed(_lhs.m_value / _rhs.m_value);
}

Fixed min(const Fixed &a, const Fixed &b);
Fixed max(const Fixed &a, const Fixed &b);

// Carmack's algorithm from Quake, adapted to use the Fixed class
// and double-precision floats.
inline
Fixed sqrt(const Fixed &number)
{
    Fixed x, y;
    Fixed f = Fixed::FromDouble(1.5);

    x = number * Fixed::FromDouble(0.5);
    y  = number;

	union
	{
		double doublePart;
		long long intPart;
	} value;
	
	value.doublePart = y.m_value;
	value.intPart = 0x5FE6EB3BE0000000LL - ( value.intPart >> 1 );

	y.m_value = value.doublePart;

	y  = y * ( f - ( x * y * y ) );
    y  = y * ( f - ( x * y * y ) );
    return number * y;
}

Fixed sin(const Fixed &f);
Fixed cos(const Fixed &f);
Fixed asin( const Fixed& _x );
Fixed acos( const Fixed& _x );

// For debugging
std::ostream& operator << (std::ostream& _os, const Fixed& _f);

#endif

