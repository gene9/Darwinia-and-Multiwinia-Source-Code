#include "lib/universal_include.h"

#include "lib/input/inputfilterspec.h"


unsigned long newFilterSpecID() {
	static unsigned long nextID = 0;
	return nextID++;
}
