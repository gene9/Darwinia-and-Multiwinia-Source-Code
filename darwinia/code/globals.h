#ifndef INCLUDED_GLOBALS_H
#define INCLUDED_GLOBALS_H

#define NUM_TEAMS               4
#define NUM_SLICES_PER_FRAME    10                              // Num slices to break up heavy weight physics into
#define SERVER_ADVANCE_PERIOD   0.1f                            // Server ticks at this rate
#define SERVER_ADVANCE_FREQ     10.0f                           // Server ticks at this rate
#define IAMALIVE_PERIOD         0.1f                            // Clients must send IAmAlive this often
#define MINIMUM_RENDER_PERIOD   1.0f                            // Render at least this frequently
#define GRAVITY	                10.0f


#endif
