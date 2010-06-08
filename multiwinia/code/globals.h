#ifndef INCLUDED_GLOBALS_H
#define INCLUDED_GLOBALS_H

#define NUM_TEAMS               6								// If increased, location.cpp's IsFriend method must be also be updated.
#define NUM_SLICES_PER_FRAME    10                              // Num slices to break up heavy weight physics into
#define IAMALIVE_PERIOD         0.1                            // Clients must send IAmAlive this often
#define MINIMUM_RENDER_PERIOD   1.0                            // Render at least this frequently
#define GRAVITY	                10.0


#endif
