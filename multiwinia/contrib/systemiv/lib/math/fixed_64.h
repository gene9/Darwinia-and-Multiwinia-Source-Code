#ifndef FIXED64_H
#define FIXED64_H
 
#include <ostream>
#include <math.h>

//
// Numeric type definitions
//
typedef unsigned long long UInt64;
typedef signed long long SInt64;
typedef int SInt32;
typedef unsigned int UInt32;

#if defined(__BIG_ENDIAN__)
	typedef struct { SInt64 hi; UInt64 lo; } SInt128;
	typedef struct { UInt64 hi; UInt64 lo; } UInt128;
#elif defined(__LITTLE_ENDIAN__)
	typedef struct { UInt64 lo; SInt64 hi; } SInt128;
	typedef struct { UInt64 lo; UInt64 hi; } UInt128;
#else
    #warning unknown endianness
#endif
typedef SInt64 SFix64;

class Fixed
{
	friend std::ostream& operator << (std::ostream& _os, const Fixed& _f);
	
	// for serialization
	friend class Directory;
	friend class DirectoryData;
								
	private:
		Fixed(SFix64 _rawValue);
		
	public:
		static const Fixed MAX;
		static const Fixed PI;
		
		Fixed(); // init to zero
		Fixed(SInt32 _n); // construct from integer
		Fixed(const Fixed& _f); // copy constructor
		static Fixed FromDouble(double _f); // construct from float (NON-PORTABLE)
		static Fixed Hundredths(int _hundredths); // construct decimal numbers
		
		Fixed& operator += (const Fixed& _rhs);
		Fixed& operator -= (const Fixed& _rhs);
		Fixed& operator *= (const Fixed& _rhs);
		Fixed& operator /= (const Fixed& _rhs);
		
		// unary minus
		Fixed operator -() const
		{
			Fixed opposite;
			opposite.m_value = -m_value;
			return opposite;
		}
		
		bool operator < (const Fixed& _rhs) const;
		bool operator <= (const Fixed& _rhs) const;
		bool operator == (const Fixed& _rhs) const;
		bool Fixed::operator != (const Fixed& _rhs) const;
		bool operator >= (const Fixed& _rhs) const;
		bool operator > (const Fixed& _rhs) const;
		
		// for explicit conversions
		double DoubleValue() const;
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

	protected:
	
		SFix64 m_value;
};

Fixed operator + (const Fixed& _lhs, const Fixed& _rhs);
Fixed operator - (const Fixed& _lhs, const Fixed& _rhs);
Fixed operator * (const Fixed& _lhs, const Fixed& _rhs);
Fixed operator / (const Fixed& _lhs, const Fixed& _rhs);

Fixed min(const Fixed &a, const Fixed &b);
Fixed max(const Fixed &a, const Fixed &b);
Fixed sqrt(const Fixed& _x);
Fixed sin(const Fixed& _x);
Fixed cos(const Fixed& _x);

// For debugging
std::ostream& operator << (std::ostream& _os, const Fixed& _f);

#endif

