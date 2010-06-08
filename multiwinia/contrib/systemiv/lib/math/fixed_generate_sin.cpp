#include "fixed.h"

#include <cmath>
#include <iostream>

const double piOverTwoThousandAndFortyEight = M_PI / 2048.0;

int main()
{
	for (int i = 0; i <= 1024; i++) {
		double x = i * piOverTwoThousandAndFortyEight; 
		double result = sin( x );
		
		std::cout << "0x" << std::hex << Fixed::FromFloat(result).InternalValue() << "ULL";
		if (i < 1024)
			std::cout << ",\n";
	}
	std::cout << "\n";
}