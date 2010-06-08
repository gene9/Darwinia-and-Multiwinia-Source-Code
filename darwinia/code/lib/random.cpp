#include "lib/universal_include.h"

#include <stddef.h>
#include <stdlib.h>

static long holdrand = 1L;

void  darwiniaSeedRandom (unsigned int seed)
{
        holdrand = (long) seed;
}

int  darwiniaRandom ()
{
        return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}