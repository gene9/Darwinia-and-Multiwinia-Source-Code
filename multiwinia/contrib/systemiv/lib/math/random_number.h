#ifndef _included_randomnumbers_h
#define _included_randomnumbers_h


/*
 * =============================================
 *	Basic random number generators
 *
 */

#define APP_RAND_MAX    32767


void    AppSeedRandom   (unsigned int seed);                                
int     AppRandom       ();                                                 // 0 <= result < APP_RAND_MAX

double   frand           (double range = 1.0f);                               // 0 <= result < range
double   sfrand          (double range = 1.0f);                               // -range < result < range




/*
 * =============================================
 *	Network Syncronised random number generators
 *  
 *  These must only be called in deterministic
 *  net-safe code.
 *
 *  Don't call the underscored functions directly -
 *  instead call syncrand/syncfrand and the preprocessor
 *  will do the rest.
 *
 */


void            syncrandseed    ( unsigned long seed = 0 );
unsigned long   getsyncrandseed ();
unsigned long   _syncrand       ();
double           _syncfrand      ( double range = 1.0f );                     // 0 <= result < range
double           _syncsfrand     ( double range = 1.0f );                     // -range < result < range




/*
 * ==============================================
 *  Debug versions of those Net sync'd generators
 *
 *  Handy for tracking down Net Syncronisation errors
 *  To use : #define TRACK_SYNC_RAND in universal_include.h
 *         : then call DumpSyncRandLog when you know its gone wrong
 *
 *
 */

unsigned long   DebugSyncRand   (const char *_file, int _line );
double           DebugSyncFrand  (const char *_file, int _line, double _min, double _max );
void            DumpSyncRandLog (const char *_filename);
void 			FlushSyncRandLog ();


#ifdef TRACK_SYNC_RAND
    #define         syncrand()      DebugSyncRand(__FILE__,__LINE__)
    #define         syncfrand(x)    DebugSyncFrand(__FILE__,__LINE__,0.0f,x)
    #define         syncsfrand(x)   DebugSyncFrand(__FILE__,__LINE__,-x/2.0,x/2.0)
#else
    #define        syncrand          _syncrand
    #define        syncfrand(x)      _syncfrand(x)
    #define        syncsfrand(x)     _syncsfrand(x)
#endif


// Temporary until Net Sync problem is fixed

void            SyncRandLog     ( const char *_message, ... );
void			SyncRandLogText	( const char *_text );

/*
 * =============================================
 *	Statistics based random number generatores
 */


double       RandomNormalNumber  ( double _mean, double _range );	            // result ~ N ( mean, range/3 ), 					
                                                                            // mean - range < result < mean + range	
  
double       RandomApplyVariance ( double _num, double _variance );			// Applies +-percentage Normally distributed variance to num
                                                                            // variance should be in fractional form eg 1.0, 0.5 etc                
                                                                            // _num - _variance * _num < result < _num + _variance * _num



#endif
