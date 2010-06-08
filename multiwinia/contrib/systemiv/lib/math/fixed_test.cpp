#include "fixed.h"
#include <iostream>
#include <cmath>

namespace fixed64 {
	Fixed TaylorExpansionSinFirstQuarter( const Fixed &x );
}

int main()
{
	std::cout 
		<< "3.2 + 4.5 = " << Fixed::FromFloat(3.2f) + Fixed::FromFloat(4.5f) << "\n" 
		<< "3 + 4 = " << Fixed(3) + Fixed(4) << "\n"
		<< "10/2 = " << Fixed(10) / Fixed(2) << "\n"
		<< "10/0.5 = " << Fixed(10) / Fixed::FromFloat(0.5f) << "\n"
		<< "sin(0) = " << sin(Fixed(0)) << "\n"
		<< "sin(pi/2) = " << sin(Fixed::FromFloat(3.1415926f / 2.0f)) << "\n"
		<< "cos(0) = " << cos(Fixed(0))  << "\n"
		<< "sqrt(10000) = " << sqrt(Fixed(10000)) << "\n"
		<< std::endl;
		
	for (double x = 0; x < M_PI / 2 + 0.5; x += 0.1) {
		// std::cout 
		// 	<< "tableSin( " << 360.0 * x / (2 * M_PI) << " degrees) = " << sin(Fixed(float(x))) << " "
		// 	<< "  actual = " << sin(x) << " \n";

		Fixed fx = Fixed::FromFloat(x);
		Fixed sinfx = sin(fx);
		//Fixed asinsinfx = asin(sinfx);
		
		std::cout << "sin( " << x << ") = " << sin(fx) << ", Correct answer is " << sin(x) /*<< ", asin = " << asin(sin(fx))*/ << "\n";			
//		std::cout << "cos( " << x << ") = " << cos(fx) /*<< ", acos = " << acos(cos(fx))*/ << "\n";			
	}
	
}