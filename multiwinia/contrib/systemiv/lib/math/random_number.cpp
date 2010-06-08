#include "lib/universal_include.h"

#include <stdarg.h>

#include "lib/string_utils.h"
#include "lib/tosser/llist.h"
#include "lib/preferences.h"

#include "random_number.h"
#include <fstream>

static LList<char *> s_syncrandlog;


static long holdrand = 1L;
static int s_callCount = 0;

void AppSeedRandom (unsigned int seed)
{
    holdrand = (long) seed;
}

int AppRandom ()
{
    return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

#ifdef TRACK_SYNC_RAND
    static int s_maxSize = 100000;
#endif


void SyncRandLog( const char *_message, ... )
{
#ifdef TRACK_SYNC_RAND
	const int maxChars = 1024;

    char *buf = new char[maxChars];
    va_list ap;
    va_start (ap, _message);
	int charsWritten = vsnprintf( buf, maxChars - 1, _message, ap);
	buf[maxChars - 1] = '\0';

    s_syncrandlog.PutData( buf );

    // Keep the size down (assuming we aren't doing heavy duty logging)

    while( s_syncrandlog.Size() > s_maxSize )
    {
        char *elemZero = s_syncrandlog[0];
        s_syncrandlog.RemoveData(0);
        delete [] elemZero;
    }
#endif
}


void SyncRandLogText( const char *_message )
{
#ifdef TRACK_SYNC_RAND
    s_syncrandlog.PutData( strdup( _message ) );

    // Keep the size down (assuming we aren't doing heavy duty logging)

    while( s_syncrandlog.Size() > s_maxSize )
    {
        char *elemZero = s_syncrandlog[0];
        s_syncrandlog.RemoveData(0);
        delete [] elemZero;
    }
#endif
}

char *HashDouble( double value, char *buffer );

void LogSyncRand(const char *_file, int _line, double _num )
{
#ifdef TRACK_SYNC_RAND
    const char *fromPoint = strrchr( _file, '/' );
    if( !fromPoint ) fromPoint = strrchr( _file, '\\' );
    if( fromPoint ) fromPoint += 1;
    else fromPoint = _file;
	
	char buf[256];
	HashDouble(_num, buf);
    SyncRandLog( "%8d | %s | %s line %d", s_callCount, buf, fromPoint, _line );
    
    ++s_callCount;
#endif
}


unsigned long DebugSyncRand(const char *_file, int _line )
{    
    unsigned long result = _syncrand();

    LogSyncRand( _file, _line, result );

    return result;
}


double DebugSyncFrand(const char *_file, int _line, double _min, double _max )
{
    double range = _max - _min;
    double result = _min + (_syncrand() * range) * (1/4294967296.0);
	
    LogSyncRand( _file, _line, result );

    return result;
}


void DumpSyncRandLog (const char *_filename)
{
	std::ofstream file( _filename, std::ios::binary /* Use Unix line endings */ );

    if( !file ) return;

	file << "SyncRandSeed " << getsyncrandseed() << " (as signed int: " << (int) getsyncrandseed() << ")\n";
    
    for( int i = 0; i < s_syncrandlog.Size(); ++i )
    {
        char *thisEntry = s_syncrandlog[i];
		file << thisEntry << "\n";
    }

	file.close();
}

void FlushSyncRandLog ()
{
    while( s_syncrandlog.ValidIndex ( 0 ) )
    {
        char *elemZero = s_syncrandlog[0];
        s_syncrandlog.RemoveData(0);
        delete [] elemZero;
    }
}



// ****************************************************************************
// Mersenne Twister Random Number Routines
// ****************************************************************************

/* This code was taken from
   http://www.math.keio.ac.jp/~matumoto/MT2002/emt19937ar.html
   
   C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)  
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   Any feedback is very welcome.
   http://www.math.keio.ac.jp/matumoto/emt.html
   email: matumoto@math.keio.ac.jp
*/

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */
static unsigned long mag01[2];
static unsigned long s_randseed = 0;

/* initializes mt[N] with a seed */
void syncrandseed(unsigned long s)
{
    if( s == (unsigned long) -1 ) s = 5489UL;

    s_randseed = s;

    s_callCount = 0;
    LogSyncRand( "SyncRandSeed called", 0, s );
    
    mag01[0]=0x0UL;
    mag01[1]=MATRIX_A;

    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) {
        mt[mti] = 
	    (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}


unsigned long getsyncrandseed()
{
    return s_randseed;
}


// Generates a random number on [0,0xffffffff]-interval
unsigned long _syncrand()
{
    unsigned long y;
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) { /* generate N words at one time */
        int kk;

        if (mti == N+1)         /* if init_genrand() has not been called, */
            syncrandseed();     /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }
  
    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}


double _syncfrand( double range )
{
    double result = (_syncrand() * range)/4294967296.0; 
    return result;
}


double _syncsfrand( double range )
{
    double result = (_syncfrand(1.0) - 0.5) * range; 
    return result;
}


double frand(double range)
{ 
    return range * ((double)AppRandom() / (double)APP_RAND_MAX); 
}


double sfrand(double range)
{
    return (0.5 - (double)AppRandom() / (double)(APP_RAND_MAX)) * range; 
}



double RandomNormalNumber ( double mean, double range )	
{	
	// result ~ N ( mean, range/3 )

	double s = 0;

	for ( int i = 0; i < 12; ++i )
    {
		s += syncfrand(1.0);
    }
    
	s = ( s - 6.0 ) * ( range/3.0 ) + mean;
    
	if ( s < mean - range ) s = mean - range;
	if ( s > mean + range ) s = mean + range;

	return s;
}


double RandomApplyVariance ( double num, double variance )
{
	double variancefactor = 1.0 + RandomNormalNumber ( 0.0, variance );
	num *= variancefactor;								

	return num;
}
