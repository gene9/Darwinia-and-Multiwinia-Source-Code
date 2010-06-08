#ifndef _included_hash_h
#define _included_hash_h

/*
 *	Hash data using SHA1
 *  Can pass in a seed hash token
 *
 *  result is written into
 */


void HashData( const char *_data, int _hashToken, char *_result );                        // No more than 500 bytes of data please

void HashData( char *_data, char *_result );                                        // Any amount of data is Ok (trashes data)

void HashDataVoid( void *_data, int _dataLength, unsigned *hash /* Array of 5 unsigned ints */ ); // (trashes data)

#endif
