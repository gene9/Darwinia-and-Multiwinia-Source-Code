#ifndef FIXED_H
#define FIXED_H

// Decide what implemention of the Fixed class we'll use
// 
#if defined(FLOAT_NUMERICS)
	#include "fixed_float.h" // floating-point based datatype
#elif defined(FIXED64_NUMERICS)
	#include "fixed_64.h" // 64-bit fixed point datatype
#else
	#error No numerics implementation defined
#endif

#endif
