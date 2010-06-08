#include "lib/universal_include.h"

#include "lib/input/input_types.h"
#include "lib/input/control_bindings.h"

static ControlAction s_actions[] = {

	#define DEF_CONTROL_TYPE(x,y) "x", y,
	#include "lib/input/control_types.inc"
	#undef DEF_CONTROL_TYPE
	
	"",                                  INPUT_TYPE_ANY,

	NULL,                                NULL

};

